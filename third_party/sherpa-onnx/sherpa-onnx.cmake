# sherpa-onnx 的 Android 集成胶水层。
#
# 照抄 third_party/android_openssl/android_openssl.cmake 的既有模式：一个
# IMPORTED 目标 + 一个把 .so 塞进 APK 的 add_*_libraries() 函数。
#
# 采用 v1.13.7 的**非静态链接 ONNX Runtime** 产物，即两个 .so：
#   jniLibs/<ABI>/libsherpa-onnx-c-api.so   —— 纯 C API，不需要 JNI
#   jniLibs/<ABI>/libonnxruntime.so         —— 其运行时依赖
#
# 为什么不用标称「静态链接 ONNX Runtime」的那份产物（34.3 MB tar.bz2）：
# 该产物里**只有 libsherpa-onnx-jni.so**，没有 C API 库（已逐个 ABI 核实）。
# 用它就必须引入 JNI，而本工程是纯 C++、零 Java、无 gradle 文件。两者相权，
# 保住「不引入 JNI」而放弃「单个 .so」，代价是 24.93 MB 而非 22.55 MB。
# 详见同目录 README.md。
#
# .so 不入版本库（.gitignore 全局排除 *.so），首次构建前请运行 download-libs.sh。

if(NOT TARGET sherpa-onnx-c-api)
    set(SHERPA_ONNX_ROOT ${CMAKE_CURRENT_LIST_DIR})
    set(SHERPA_ONNX_INCLUDE_DIR ${SHERPA_ONNX_ROOT}/include)
    set(SHERPA_ONNX_LIB_DIR ${SHERPA_ONNX_ROOT}/jniLibs/${CMAKE_ANDROID_ARCH_ABI})

    set(_SHERPA_C_API      "${SHERPA_ONNX_LIB_DIR}/libsherpa-onnx-c-api.so")
    set(_SHERPA_ONNXRUNTIME "${SHERPA_ONNX_LIB_DIR}/libonnxruntime.so")

    if(NOT EXISTS "${_SHERPA_C_API}" OR NOT EXISTS "${_SHERPA_ONNXRUNTIME}")
        message(FATAL_ERROR
            "sherpa-onnx 预编译库缺失，期望目录: ${SHERPA_ONNX_LIB_DIR}\n"
            "这两个 .so 不入版本库（.gitignore 排除 *.so），请在构建机上先执行:\n"
            "    bash ${SHERPA_ONNX_ROOT}/download-libs.sh\n"
            "详见 ${SHERPA_ONNX_ROOT}/README.md")
    endif()

    add_library(sherpa-onnx-c-api SHARED IMPORTED)
    set_target_properties(sherpa-onnx-c-api PROPERTIES
        IMPORTED_LOCATION "${_SHERPA_C_API}"
        INTERFACE_INCLUDE_DIRECTORIES "${SHERPA_ONNX_INCLUDE_DIR}"
    )

    # libonnxruntime.so 是 libsherpa-onnx-c-api.so 的运行时依赖。
    # androiddeployqt 不会替 QT_ANDROID_EXTRA_LIBS 解析传递依赖，两个都要显式列出。
    set(SHERPA_ONNX_EXTRA_LIBS_PATHS
        "${_SHERPA_C_API}"
        "${_SHERPA_ONNXRUNTIME}"
        CACHE INTERNAL "Android sherpa-onnx libraries for QT_ANDROID_EXTRA_LIBS"
    )
endif()

function(add_sherpa_onnx_libraries)
    foreach(TARGET_NAME ${ARGN})
        if(TARGET ${TARGET_NAME})
            set_property(TARGET ${TARGET_NAME} APPEND PROPERTY
                QT_ANDROID_EXTRA_LIBS ${SHERPA_ONNX_EXTRA_LIBS_PATHS}
            )
            target_link_libraries(${TARGET_NAME} PUBLIC sherpa-onnx-c-api)
        else()
            message(WARNING "add_sherpa_onnx_libraries(): target '${TARGET_NAME}' does not exist.")
        endif()
    endforeach()
endfunction()
