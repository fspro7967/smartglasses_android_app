# 03 — 只有 1 个可离线运行的测试,没有 CI,CMake 用 file(GLOB)

Status: needs-triage
Type: task

## 现状

- 唯一测试:`tests/tst_sentencesplitter.cpp`,25 条用例,跑 `splitSentences()`
  (已由 `.scratch/offline-tts/issues/06` 在构建机上验证通过:26 passed / 0 failed)。
- 没有 CI:`ls -d .github .gitlab-ci.yml .circleci` 都不存在。
- 根 `CMakeLists.txt:44-45`:
  ```cmake
  file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
  file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/includes/*.h)
  ```
  新增源文件后如果 CMake 不重新 configure(`cmake -B`),它不会进构建,
  表现为「明明写了代码却像没生效」。

## 为什么测试这么少(不是懒,是结构上测不了)

`tests/CMakeLists.txt` 的注释写明了原因:整个 app 的桌面目标链接不起来,
因为 `includes/whisper_manager.h:7` 直接 `#include <sys/resource.h>`(POSIX),
而且还要链 whisper/sherpa/Bluetooth/Multimedia。于是目前只能「只编译被测的那一个 .cpp」。

也就是说,值得测的逻辑里有相当一部分**因为放错了地方而无法测试**:

| 想测的东西 | 现在在哪 | 为什么测不了 |
|---|---|---|
| 分包大小 / 发包节拍(`writeChunkSize`、`streamTickIntervalMs`) | `DeviceHandler` 私有成员函数 | 依赖 `QLowEnergyController` |
| 8k↔16k 重采样(`resampleLinearLe`) | `src/mainwindow.cpp` 的匿名命名空间 | 需要整个 MainWindow |
| 提取清单校验(`isExtracted` / `writeManifest`) | `AssetExtractor` 自由函数 | **其实可以测**,只是没写 |
| 逐句切分 | `sentencesplitter` | 已测 ✅ |

## 建议做法

1. `AssetExtractor` 的两个自由函数立刻可测,先补 `tst_assetextractor`
   (临时目录 + 伪造清单:少一行、行名不符、字节数不符、`.tmp` 残留四个用例)。
2. 把 `resampleLinearLe()` 从 `mainwindow.cpp` 挪到 `includes/`+`src/` 的独立
   自由函数(纯计算,无 Qt GUI 依赖),再补边界用例(空、单样本、等比、非整比)。
3. `writeChunkSize()` / `streamTickIntervalMs()` 抽成不依赖 controller 的纯函数
   (`mtu`、`bytesPerSecond` 作入参),`DeviceHandler` 只做取值转发。
   这样「MTU 23 → 20 B」「16 kHz → 节拍」这类算术就能被测试锁住 ——
   它们正是 `.scratch/board-esp32-link/issues/02` 里算错就出事的参数。
4. `file(GLOB)` 换成显式文件列表(`set(SRC_FILES ...)`)。
5. 加一个最小 CI:host Qt 上 configure + build + `ctest`,再加一个 Android
   `assemble`/`apk` 目标(可以是 nightly 或仅在相关路径变动时触发)。

## 完成判据

- [ ] 至少新增 `tst_assetextractor`,并在 CI 或构建机文档里给出运行命令
- [ ] `resampleLinearLe` 与分包/节拍算术有单元测试
- [ ] CI 在 PR 上跑测试(或至少有 workflow 文件并被实际触发过)

## 相关

- 既有测试与运行方式:`tests/CMakeLists.txt`、`.scratch/offline-tts/issues/06`
- 被算错的参数:`src/devicehandler.cpp:342-358`
