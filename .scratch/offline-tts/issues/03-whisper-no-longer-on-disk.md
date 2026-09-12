# 03 — 验证 AppDataLocation 下不再出现 whisper 模型，且转写仍工作

Status: ready-for-human
Type: task

对应 spec.md 步骤 3 的完成判据 [构建机]：

> `AppDataLocation` 下**不再出现** whisper 模型文件，而 `转写` 仍然工作。

改动内容：`WhisperManager::init()` 改为把 `assets:/models/model.bin` 整块读进内存后
交给 `whisper_init_from_buffer_with_params()`，不再写盘。原先的
`MainWindow::extractModelToFile()` 已删除。

注意：这一步会让**旧版本残留**的 `model.bin` 继续留在设备上（新代码不再写它，
但也不会去删它）。所以首次验证时要么先卸载应用，要么接受目录里那份历史遗留文件
并确认它不再被写入、也不影响功能。

`转写` 仍工作 = 对着麦克风说英文能拿到识别文本。
