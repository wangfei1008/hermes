# componect/collent/modbus/CMakeLists.txt
# 1. 搜集源文件
file(GLOB_RECURSE MODBUS_SRC_FILES 
    "*.cpp"
    "*.c"
)

file(GLOB_RECURSE MODBUS_HEADER_FILES
    "*.h"
)

# 2. 生成静态库 (也可以设为 SHARED)
add_library(collent_trigger_modbus SHARED 
            ${MODBUS_SRC_FILES}
            ${MODBUS_HEADER_FILES}
)

# 链接 cf_base
target_link_libraries(collent_trigger_modbus PRIVATE cf_base)
# 链接 core_headers
target_link_libraries(collent_trigger_modbus PUBLIC CORE_HEADERS)

# 平台相关配置
if (WIN32)
    # ---------- include ----------
    target_include_directories(collent_trigger_modbus PRIVATE
        "${SDK_DIR}/communicate/libmodbus-3.1.10/src"
    )

    # ---------- lib path ----------
    target_link_directories(collent_trigger_modbus PRIVATE
        $<$<CONFIG:Debug>:${SDK_DIR}/communicate/libmodbus-3.1.10/src/win32/x64/Debug>
        $<$<CONFIG:Release>:${SDK_DIR}/communicate/libmodbus-3.1.10/src/win32/x64/Release>
    )

    # ---------- link ----------
    target_link_libraries(collent_trigger_modbus PUBLIC
        modbus
        ws2_32
    )

else()  # Linux
    target_link_libraries(collent_trigger_modbus PUBLIC
        modbus
    )

endif()