
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
)
# 链接 cf_base
target_link_libraries(hermes PRIVATE cf_base)
# 链接 core_headers
target_link_libraries(hermes PUBLIC CORE_HEADERS)