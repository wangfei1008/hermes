# 1. 搜集源文件
file(GLOB_RECURSE MQTT_SRC_FILES 
    "*.cpp"
    "*.c"
)

file(GLOB_RECURSE MQTT_HEADER_FILES
    "*.h"
)

# 2. 生成静态库 (也可以设为 SHARED)
add_library(forward_send_mqtt SHARED 
            ${MQTT_SRC_FILES}
            ${MQTT_HEADER_FILES}
)

# 链接 cf_base
target_link_libraries(forward_send_mqtt PRIVATE cf_base)
# 链接 core_headers
target_link_libraries(forward_send_mqtt PUBLIC CORE_HEADERS)

# 平台相关配置
if (WIN32)
    # ---------- include ----------
    target_include_directories(forward_send_mqtt PRIVATE
        "${SDK_DIR}/communicate/paho.mqtt.c-1.3.13/src"
    )

    # ---------- lib path ----------
    target_link_directories(forward_send_mqtt PRIVATE
        $<$<CONFIG:Debug>:${SDK_DIR}/communicate/paho.mqtt.c-1.3.13/build/src/Debug>
        $<$<CONFIG:Release>:${SDK_DIR}/communicate/paho.mqtt.c-1.3.13/build/src/Release>
    )

    # ---------- link ----------
    target_link_libraries(forward_send_mqtt PUBLIC
        paho-mqtt3cs
    )

else()  # Linux
    target_link_libraries(forward_send_mqtt PUBLIC
        paho-mqtt3cs
    )

endif()