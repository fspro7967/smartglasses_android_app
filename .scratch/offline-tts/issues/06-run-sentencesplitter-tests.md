# 06 — 在构建机上跑 tst_sentencesplitter 的 25 条用例

Status: ready-for-human
Type: task

这是 spec.md 步骤 5 的 C++ 侧绿灯。实施机没有 Qt / CMake / C++ 编译器，因此
只完成了算法层的独立验证（把 `src/sentencesplitter.cpp` 的算法用 Python 重写一遍，
跑与 `tests/tst_sentencesplitter.cpp` 逐条相同的用例表，25 条全过）。
**那验证的是算法，不是 C++ 代码**，C++ 侧的编译与运行仍需在此工单完成。

    cmake -B build-tests -DCMAKE_BUILD_TYPE=Debug
    cmake --build build-tests
    ctest --test-dir build-tests --output-on-failure

测试目标只在**非 Android** 构建中生成（见根 `CMakeLists.txt` 末尾），所以它用的是
构建机上的 host Qt（例如 `/opt/Qt/6.11.1/gcc_64`），不需要 Android 设备或 NDK。
它只编译被测的那一个 .cpp，不链接整个 app（app 依赖 POSIX 头）。

若 host Qt 未安装 Qt6Test，configure 会明确报错；此时可加 `-DBUILD_TESTING=OFF`
跳过，但那样这条判据就没闭合。
