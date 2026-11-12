# 纯逻辑模块的单元测试：sentencesplitter、wavpcm、assetextractor。
#
# 这个目标只在**非 Android** 构建中生成（见根 CMakeLists.txt 末尾），它是给构建
# 机的 host Qt（例如 /opt/Qt/6.11.1/gcc_64）跑的桌面程序：不需要 Android 设备、
# 不需要 NDK，因此 APK 构建完全不受影响。
#
# 每个测试只编译被测的那一个 .cpp，不链接整个 app —— app 依赖 whisper、
# sherpa-onnx 与蓝牙，在桌面目标上既没必要也链接不起来。能进这里的模块有一个
# 共同点：只依赖 QtCore，不碰 QObject 信号槽、设备或第三方库。
#
# 运行方式（构建机上）：
#   cmake -B build-tests -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build-tests
#   ctest --test-dir build-tests --output-on-failure
#   或直接跑 ./build-tests/tests/tst_sentencesplitter

find_package(Qt6 REQUIRED COMPONENTS Test)

# add_unit_test(<测试名> <被测源文件名>)
#   测试源文件为 tests/<测试名>.cpp，被测源文件取自 src/。
function(add_unit_test name source)
    add_executable(${name}
        ${name}.cpp
        ${CMAKE_SOURCE_DIR}/src/${source}
    )
    target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/includes)
    target_link_libraries(${name} PRIVATE Qt6::Test Qt6::Core)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_unit_test(tst_sentencesplitter sentencesplitter.cpp)
add_unit_test(tst_wavpcm           wavpcm.cpp)
add_unit_test(tst_assetextractor   assetextractor.cpp)
