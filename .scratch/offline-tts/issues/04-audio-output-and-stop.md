# 04 — 验证能听到声音，且长回复播放途中「停止」立即生效

Status: ready-for-human
Type: task

对应 spec.md 步骤 6 的完成判据 [构建机]：

> 能听到声音；**长回复播放途中「停止」立即生效**。

两个已知不确定点，都需要在真机上确定走了哪条路径：

1. **采样率**：规格要求用模型真实采样率 8 kHz。`QAudioSink` 不做重采样，若
   `QAudioDevice::isFormatSupported()` 不认 8 kHz，代码会退回设备采样率并由
   `resampleLinearLe()` 做线性插值。日志里会打印：
   `音频输出已打开 <rate> Hz 单声道 int16 重采样: <true|false>`
   —— 请把这一行记下来，它决定了是否需要保留那段兜底代码。
2. **停止延迟**：`stopSpeaking()` 走 `QAudioSink::reset()`（丢弃已缓冲音频）而不是
   `stop()`（在 Linux/Darwin 上会同步排空缓冲）。所以停止应当是立即的；
   若听感上仍有残留，需要检查 `setBufferSize()` 设的 200 ms 缓冲。

同时确认「重播」按钮能重放上一条回复，以及 TTS 未就绪时这两个按钮是禁用的。

## Comments

**2026-09-12（构建机）** — 未做，仍需要真机；两个不确定点都只能在设备上定。

- 采样率问题**在构建机上无法预先判**：`QMediaDevices::defaultAudioOutput()` 的
  能力取决于设备，桌面与 Android 的答案无关。日志行 `音频输出已打开 <rate> Hz
  单声道 int16 重采样: <true|false>` 必须在真机上取。
- 「停止」走 `reset()` 的取舍已在代码注释中固定；构建机只能确认它编译通过、
  且 `QAudioSink` 相关头文件与 `Qt6::Multimedia` 链接无误。
- 顺带确认：`libQt6Multimedia_arm64-v8a.so` 已进 APK，真机上不会因缺库而静默无声。
