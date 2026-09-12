# 07 — 记录真实的 APK 体积增量

Status: ready-for-human
Type: task

对应 spec.md 步骤 1 的完成判据 [静态] 中尚未闭合的一项：

> **记录 APK 体积增量(改动前/后各一个数字)**

实施机产出不了 APK，只能给出可计算的等价数字（未压缩）：

| 项 | 体积 |
|---|---:|
| `libsherpa-onnx-c-api.so` | 4.46 MB |
| `libonnxruntime.so` | 21.68 MB |
| TTS 模型（assets，已剔除 `rule.far`） | 31.24 MB |
| **合计** | **57.38 MB** |

真实 APK 增量会明显小于这个数：`assets/` 与 `lib/` 在 APK 内的压缩率不同，
而 `rule.far`（172.3 MB，高度可压缩）被剔除的收益也体现在这里。
请用 `git stash` / 切换 `snapshot/pre-tts-implementation` 标签各构建一次来量，
并把两个数字补进 spec.md 第六节。
