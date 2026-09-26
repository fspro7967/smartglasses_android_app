# 03 — QML 音频回传开关

Status: resolved
Type: task

对应 `spec.md` 步骤 3。

## 做法

- 服务卡片的状态行下方新增「语音回传眼镜」开关行
  - `enabled: backend.writeTarget !== "" && backend.ttsReady`
    —— 没选可写特征、或 TTS 尚未就绪时不可点，避免开了却什么也不发生
  - 卡片 `Layout.preferredHeight` 加入 `audioOutRowH`，否则新行会被裁掉
- 状态行在回传开启时由「AI 回复输出: X」变为「语音回传: X（本机静音）」
- `Connections { target: backend; function onBleAudioOutChanged() { checked = ... } }`：
  后端会因积压溢出**自行**关闭回传；而用户点击开关会打断 `checked` 的绑定，
  不显式回同步的话界面会停在一个与后端不一致的状态

## Comments

**2026-09-12（构建机）** — 已实施。

- `ninja` 通过，其中包含 qmlcachegen 对 `Main.qml` 的编译 → QML 语法有效。
- `get_errors` 对 `Main.qml` 报的仍只是该文件既有的 unqualified-access 告警
  （全文件 200+ 条，例如 `color: cText`），本次新增代码沿用同样的写法，未新增类别。
- **未做**：真机上开关的可点性、状态行文案换行/省略效果。
