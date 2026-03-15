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

else()  # Linux / macOS
    set(PAHO_SDK_DIR "${SDK_DIR}/communicate/paho.mqtt.c-1.3.13")
    set(PAHO_LIBS_DIR "${PAHO_SDK_DIR}/build/src")
    target_include_directories(forward_send_mqtt PRIVATE "${PAHO_SDK_DIR}/src")
    target_link_directories(forward_send_mqtt PRIVATE "${PAHO_LIBS_DIR}")
    target_link_libraries(forward_send_mqtt PUBLIC paho-mqtt3cs)

    # macOS: 运行时从 out/lib 加载 paho，避免 @rpath 被解析到 log4cplus 等错误路径
    if(APPLE)
        set_target_properties(forward_send_mqtt PROPERTIES
            BUILD_RPATH "@loader_path"
            INSTALL_RPATH "@loader_path"
        )
        add_custom_command(TARGET forward_send_mqtt POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${PAHO_LIBS_DIR}/libpaho-mqtt3cs.1.dylib"
                "$<TARGET_FILE_DIR:forward_send_mqtt>/libpaho-mqtt3cs.1.dylib"
            COMMAND install_name_tool -change
                "@rpath/libpaho-mqtt3cs.1.dylib"
                "@loader_path/libpaho-mqtt3cs.1.dylib"
                "$<TARGET_FILE:forward_send_mqtt>"
            COMMENT "Copy paho-mqtt3cs to output lib and fix plugin rpath"
        )
    endif()
endif()