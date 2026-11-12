# 08 — 固件的 volume / status / echo 指令手机侧没有使用

Status: needs-triage
Type: task

从 `issues/01` 的问题 4 拆出来:那张工单的主问题(适配改动要不要保留)已经有了结论,
这一条还没人回答,单独留着免得跟着一起关掉。

## 现状

固件控制特征支持的指令(`firmware/esp32c3/src/main.cpp` 的 `ControlCallbacks::onWrite`):
`play` / `stop` / `rate:N` / `volume:0-100` / `status` / `echo` / `mic:auto|force|off`。

手机侧现在用到的只有 `stop`、`rate:N`、`play`,以及 `play` 的回执 `OK:playing`
(`MainWindow::requestBoardPlay()` / `onBoardControlReply()`)。其余都没用:

- **音量**:界面没有音量控制。回传时本机是静音的,板子的音量完全由固件默认值
  (`VOLUME_DEFAULT` 180/255)决定。
- **status**:回执里有 `buffer:`、`drop:`、`mic:present|absent`、`micmode:`。手机侧不查询,
  所以「麦克风到底有没有被识别到」「播放缓冲有没有在丢样本」只能看串口。
- **echo**:本可以在自动配置之后做一次「写得通、回执收得到」的自检。
- **mic:off**:可以在朗读期间关掉上行。是否需要,取决于
  `docs/adr/0007-mic-stays-live-while-speaking.md` 那个决定将来是否重新打开。

控制通道(写指令 + 收回执)已经在 `issues/04` 里建好,技术上加这些只是多几条指令。

## 需要所有者回答的

1. 要不要音量控制?要的话是界面滑块,还是跟随手机媒体音量?
2. 要不要在连接后查一次 `status`,把 `mic:absent` 这类情况显示在界面上?
3. 要不要用 `echo` 做连接自检(失败时直接提示「控制通道不通」,而不是等到第一次播放才发现)?

## 相关

- 控制通道实现:`.scratch/board-esp32-link/issues/04`
- 原始出处:`issues/01` 的「问题」第 4 条
