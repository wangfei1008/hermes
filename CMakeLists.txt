cmake_minimum_required(VERSION 3.16)

project(hermes_project LANGUAGES C CXX)

# ---------------------------------------------------------
# 1.基础编译设置
# ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------
# 2.定义外部路径变量 (将硬编码路径提取至此处，方便一键修改)
# ---------------------------------------------------------
set(BASE_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/common CACHE PATH "Path to base libraries")
if (MSVC)
	set(SDK_DIR "D:/code/sdk" CACHE PATH "Path to external SDKs")
else()
	#set(BASE_LIB_DIR "/cf/base" CACHE PATH "Path to base libraries")
	#set(SDK_DIR "sdk" CACHE PATH "Path to external SDKs")
endif()
add_library(CORE_HEADERS INTERFACE)
target_include_directories(CORE_HEADERS INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/core
)

# ---------------------------------------------------------
# 3.统一输出目录
# ---------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ---------------------------------------------------------
# 4.进入子工程
# ---------------------------------------------------------
# 必须先添加 base，因为 hermes 依赖它
add_subdirectory("${BASE_LIB_DIR}" "${CMAKE_BINARY_DIR}/base_build")
add_subdirectory(componect)
add_subdirectory(hermes)
