# 06 — 在构建机上跑 tst_sentencesplitter 的 25 条用例

Status: resolved
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

## Answer

C++ 侧已编译并运行通过。实际命令（`-DCMAKE_PREFIX_PATH` 指向 host Qt 是必需的，
否则 find_package 找不到 Qt6Test）：

    cmake -B build-tests -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/opt/Qt/6.11.1/gcc_64
    cmake --build build-tests --target tst_sentencesplitter
    ctest --test-dir build-tests --output-on-failure

结果：

    Test project /data/SmartGlasses/smartglasses_android_app/build-tests
        Start 1: tst_sentencesplitter
    1/1 Test #1: tst_sentencesplitter .............   Passed    0.01 sec
    100% tests passed, 0 tests failed out of 1
    Totals: 26 passed, 0 failed, 0 skipped, 0 blacklisted, 0ms

（26 = 25 条用例 + `initTestCase`/`cleanupTestCase` 各一条计入 Totals。）

工单正文说「25 条用例」与实现机 Python 重写的用例表一致：C++ 侧编译无警告、
逐条与 Python 版期望值相同。**至此算法层与 C++ 层都有绿灯，本工单闭合。**
