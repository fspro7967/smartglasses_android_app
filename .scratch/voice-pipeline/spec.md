# 现状记录:语音管线(上行音频 → 识别 → 应答 → 朗读)

**状态:** 盘点记录,不是冻结规格。工单来自两次代码通读(2026-10-03、2026-11-11)
**日期:** 2026-11-11

---

## 这份文档要做什么

「语音管线」指从蓝牙上行音频进入手机,到一条回复开始被朗读为止的这一段:

```
上行 PCM ─► WhisperManager ─► MainWindow::onTranscriptionResult ─► MsgSender ─► MainWindow::onAIResponse ─► speakReply
           成段 / 排队 / 转写                                       一次请求一条回复                         切句、排队朗读
```

合成与播放本身归 `.scratch/offline-tts/`,蓝牙下行归 `.scratch/ble-audio-out/` 与
`.scratch/board-esp32-link/`。本目录没有一份事先写好的规格——这一段是逐步长出来的,
工单都是事后通读代码发现的问题。本文只记录**现在是怎么工作的**和**哪些事还没人决定**。

## 一、现状(2026-11-11 之后)

| 环节 | 行为 | 出处 |
|---|---|---|
| 成段 | 上行 PCM 凑满 3 秒切一段;600 ms 没有新数据时把不足 3 秒(但至少 0.5 秒)的尾巴也成段 | `WhisperManager::feedAudioData` / `flush`,`MainWindow::m_tailFlushTimer` |
| 排队 | 同一时刻只转写一段;最多排 4 段,满了丢最早的并上报 | `WhisperManager::enqueueSegment` |
| 就绪 | 模型在后台线程加载;未就绪时拒收上行音频与文件 | `WhisperManager::initAsync`,`MainWindow::whisperReady` |
| 识别语言 | 固定英文 | `docs/adr/0002-english-in-chinese-out.md` |
| 应答 | 每段转写各发一次**单轮**请求(system prompt + 这一句),没有历史 | `MsgSender::sendToApi` / `sendToOllama` |
| 超时 | API 15 秒 / Ollama 60 秒无数据即失败;失败只显示、不朗读 | `MsgSender`,`issues/06` |
| 回复重叠 | 新回复排在正在朗读的回复后面 | `docs/adr/0006-queue-overlapping-replies.md` |
| 朗读期间的上行 | 照常进入识别,不屏蔽 | `docs/adr/0007-mic-stays-live-while-speaking.md` |
| 断开 / 切换设备 | 丢弃全部未转写的音频,正在转写的那段结果作废 | `MainWindow::resetAudioInput` |

## 二、工单

| # | 标题 | 状态 |
|---|---|---|
| 01 | 音频缓冲被局部变量遮蔽;忙时整块丢弃 | resolved |
| 02 | 尾部音频永不转写;断开时缓冲不清 | resolved(静音检测未做) |
| 03 | 被「暂时」注释掉的 whisper 解码参数 | needs-triage |
| 04 | 没有会话/上下文概念 | needs-triage |
| 05 | Whisper 初始化挡住首屏 | resolved |
| 06 | AI 请求没有超时 | resolved(重试未做) |
| 07 | 断网时没有任何回复能力 | needs-triage |
| 08 | 朗读途中来了新回复,队列被覆盖 | resolved |
| 09 | 朗读期间上行仍进入识别 | wontfix |
| 10 | 「选音频」把 WAV 文件头当 PCM | resolved |
| 11 | 真机验收(2026-11-11 这批修改) | ready-for-human |

状态以各工单文件的 `Status:` 行为准,这张表只是目录。

## 三、还没人决定的事

这三件事互相牵连,建议放在一起决定:

1. **一轮对话的边界在哪里。** 现在是「每 3 秒一段、每段一次提问」。一句话被切成两段就会得到
   两个互不知情的回答;持续上行的设备上短指令要等窗口填满。按语音活动切段(`issues/02` 问题 4、
   `issues/04` 方向 2)能同时解决这两点。
2. **要不要对话历史**(`issues/04` 方向 1)。
3. **回复比朗读快时怎么办。** 现在是无上限地排队(`issues/08` 的已知代价),请求之间也不保证
   回复顺序(`issues/06` 建议做法 4)。

另外两件独立的:`issues/03`(那几行解码参数到底要不要恢复)、`issues/07`(离线应答做不做)。
