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

## Comments

**2026-09-12（构建机）** — 未做，仍需要真机。

静态侧已核对：`MainWindow::extractModelToFile()` 已从代码中消失（`grep` 只剩注释里
的说明），`WhisperManager::init()` 走 `whisper_init_from_buffer_with_params()`，
调用处传的是 `assets:/models/model.bin`。**没有任何路径会再写 `model.bin` 到
`AppDataLocation`。** 判据要求的「不再出现 + 转写仍工作」仍需设备实测，
尤其是「转写仍工作」这一半——从内存加载是本次唯一有功能风险的改动。
