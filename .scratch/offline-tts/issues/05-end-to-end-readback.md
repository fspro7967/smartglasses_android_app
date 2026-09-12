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
