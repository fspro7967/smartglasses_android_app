# 07 — 记录真实的 APK 体积增量

Status: resolved
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

## Answer

两个数字都拿到了。基线没有用 `git stash` 折腾源码，而是直接量**同一构建目录里
改动前留下的那个 APK**（`builds/Qt-6.11.1-android_arm64_v8a-arm64/Release/`，
时间戳 2026-09-12 19:07，即合并 TTS 之前最后一次构建的产物），因此两次构建的
工具链、ABI、签名方式、构建类型完全一致，可比性比重新切标签构建更好。

| 项 | 字节 | MiB |
|---|---:|---:|
| 改动前（`master`，无 TTS） | 86,019,540 | 82.03 |
| 改动后（含 TTS） | 132,887,802 | 126.73 |
| **增量** | **+46,868,262** | **+44.70（+54.5%）** |

实测增量（+44.70 MiB）**小于**工单里按未压缩体积估的 57.38 MB，比例约 0.78——
与预期方向一致（APK 对 `lib/` 与 `assets/` 都有压缩）。已补进 `spec.md`「六、实测数据」。

APK 内相关条目（`unzip -l`）：

    lib/arm64-v8a/libQt6Multimedia_arm64-v8a.so      1,408,416
    lib/arm64-v8a/libonnxruntime.so                21,684,872
    lib/arm64-v8a/libsherpa-onnx-c-api.so           4,456,784
    lib/arm64-v8a/libsmartglasses_android_app_arm64-v8a.so   559,064
    assets/models/tts-vits-icefall-zh-aishell3/model.onnx   30,482,262
    assets/models/tts-vits-icefall-zh-aishell3/lexicon.txt   2,042,943
    （另有 phone.fst / date.fst / number.fst / new_heteronym.fst / tokens.txt / speakers.txt）
