# 05 — CMake 的 OpenMP 开关与优化选项写在目标创建之后,没有按预期生效

Status: resolved
Type: bug

## 问题

根 `CMakeLists.txt` 原来的顺序:

```cmake
add_subdirectory(third_party/whisper.cpp)
set(GGML_OPENMP OFF CACHE BOOL "..." FORCE)      # ← 子目录已经读过它了
...
# 文件末尾:
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
add_compile_options(-march=armv8.2-a+fp16+dotprod)   # ← 所有目标都已创建
```

- `GGML_OPENMP`:全新构建目录的**首次**配置里,whisper/ggml 读到的是它自己的默认值 ON,
  要到第二次配置才变成 OFF。
- `add_compile_options()` 只作用于之后创建的目标,所以 `-march` 对应用和 whisper **都不生效**。
- `CMAKE_CXX_FLAGS_RELEASE` 的追加只落到应用自身(子目录在 `add_subdirectory` 时已拷走变量),
  而且只设了 CXX,ggml 里的 C 源文件不受影响。

README 写着「release 构建启用了速度优化」,实际上那段「强制启用 FP16 / DotProduct」是空操作。
2026-11-11 通读代码时发现,此前没有工单。

## 可选的处理(所有者 2026-11-11 选了 B)

- A. 只把 OpenMP 移到前面,`-march` 那段删掉。
- **B. 两处都移到前面,让它真正生效。** ← 选定。代价:最低硬件要求抬到 ARMv8.2-A。

## Answer

**2026-11-11 — 按 B 修改并在全新构建目录里验证。** 决策留档:`docs/adr/0004-require-armv8-2.md`。

三项设置都移到 `add_subdirectory(third_party/whisper.cpp)` 之前,C 与 C++ 的 Release 标志都设,
`${ANDROID_ABI}` 的裸展开改成 `ANDROID_ABI STREQUAL`,并在原地写明为什么顺序在这里是语义。

验证(临时目录里的全新 Android Release 配置 + 构建,arm64-v8a):

- 首次配置后 `CMakeCache.txt` 里 `GGML_OPENMP:BOOL=OFF`;
- `compile_commands.json` 的 47 个编译单元**全部**带 `-march=armv8.2-a+fp16+dotprod` 与 `-O3`,
  没有 `-fopenmp`;
- 应用与 whisper/ggml 各动态库编译链接通过,`NEEDED` 里没有 `libomp.so`。

**没有验证的**:真机上的识别速度有没有变化,以及在 ARMv8.0 设备上是否如预期直接崩溃
(手头是否有这样的设备未知)。请在下一次真机测试时留意启动是否正常。
