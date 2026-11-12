# 05 — 有响应写入失败一次后,蓝牙下行永久卡死

Status: resolved
Type: bug

## 问题

`DeviceHandler::onServiceError()` 开头用下面这个条件判定「这个错误与下行无关」并提前返回:

```cpp
if (m_writeQueue.isEmpty() && m_inflightChunks == 0)
    return;
```

`m_inflightChunks` 只统计**无响应**写入的流式分片。有响应写入(`WriteWithResponse`)时,
「已发出、等确认」的那一包记在 `m_writeInFlight` 里,不在这个条件内。于是:

1. **普通下行(一条 AI 回复)**:最后一包——短回复就是唯一一包——发出后队列已空。它若出错,
   错误被当作无关而忽略,`m_writeInFlight` 与 `m_oneShotPending` 永不复位。此后每一条回复都被
   `writeData()` 以「正在发送上一条数据,请稍候」拒绝,直到重新选择写入目标。
2. **流式回传**:单次错误走「容忍,丢这一包继续」分支后直接 `return`。无响应模式有定时器
   继续推进;有响应模式全靠完成回调推进,而出错的这一包不会再有回调——没人续发。队列涨到
   高水位后背压把合成暂停,界面永远停在「正在朗读」,也不报错。

2026-11-11 通读代码时发现,此前没有工单。

## Answer

**2026-11-11 — 已修(代码在工作区,未经真机验证)。**

- 「有没有下行数据在途」现在把 `m_writeInFlight` 和在途记录都算进去,队列为空不再等于无关。
- 出错的那一包占着的发送名额当场归还:有响应模式清 `m_writeInFlight`,无响应模式递减在途数。
- 流式分片被容忍的错误之后,有响应模式立即 `sendNextWriteChunk()` 续发(队列若已空则走收尾,
  发 `streamDrained`)。
- 普通下行有一包失败:整条作废,`clearWriteQueue()` 复位全部状态并发 `writeError`,
  下一条回复不再被挡住。
- 出错的是哪一类写入,由在途记录 `m_inflightWrites` 按提交顺序判定;这套记账同时用于把
  控制指令的错误与音频分开,见 `.scratch/board-esp32-link/issues/04`。

验证:Android Release 目标编译链接通过。写入错误没法在构建机上制造,真机判据并入 `issues/04`
(2026-11-11 的补充项)。
