# 05 — 端到端：说一句英文，得到中文回复并被朗读出来

Status: ready-for-human
Type: task

对应 spec.md 步骤 8 的完成判据 [构建机]：

> APK 构建通过；对着麦克风说一句英文，得到中文回复**并被朗读出来**。

这是整条流水线的验收：蓝牙音频 → whisper（`params.language = "en"`）→ 英文转写
→ `MsgSender`（中文 system prompt）→ 中文回复 → `splitSentences()` 逐句切分
→ sherpa-onnx 合成 → `QAudioSink` 手机扬声器。

构建前的准备（这两个 .so 不入版本库）：

    bash third_party/sherpa-onnx/download-libs.sh

脚本会下载并校验 sha256；缺失时 `third_party/sherpa-onnx/sherpa-onnx.cmake`
会给出明确的 FATAL_ERROR 而不是晦涩的链接错误。

另外需要确认的三条**静默路径**都不出声：回复文本为空、`[无识别结果]`、`应答`失败。

## Comments

**2026-09-12（构建机）** — 判据的前半段（「APK 构建通过」）已闭合，后半段（真机朗读）未做。

- 依赖已就绪：`download-libs.sh` 跑通，`libsherpa-onnx-c-api.so`（4,456,784 B）与
  `libonnxruntime.so`（21,684,880 B）sha256 均与脚本内记录一致。
- `apk` 目标构建成功（Release，arm64-v8a），含签名步骤；构建时用
  `-DAUTO_INSTALL_APK=OFF` 关闭了自动安装，因此**没有**执行 `adb install`。
- 编译期无错误：仅 Qt 的 `QTP0002` 策略警告与 NDK 工具链的 `cmake_minimum_required`
  弃用警告，均与本改动无关。
- 产物核对：`libQt6Multimedia_arm64-v8a.so`、两个 sherpa .so、以及 8 个 TTS 模型
  文件都已在 APK 内（见工单 07 的 Answer）。

**仍待真机**：对麦克风说一句英文 → 中文回复 → 被朗读；以及三条静默路径逐条试听。
