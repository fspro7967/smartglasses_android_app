# 01 — 确认 Qt 6.11.1 的 Android 套件含 Multimedia 模块

Status: ready-for-human
Type: task

spec.md 步骤 0 的第 1 条：

> 确认 Qt 6.11.1 的 Android 套件里有 Multimedia 模块：
> 检查 `/opt/Qt/6.11.1/android_arm64_v8a/lib/cmake/Qt6Multimedia` 是否存在。
> 没有就用 Qt Maintenance Tool 补装。

为什么阻塞：`CMakeLists.txt` 现在 `find_package(Qt6 REQUIRED COMPONENTS ... Multimedia)`，
模块缺失会让**整个 APK 构建在 configure 阶段失败**，而不是只让朗读功能不可用。

实施者在 Windows 开发机上无法验证（该机没有 Qt）。
