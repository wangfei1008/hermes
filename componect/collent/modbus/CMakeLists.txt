# componect/collent/modbus/CMakeLists.txt
# 1. 搜集源文件
file(GLOB_RECURSE MODBUS_SRC_FILES 
    "*.cpp"
    "*.c"
)

file(GLOB_RECURSE MODBUS_HEADER_FILES
    "*.h"
)

# 2. 生成动态库
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

else()  # Linux / macOS

  # ---------- include ----------
  target_include_directories(collent_trigger_modbus PRIVATE
       "${SDK_DIR}/communicate/libmodbus-3.1.10/src"
    )

    # ---------- lib path ----------
    # 使用固定路径，避免在单配置生成器（Unix Makefiles）下 CONFIG 判定为空导致 -L 丢失
    set(MODBUS_SDK_LIBS "${SDK_DIR}/communicate/libmodbus-3.1.10/src/.libs")
    target_link_directories(collent_trigger_modbus PRIVATE "${MODBUS_SDK_LIBS}")

    # 在 Unix 系统上，库文件名通常是 libmodbus.so / libmodbus.dylib，
    # 这里链接名应写成 "modbus"（CMake 会自动加前缀 lib 和后缀）
    target_link_libraries(collent_trigger_modbus PUBLIC
        modbus
    )

    # ---------- macOS: 运行时从 out/lib 加载 libmodbus，不依赖 /usr/local/lib ----------
    if(APPLE)
      set_target_properties(collent_trigger_modbus PROPERTIES
          BUILD_RPATH "@loader_path"
          INSTALL_RPATH "@loader_path"
      )
      # 构建后：拷贝 SDK 的 libmodbus 到输出 lib 目录，并修正插件的依赖路径
      add_custom_command(TARGET collent_trigger_modbus POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "${MODBUS_SDK_LIBS}/libmodbus.5.dylib"
              "$<TARGET_FILE_DIR:collent_trigger_modbus>/libmodbus.5.dylib"
          COMMAND install_name_tool -change
              "/usr/local/lib/libmodbus.5.dylib"
              "@loader_path/libmodbus.5.dylib"
              "$<TARGET_FILE:collent_trigger_modbus>"
          COMMENT "Copy libmodbus to output lib and fix plugin rpath"
      )
    endif()

endif()