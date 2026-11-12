# 09 — 模型与第三方源码直接入库,仓库体积大

Status: wontfix
Type: task

## 现状

| 内容 | 大小 / 数量 |
|---|---|
| `android/assets/models/model.bin`(whisper 模型) | 59.7 MB |
| `android/assets/models/tts-vits-icefall-zh-aishell3/model.onnx` | 29 MB |
| `third_party/whisper.cpp/`(整份源码) | 1179 个文件 |
| `third_party/android_openssl/`(头文件等) | 867 个文件 |

没有使用 Git LFS,`.git` 约 120 MB。2026-11-11 通读代码时发现。

## 当时给出的选项

- **A. 保持现状。** ← 所有者 2026-11-11 的决定
- B. 模型改用 Git LFS。
- C. 模型改用下载脚本。

## 决定

**保持现状。** 决策留档:`docs/adr/0005-models-and-whisper-vendored-in-repo.md`。

与之相对的是两组**没有**入库、需要另外获取的二进制:sherpa-onnx 的 `.so`(有
`third_party/sherpa-onnx/download-libs.sh`)和 OpenSSL 的 `.so`(什么都没有,见 `issues/02`)。
