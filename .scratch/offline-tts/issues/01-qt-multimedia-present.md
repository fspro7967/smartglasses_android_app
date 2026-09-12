# 01 — 确认 Qt 6.11.1 的 Android 套件含 Multimedia 模块

Status: resolved
Type: task

spec.md 步骤 0 的第 1 条：

> 确认 Qt 6.11.1 的 Android 套件里有 Multimedia 模块：
> 检查 `/opt/Qt/6.11.1/android_arm64_v8a/lib/cmake/Qt6Multimedia` 是否存在。
> 没有就用 Qt Maintenance Tool 补装。

为什么阻塞：`CMakeLists.txt` 现在 `find_package(Qt6 REQUIRED COMPONENTS ... Multimedia)`，
模块缺失会让**整个 APK 构建在 configure 阶段失败**，而不是只让朗读功能不可用。

实施者在 Windows 开发机上无法验证（该机没有 Qt）。

## Answer

在 Linux 构建机上确认存在，无需补装：

    $ ls -d /opt/Qt/6.11.1/android_arm64_v8a/lib/cmake/Qt6Multimedia*
    /opt/Qt/6.11.1/android_arm64_v8a/lib/cmake/Qt6Multimedia
    /opt/Qt/6.11.1/android_arm64_v8a/lib/cmake/Qt6MultimediaPrivate
    /opt/Qt/6.11.1/android_arm64_v8a/lib/cmake/Qt6MultimediaQuickPrivate
    /opt/Qt/6.11.1/android_arm64_v8a/lib/cmake/Qt6MultimediaTestLibPrivate
    /opt/Qt/6.11.1/android_arm64_v8a/lib/cmake/Qt6MultimediaWidgets
    /opt/Qt/6.11.1/android_arm64_v8a/lib/cmake/Qt6MultimediaWidgetsPrivate

该模块确实被链接进产物：`libQt6Multimedia_arm64-v8a.so`（1,408,416 B）出现在
最终 APK 的 `lib/arm64-v8a/` 下。host 侧 `/opt/Qt/6.11.1/gcc_64/lib/cmake/Qt6Test`
同样存在，因此工单 06 的桌面测试目标可以生成。
