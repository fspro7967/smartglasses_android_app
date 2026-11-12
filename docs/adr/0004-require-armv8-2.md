# Release 构建要求 ARMv8.2-A(FP16 + DotProduct)

根 `CMakeLists.txt` 里一直写着 `-march=armv8.2-a+fp16+dotprod`,但因为写在所有目标创建之后,
它从来没有生效过。2026-11-11 修正顺序时面临一个选择:删掉它,还是让它真的生效。所有者选择
**让它生效**——Whisper 在手机上的转写速度是整条语音管线的瓶颈,这两组指令对它的矩阵运算有
实打实的加速。

## 考虑过的选项

- **删掉 `-march`,只保留基线 ARMv8.0**:任何 arm64 手机都能跑,转写更慢。
- **启用 ARMv8.2 + FP16 + DotProduct**(选定):更快,但抬高了最低硬件要求。

## 后果

- 只有 ARMv8.0 的芯片(Cortex-A53 / A57 / A72 / A73,例如骁龙 835 及更早、多数 2018 年以前的
  中低端机)会在执行到这些指令时以 **SIGILL 直接崩溃**,没有降级路径,也没有提示。
  开发者自己的手机不受影响,但把 APK 发给别人之前要先确认对方的机型。
- 选项对应用自身的代码和 whisper/ggml 全部生效(全新构建目录里核对过 47 个编译单元)。
  sherpa-onnx 与 ONNX Runtime 是预编译库,不受这里影响。
- 这几行在 `CMakeLists.txt` 里的**位置**是语义的一部分:必须在 `add_subdirectory(whisper.cpp)`
  之前。挪到后面不会报错,只会悄悄失效。

工单:`.scratch/repo-hygiene/issues/05`。真机上的实际加速幅度没有测过。
