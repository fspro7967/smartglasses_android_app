cmake_minimum_required(VERSION 3.16)

project(smartglasses_android_app VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Android 目标 Qt 依赖 host 端工具包（Qt6CoreTools 等），
# 未由 IDE/外部注入 QT_HOST_PATH 时自动指向本机 host Qt。
if(ANDROID AND NOT QT_HOST_PATH AND EXISTS "/opt/Qt/6.11.1/gcc_64")
    set(QT_HOST_PATH "/opt/Qt/6.11.1/gcc_64" CACHE PATH "Path to host Qt installation")
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Quick Qml)

# Multimedia 用于 QAudioSink：把 TTS 合成出的 PCM 推到手机扬声器。
find_package(Qt6 REQUIRED COMPONENTS Quick Qml Bluetooth Concurrent Network Multimedia)

# QTP0001 NEW：QML 模块资源前缀使用 /qt/qml（默认 QML 导入路径）。
# OLD 行为（资源放在 :/SmartGlasses/）在 Qt 6.11 上找不到 qmldir，
# 会导致 loadFromModule 报 "Module contains no type named Main"。
qt_policy(SET QTP0001 NEW)
find_package(Qt6LinguistTools QUIET)
find_package(Qt5LinguistTools QUIET)

set(TS_FILES smartglasses_android_app_zh_CN.ts)

set(PROJECT_SOURCES
        ${TS_FILES}
)

include_directories(./includes/)

