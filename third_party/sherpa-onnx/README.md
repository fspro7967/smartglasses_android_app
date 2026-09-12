# third_party/sherpa-onnx

离线中文语音合成引擎（sherpa-onnx）的 Android 集成件。**预编译 `.so` 不入版本库**，
头文件与集成胶水入库。

| 项 | 值 |
|---|---|
| 引擎 | sherpa-onnx **v1.13.7**（2026-09-01 发布） |
| 许可证 | Apache-2.0 |
| API | 纯 C API（`sherpa-onnx/c-api/c-api.h`），**不使用 JNI** |
| ABI | `arm64-v8a`（与 `settings.json` 的 `ANDROID_ABI` 一致） |

## 目录内容

```
include/sherpa-onnx/c-api/c-api.h   # 入库；取自源码仓库 tag v1.13.7
sherpa-onnx.cmake                   # 入库；IMPORTED 目标 + QT_ANDROID_EXTRA_LIBS
download-libs.sh                    # 入库；在构建机上拉取下面两个 .so
jniLibs/arm64-v8a/*.so              # 不入库，由脚本生成
```

## 为什么是两个 `.so` 而不是一个

规格原本选择「静态链接 ONNX Runtime 的构建产物（单个 `.so`）」。**该形态在 v1.13.7
的已发布产物中不存在**，已对 289 个 release 资产逐个核实：

| 产物 | 大小 | arm64-v8a 里的内容 | 能否用纯 C API |
|---|---:|---|---|
| `sherpa-onnx-v1.13.7-android-static-link-onnxruntime.tar.bz2` | 34.3 MB | 仅 `libsherpa-onnx-jni.so` (22.55 MB) | **不能**，只有 JNI |
| `sherpa-onnx-static-link-onnxruntime-1.13.7.aar` | 36.1 MB | 仅 `libsherpa-onnx-jni.so` | **不能** |
| `sherpa-onnx-v1.13.7-android.tar.bz2` | 45.2 MB | `libsherpa-onnx-c-api.so` + `libonnxruntime.so` | **能** ← 本项目采用 |

即「静态链接 ONNX Runtime」与「纯 C API、不引入 JNI」在已发布产物里互斥。
本项目是纯 C++、零 Java、无 gradle 文件，引入 JNI 需要额外加 Java 层与 Gradle
配置；而放弃静态链接只多 2.38 MB。因此保住前者。

若将来确实需要单个 `.so`，须在构建机上从源码构建
（`-DSHERPA_ONNX_ENABLE_C_API=ON -DBUILD_SHARED_LIBS=ON` 并静态链接 ONNX Runtime）。

## 拉取预编译库

```bash
bash third_party/sherpa-onnx/download-libs.sh
```

脚本会下载并**校验 sha256**，只取 `arm64-v8a` 的两个 `.so`：

| 文件 | 字节数 | sha256 |
|---|---:|---|
| `libsherpa-onnx-c-api.so` | 4,456,784 | `73dba26ddf63e47e6c6e8b7c663d50c834b267ec0bb1ada1bc8e472e390bc41a` |
| `libonnxruntime.so` | 21,684,880 | `dc5e4c172b1be9e530c6a62ad8f1be3e0a911cabdee6195abf28dab72477e194` |

合计 26,141,664 B（24.93 MB）。

上传归档 `sherpa-onnx-v1.13.7-android.tar.bz2`：
45,287,000 B，sha256 `7208b26f5109777d1c3c22a45f04665419d6f3ea81f6811a2dfd5ff2d7622e6e`

头文件 `c-api.h` 取自源码仓库 tag `v1.13.7`（167,345 B，
sha256 `426db2c6acfb51e02143aece67c45779fae699d961c7c26ccf6f1388fdeaa2df`）。
它只 `#include <stdint.h>`，自包含。

## 配套的 TTS 模型

模型在 `android/assets/models/tts-vits-icefall-zh-aishell3/`，**随仓库提交**
（与既有 whisper `model.bin` 的做法一致）。来源：

`sherpa-onnx` release tag `tts-models` 的 `vits-icefall-zh-aishell3.tar.bz2`
（31,559,701 B，sha256 `ab468db3a3308cdd861495e0db2f25d79418a0c00639f74944c7cdf5dd8c6ec1`）。

**归档里有一个 172.3 MB 的 `rule.far` 被刻意剔除**：归档解压后共 203.59 MB，
但官方文档对该模型只传
`--tts-rule-fsts=phone.fst,date.fst,number.fst`，**从不引用 `rule.far`**。
剔除后模型为 31.24 MB（`model.onnx` 29.07 MB + `lexicon.txt` 1.95 MB + 其余）。

`C API` 的 `SherpaOnnxOfflineTtsConfig` 同时提供 `rule_fsts` 与 `rule_fars` 两个
独立字段，本项目只用 `rule_fsts`。
