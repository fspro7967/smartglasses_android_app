# 02 — MainWindow 把合成音频接到 BLE 下行

Status: resolved
Type: task

对应 `spec.md` 步骤 2。

## 做法

- 新属性/接口：`bleAudioOut`（Q_PROPERTY）、`setBleAudioOut(bool)`、`bleAudioOutChanged`
- `onAudioChunk()`：在**重采样之前**镜像原始 PCM 到蓝牙。
  本机那段 `resampleLinearLe()` 是为 `QAudioSink` 的设备兼容服务的，
  回传音频由眼镜播放，不该跟着手机扬声器的采样率走
- `setBleAudioOut()` / `applyAudioVolume()`：回传时 `QAudioSink::setVolume(0)`
  —— 只静音，**不**停播（停播会让 `m_pendingPcm` 排不空，被判定为「音频输出无响应」）
- `setupAudioOutput()` 重建 sink 后调一次 `applyAudioVolume()`：
  TTS 初始化可能在回传已开启之后才完成
- `clearWriteTarget()` 里顺带关掉回传：输出目标没了，回传没有去处，
  不能让开关停在「已开启」而实际发不出去
- `stopSpeaking()` 里 `clearStreamQueue()`：「停止」之后眼镜不该再播完已排队的几秒
- `onAIResponse()`：回传开启时不回写文本。两者共用同一个写入目标，裸字节无法区分，
  混在一起会让眼镜把文本当 PCM 播出来
- 构造函数里接 `streamAborted` → 关回传 + 状态栏说明「已停止蓝牙音频回传: <原因>」

## Comments

**2026-09-12（构建机）** — 已实施。

- `-fsyntax-only`：`mainwindow.cpp` exit 0；`ninja` 全量通过（moc 重新生成，链接通过）。
- **未经真机验证**：是否有回声残留（音量 0 的实际效果）、
  眼镜侧能否正确播放 8 kHz，见工单 04。