# 添加 whisper.cpp 子目录
add_subdirectory(third_party/whisper.cpp)
# 禁用 OpenMP，避免依赖 libomp.so
set(GGML_OPENMP OFF CACHE BOOL "Disable OpenMP for Android" FORCE)
set(GGML_OPENMP OFF)

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/includes/*.h)
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(smartglasses_android_app
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${SRC_FILES}
        ${HEADER_FILES}
    )

    # 文件位于 qml/ 子目录，映射到模块根，使 loadFromModule("SmartGlasses", "Main") 可加载
    # 注意：QT_RESOURCE_ALIAS 必须在 qt6_add_qml_module 之前设置，
    # 否则宏在调用时读不到该属性，类型会被注册为 SmartGlasses.qml.Main 而非 SmartGlasses.Main
    set_source_files_properties(qml/Main.qml PROPERTIES QT_RESOURCE_ALIAS "Main.qml")

    # 注册 QML 模块（qml/Main.qml 作为入口界面）
    qt_add_qml_module(smartglasses_android_app
        URI SmartGlasses
        VERSION 1.0
        QML_FILES
            qml/Main.qml
    )

# Define target properties for Android with Qt 6 as:
    set_property(TARGET smartglasses_android_app APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    if(Qt6LinguistTools_FOUND)
        qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
    endif()
else()
    if(ANDROID)
        add_library(smartglasses_android_app SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(smartglasses_android_app
            ${PROJECT_SOURCES}
        )
    endif()

    if(Qt5LinguistTools_FOUND)
        qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
    endif()
endif()

target_link_libraries(smartglasses_android_app PRIVATE
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Bluetooth
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Multimedia    # QAudioSink：TTS 播放
    whisper                              # 链接 whisper 库
)

target_include_directories(smartglasses_android_app PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/whisper.cpp/include   # 头文件路径
)

# Qt for Android 不依赖系统 TLS，必须将 OpenSSL 库随 APK 一起打包，
# 否则 QNetworkAccessManager 请求 https 时会报 TLS 初始化错误。
if (ANDROID)
    include(${CMAKE_CURRENT_SOURCE_DIR}/third_party/android_openssl/android_openssl.cmake)
    add_android_openssl_libraries(smartglasses_android_app)

    # 离线中文 TTS：sherpa-onnx 的纯 C API 库 + 其 ONNX Runtime 依赖。
    # 这两个 .so 不入版本库，构建前需先跑 third_party/sherpa-onnx/download-libs.sh，
    # 缺失时 sherpa-onnx.cmake 会给出明确的 FATAL_ERROR。
    include(${CMAKE_CURRENT_SOURCE_DIR}/third_party/sherpa-onnx/sherpa-onnx.cmake)
    add_sherpa_onnx_libraries(smartglasses_android_app)
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.smartglasses_android_app)
endif()
set_target_properties(smartglasses_android_app PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS smartglasses_android_app
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(smartglasses_android_app)
endif()

# 可选：APK 构建完成后自动安装到已连接的 Android 设备
option(AUTO_INSTALL_APK "Automatically install the built APK on a connected device after the apk target completes" ON)

# Qt 6.11 的 CMake 部署目标会生成 unsigned APK；QT_ANDROID_SIGN_APK
# 不是该版本的有效 target property，因此在 APK 目标完成后显式调用 apksigner。
if (ANDROID AND QT_VERSION_MAJOR EQUAL 6)
    # androiddeployqt 产出的最终 APK（Debug 为 debug 签名，Release 在下方签名后覆盖回此路径）
    set(APK_PATH "${CMAKE_BINARY_DIR}/android-build/smartglasses_android_app.apk")

    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        set(SIGNED_APK "${APK_PATH}.signed")
        find_program(APKSIGNER_EXECUTABLE
            NAMES apksigner
            PATHS "${ANDROID_SDK_ROOT}/build-tools/37.0.0"
            NO_DEFAULT_PATH
        )
        if (NOT APKSIGNER_EXECUTABLE)
            message(FATAL_ERROR "Cannot find apksigner under ${ANDROID_SDK_ROOT}/build-tools/37.0.0")
        endif()
        if (NOT EXISTS "${KEYSTORE}")
            message(FATAL_ERROR "Android release keystore does not exist: ${KEYSTORE}")
        endif()

        add_custom_command(
            OUTPUT "${SIGNED_APK}"
            DEPENDS "${APK_PATH}"
            COMMAND "${APKSIGNER_EXECUTABLE}" sign
                --ks "${KEYSTORE}"
                --ks-pass "pass:${STORE_PASS}"
                --key-pass "pass:${KEY_PASS}"
                --ks-key-alias "${ALIAS}"
                --out "${SIGNED_APK}" "${APK_PATH}"
            COMMAND "${CMAKE_COMMAND}" -E copy "${SIGNED_APK}" "${APK_PATH}"
            COMMAND "${CMAKE_COMMAND}" -E rm -f "${SIGNED_APK}"
            COMMENT "Signing Android APK with release keystore: ${APK_PATH}"
            VERBATIM
        )
        add_custom_target(smartglasses_android_app_sign_apk DEPENDS "${SIGNED_APK}")
        add_dependencies(apk smartglasses_android_app_sign_apk)
    endif()

    # 统一在 apk 目标（Debug 打包 / Release 打包+签名）全部完成后再安装。
    # 注意不要挂到可执行目标 smartglasses_android_app 的 POST_BUILD 上——
    # 那会在 APK 打包完成之前就执行，导致安装的文件不存在或版本过期。
    if (AUTO_INSTALL_APK)
        # CMake 4.x 已移除 `cmake -E adb` 子命令，需直接调用 adb 可执行文件
        find_program(ADB_EXECUTABLE NAMES adb)
        if (NOT ADB_EXECUTABLE)
            message(WARNING "AUTO_INSTALL_APK is enabled but adb was not found in PATH; skipping auto-install")
        else()
            add_custom_command(
                TARGET apk POST_BUILD
                COMMAND "${ADB_EXECUTABLE}" install -r "${APK_PATH}"
                COMMENT "Installing APK on connected device"
            )
        endif()
    endif()
endif()

if (ANDROID)
    # 开启 Release 高度优化
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

    # 强制为 ARM64 启用 FP16 和 DotProduct 指令集支持
    if (${ANDROID_ABI} STREQUAL "arm64-v8a")
        add_compile_options(-march=armv8.2-a+fp16+dotprod)
        add_definitions(-DGGML_USE_NEON)
    endif()
endif()

# ----------------------------------------------------------------------------
# 单元测试
# ----------------------------------------------------------------------------
# 只在非 Android 构建中生成：测试目标是给构建机的 host Qt 跑的桌面程序，
# 不需要 Android 设备或 NDK，因此 APK 构建完全不受影响。
# 本仓库当前唯一可离线验证的逻辑是 sentencesplitter（纯自由函数，
# 不依赖 QObject / 信号槽 / 设备）——详见 tests/CMakeLists.txt。
include(CTest)
if (BUILD_TESTING AND NOT ANDROID)
    add_subdirectory(tests)
endif()
