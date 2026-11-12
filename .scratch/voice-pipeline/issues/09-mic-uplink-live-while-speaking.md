# 09 — 朗读期间上行音频仍进入识别,可能自问自答

Status: wontfix
Type: bug

## 问题(从代码推断,没有在真机上复现过)

`MainWindow::onDataReceived()` 不看当前是否正在朗读,上行音频一律送进识别。开发板固件在
播放期间也照常上行(`firmware/esp32c3/src/main.cpp` 的 `loop()`)。板子的喇叭和麦克风在
同一块板上,所以 TTS 的声音会被麦克风拾到、被 Whisper 转写(识别语言固定为英文,念的是
中文,结果多半是无意义的英文片段),再作为一次新的提问发给大模型——回复又会被念出来。

2026-11-11 通读代码时发现。

## 可选的处理

- A. 手机侧:朗读期间丢弃上行音频。
- B. 固件侧:`play` 期间暂停上行(或手机在播放前后发 `mic:off` / `mic:auto`)。
- **C. 不改。** ← 所有者 2026-11-11 的决定

## 决定

**不处理。** 决策留档:`docs/adr/0007-mic-stays-live-while-speaking.md`。

要重新打开时把 Status 改回 `needs-triage`。届时 A 只需在 `onDataReceived()` 开头加一个
`m_speaking` 判断;B 的 `mic:off` 指令固件已经支持,控制通道也已建好
(`.scratch/board-esp32-link/issues/04`、`issues/08`)。

真机验收时如果观察到「它在自己跟自己说话」,原因就在这里(`issues/11` 的备注里提了一句)。
