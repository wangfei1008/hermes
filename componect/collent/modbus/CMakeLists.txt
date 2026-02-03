# componect/collent/modbus/CMakeLists.txt

# 1. 搜集源文件
file(GLOB_RECURSE MODBUS_SRC_FILES 
    "*.cpp"
    "*.c"
)

# 2. 生成静态库 (也可以设为 SHARED)
add_library(modbus_lib SHARED ${MODBUS_SRC_FILES})

# 3. 设置头文件包含路径
# PUBLIC 意味着任何链接了 modbus_lib 的目标都会自动获取这些包含路径
#target_include_directories(modbus_lib PUBLIC 
#    ${CMAKE_CURRENT_SOURCE_DIR}
#    "${BASE_LIB_DIR}"
#    "${SDK_DIR}/data process/libcrc/libcrc/include" # 如果modbus需要crc
#)

# 4. 如果 modbus 依赖 base 库
#target_link_libraries(modbus_lib PRIVATE cf_base)