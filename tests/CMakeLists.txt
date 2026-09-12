# sentencesplitter 的单元测试。
#
# 这个目标只在**非 Android** 构建中生成（见根 CMakeLists.txt 末尾），它是给构建
# 机的 host Qt（例如 /opt/Qt/6.11.1/gcc_64）跑的桌面程序：不需要 Android 设备、
# 不需要 NDK，因此 APK 构建完全不受影响。
#
# 这是本仓库第一个「不需要真实设备就能跑」的测试。它只编译被测的那一个 .cpp，
# 不链接整个 app —— app 依赖 POSIX 头（whisper_manager.h 引入 <sys/resource.h>），
# 在桌面目标上链接会失败。
#
# 运行方式（构建机上）：
#   cmake -B build-tests -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build-tests
#   ctest --test-dir build-tests --output-on-failure
#   或直接跑 ./build-tests/tests/tst_sentencesplitter

find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(tst_sentencesplitter
    tst_sentencesplitter.cpp
    ${CMAKE_SOURCE_DIR}/src/sentencesplitter.cpp
)

target_include_directories(tst_sentencesplitter PRIVATE
    ${CMAKE_SOURCE_DIR}/includes
)

target_link_libraries(tst_sentencesplitter PRIVATE
    Qt6::Test
    Qt6::Core
)

add_test(NAME tst_sentencesplitter COMMAND tst_sentencesplitter)
