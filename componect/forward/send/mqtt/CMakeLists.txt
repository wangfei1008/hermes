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