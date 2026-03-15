# 收集 base 目录下所有的源文件
file(GLOB_RECURSE BASE_SRC_FILES
    "*.cpp"
    "*.c"
)

file(GLOB_RECURSE BASE_HEADER_FILES
    "*h"
)

# 定义静态库目标
add_library(cf_base STATIC 
    ${BASE_SRC_FILES} 
    ${BASE_HEADER_FILES}
)

# 导出头文件路径，使链接 cf_base 的目标能自动包含此路径
target_include_directories(cf_base PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# 平台相关配置
if (WIN32)
    # ---------- include ----------
    target_include_directories(cf_base PUBLIC
        "${SDK_DIR}/log4cplus/log4cplus-2.0.8/include"
        "${SDK_DIR}/pthread/3.0.3.1/include"
        "${SDK_DIR}/data process/json/rapidjson/include"
        "${SDK_DIR}/data process/libcrc/libcrc/include"
    )

    # ---------- lib path ----------
    target_link_directories(cf_base PUBLIC
        $<$<CONFIG:Debug>:${SDK_DIR}/log4cplus/log4cplus-2.0.8/msvc14/x64/bin.Debug>
        $<$<CONFIG:Release>:${SDK_DIR}/log4cplus/log4cplus-2.0.8/msvc14/x64/bin.Release>
        
        $<$<CONFIG:Debug>:${SDK_DIR}/pthread/3.0.3.1/lib/x64/Debug>
        $<$<CONFIG:Release>:${SDK_DIR}/pthread/3.0.3.1/lib/x64/Release>

        $<$<CONFIG:Debug>:${SDK_DIR}/data\ process/libcrc/x64/Debug>
        $<$<CONFIG:Release>:${SDK_DIR}/data\ process/libcrc/x64/Release>
    )

    # ---------- link ----------
    target_link_libraries(cf_base PUBLIC
        pthread.lib
        $<$<CONFIG:Debug>:log4cplusD.lib>
        $<$<CONFIG:Release>:log4cplus.lib>
        libcrc.lib
    )

else()  # Linux

    # ---------- include ----------
    target_include_directories(cf_base PUBLIC
        "${SDK_DIR}/log4cplus-2.0.8/include"
        #"${SDK_DIR}/pthread/3.0.3.1/include"
        "${SDK_DIR}/data_process/json/rapidjson/include"
        "${SDK_DIR}/data_process/libcrc/libcrc/include"
    )

    # ---------- lib path ----------
    # Unix 下使用 pkg 目录中的 log4cplus 静态库
    target_link_directories(cf_base PUBLIC
        "${SDK_DIR}/log4cplus-2.0.8/.libs"
    )

    find_package(OpenSSL REQUIRED)

    # Unix 平台：区分 Linux 和 macOS 的系统库依赖
    if (APPLE)
        # macOS 上没有单独的 libdl / librt，相关符号由 libSystem 提供
        target_link_libraries(cf_base PUBLIC
            OpenSSL::SSL
            OpenSSL::Crypto
            pthread
            log4cplus
        )
    else()
        # Linux 下仍然显式链接 dl / rt
        target_link_libraries(cf_base PUBLIC
            OpenSSL::SSL
            OpenSSL::Crypto
            pthread
            dl
            rt
            log4cplus
        )
    endif()

endif()

# 再次确保针对此库开启 /utf-8
if (MSVC)
    target_compile_options(cf_base PRIVATE /utf-8)
endif()

