
# ---------------------------------------------------------
# 1. 业务源文件声明
# ---------------------------------------------------------
# 建议显式列出核心文件，对频繁变动的模块目录使用相对路径 GLOB
file(GLOB_RECURSE HERMES_SRC_FILES
    hermes.cpp
    service.cpp
    data_models/*.cpp
    message/*.cpp
    library/*.cpp
    device/*.cpp
    database/*.cpp
)

file(GLOB_RECURSE HERMES_HEADER_FILES
    service.h
    data_models/*.h
    message/*.h
    library/*.h
    device/*.h
    database/*.h
    ../core/*h
)

# ---------------------------------------------------------
# 可执行程序
# ---------------------------------------------------------
add_executable(hermes
    ${HERMES_SRC_FILES}
    ${HERMES_HEADER_FILES}
)

# ---------------------------------------------------------
# 2. 编译选项与头文件
# ---------------------------------------------------------
# 编译选项
if (MSVC)
    target_compile_options(hermes PRIVATE
        /W3
        /bigobj
        /MP
        /utf-8
        /wd4828
    )
else()
    target_compile_options(hermes PRIVATE
        -Wall 
        -Wextra 
        -Werror 
        -pedantic
    )
endif()


target_include_directories(hermes PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data_models
    ${CMAKE_CURRENT_SOURCE_DIR}/database
    ${CMAKE_CURRENT_SOURCE_DIR}/message    
	"${BASE_LIB_DIR}"
)
# 链接 cf_base
target_link_libraries(hermes PRIVATE cf_base)

# 平台相关配置
if (WIN32)
    # ---------- include ----------
    target_include_directories(hermes PRIVATE
        "${BASE_LIB_DIR}"
        "${SDK_DIR}/log4cplus/log4cplus-2.0.8/include"
        "${SDK_DIR}/pthread/3.0.3.1/include"
        "${SDK_DIR}/data process/json/rapidjson/include"
        "${SDK_DIR}/data process/libcrc/libcrc/include"
    )

    # ---------- lib path ----------
    target_link_directories(hermes PRIVATE
        $<$<CONFIG:Debug>:${SDK_DIR}/log4cplus/log4cplus-2.0.8/msvc14/x64/bin.Debug>
        $<$<CONFIG:Release>:${SDK_DIR}/log4cplus/log4cplus-2.0.8/msvc14/x64/bin.Release>
        
        $<$<CONFIG:Debug>:${SDK_DIR}/pthread/3.0.3.1/lib/x64/Debug>
        $<$<CONFIG:Release>:${SDK_DIR}/pthread/3.0.3.1/lib/x64/Release>

        $<$<CONFIG:Debug>:${SDK_DIR}/data\ process/libcrc/x64/Debug>
        $<$<CONFIG:Release>:${SDK_DIR}/data\ process/libcrc/x64/Release>
    )

    # ---------- link ----------
    target_link_libraries(hermes PRIVATE
        pthread.lib
        $<$<CONFIG:Debug>:log4cplusD.lib>
        $<$<CONFIG:Release>:log4cplus.lib>
        libcrc.lib
    )

else()  # Linux
    find_package(OpenSSL REQUIRED)

    target_link_libraries(hermes PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
        pthread
        dl
        rt
    )

endif()