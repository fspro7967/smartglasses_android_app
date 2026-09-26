# 01 — DeviceHandler 支持流式下行（不拒绝、可积压、可中止）

Status: resolved
Type: task

对应 `spec.md` 步骤 1。

## 问题

`writeData()` 在队列非空时直接 `writeError("正在发送上一条数据，请稍候")` 返回。
对「一条 AI 回复」这种原子写是对的，对音频流是错的：音频每几十毫秒来一块，
除第一块之外全会被丢掉，且每丢一块就往状态栏刷一条错误。

## 做法

- 队列元素改为 `struct WriteChunk { QByteArray data; bool oneShot; }`
  - `oneShot` 只影响收尾：只有普通下行整批发完才发 `writeFinished`
- `enqueueStreamData()`：不拒绝，按 MTU 分包入队；积压超过 `kMaxStreamQueueBytes`
  （64 KiB）时**停止接收**并发 `streamAborted()`
- `clearStreamQueue()`：只摘流式分片，保留普通下行已排队的整批数据
- `writeChunkSize()` 从 `writeData()` 里抽出，两条路径共用
- `streamTickIntervalMs()`：`chunkSize × 1000 / (16000 × 1.5)`，钳在 10–50 ms。
  MTU 大时包大间隔长，MTU 小时间隔短；固定 15 ms 只对大包合适
- `startSendingIfIdle()` + `m_writeInFlight`：原代码靠「忙时拒绝」隐式保证
  `WriteWithResponse` 不会并发发两包，流式下不再成立，必须显式记账
- `onServiceError()` 在流式数据排队时报错时，额外发一次 `streamAborted()`：
  否则调用方会按真实时间继续喂数据，而写入只会一直失败

## Comments

**2026-09-12（构建机）** — 已实施。

- `-fsyntax-only`（compile_commands.json 里的 Android 编译参数）：`devicehandler.cpp` exit 0。
- `ninja libsmartglasses_android_app_arm64-v8a.so`：moc / qmlcachegen / 链接全部通过。
- **未经真机验证**：发包节拍、积压上限、溢出判定都还没有在真实链路上跑过，
  见工单 04。
