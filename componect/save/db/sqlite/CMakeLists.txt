# sqlite 保存组件工程：save_db_sqlite

file(GLOB_RECURSE SQLITE_SRC_FILES
    "*.cpp"
    "*.c"
)

file(GLOB_RECURSE SQLITE_HEADER_FILES
    "*.h"
)

add_library(save_db_sqlite SHARED
    ${SQLITE_SRC_FILES}
    ${SQLITE_HEADER_FILES}
)

# 组件复用基类能力
target_link_libraries(save_db_sqlite PRIVATE cf_base)
target_link_libraries(save_db_sqlite PUBLIC CORE_HEADERS)

# 平台相关：macOS 设置运行时搜索路径
if(APPLE)
    set_target_properties(save_db_sqlite PROPERTIES
        BUILD_RPATH "@loader_path"
        INSTALL_RPATH "@loader_path"
    )
endif()

