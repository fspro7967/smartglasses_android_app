# 07 — README 唯一未完成项:断网时没有任何回复能力

Status: needs-triage
Type: task

## 现状

README 的 Todo(仓库根 `README.md`)现在只剩一条没做:

> - (Maybe)Integrate a simple model on mobile phone to anwser some simple questions when offline, and improve the responding speed.

识别(whisper,本地)与朗读(sherpa-onnx,本地)都已经离线可用,但**中间那一步不行**:
`MainWindow::onTranscriptionResult()` → `MsgSender::sendMessage()` 只有两条路
(`src/msgsender.cpp:73-77`):

- OpenAI 兼容 API(需要公网)
- 局域网 Ollama(需要一台开着 `ollama serve` 的电脑)

两者都不可达时,`errorOccurred` → 状态栏一句「AI API 错误: …」,然后什么也不发生
(按静默策略,这也是刻意的:失败不朗读)。也就是说:**离线时设备能听、能说,但不会想。**

## 需要所有者回答的

1. 做不做?README 里写的是 `(Maybe)`。
2. 做的话走哪条路:
   - 端上小模型(如 0.5B–1.8B 量化模型 + llama.cpp/ONNX Runtime)。
     代价要算清楚:APK 已经 126.73 MiB(spec 第六节),再塞一个数百 MB 的权重不现实,
     可能需要下载式分发,而那与「离线可用」矛盾。
   - 规则/模板兜底(时间、天气缓存、开关指令等固定问法),体积极小但能力有限。
   - 不做,只在 UI 上把「当前不可用」说清楚,并保留 whisper 转写展示。
3. 若不做,建议把 README 那条 Todo 明确标成 wontfix 并写明理由,避免它一直挂在待办上。

## 相关

- 现状与本规格的关系:`.scratch/offline-tts/spec.md` 只覆盖「合成」
- 无超时/无重试让「离线」表现更像卡住:`issues/06`
- 无会话历史:`issues/04`
