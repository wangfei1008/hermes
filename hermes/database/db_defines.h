#ifndef DATABASE_DEFINES_H
#define DATABASE_DEFINES_H

#include "device_define.h"

// ==================== 表名宏定义 ====================
#define TABLE_DEVICE                "device"
#define TABLE_PROTOCOL_CONFIG       "protocol_config"
#define TABLE_DATA_POINT            "data_point"
#define TABLE_DATA_POINT_RECORD     "data_point_record"
#define TABLE_DEVICE_STREAMS        "device_streams"
#define TABLE_STREAM_COMPONENTS     "stream_components"

// ==================== device 表字段 ====================
#define FIELD_DEVICE_ID           JSON_KEY_ID
#define FIELD_DEVICE_NAME         JSON_KEY_NAME
#define FIELD_DEVICE_PROTOCOL     JSON_KEY_PROTOCOL
#define FIELD_DEVICE_CREATED_AT   JSON_KEY_CREATED_AT
#define FIELD_DEVICE_UPDATED_AT   JSON_KEY_UPDATED_AT

// ==================== protocol_config 表字段 ====================
#define FIELD_CONFIG_ID            JSON_KEY_ID
#define FIELD_CONFIG_DEVICE_ID     JSON_KEY_DEVICE_ID
#define FIELD_CONFIG_HOST          JSON_KEY_PROTOCOL_CFG_HOST
#define FIELD_CONFIG_PORT          JSON_KEY_PROTOCOL_CFG_PORT
#define FIELD_CONFIG_STATION       JSON_KEY_PROTOCOL_CFG_STATION
#define FIELD_CONFIG_BAUD          JSON_KEY_PROTOCOL_CFG_BAUD
#define FIELD_CONFIG_DATA          JSON_KEY_PROTOCOL_CFG_DATA
#define FIELD_CONFIG_STOP          JSON_KEY_PROTOCOL_CFG_STOP
#define FIELD_CONFIG_PARITY        JSON_KEY_PROTOCOL_CFG_PARITY
#define FIELD_CONFIG_TIMEOUT_MS    JSON_KEY_PROTOCOL_CFG_TIMEOUT
#define FIELD_CONFIG_RETRY         JSON_KEY_PROTOCOL_CFG_RETRY
#define FIELD_CONFIG_CREATED_AT    JSON_KEY_CREATED_AT

// ==================== data_point 表字段 ====================
#define FIELD_POINT_ID              JSON_KEY_ID
#define FIELD_POINT_DEVICE_ID       FIELD_CONFIG_DEVICE_ID
#define FIELD_POINT_TYPE            JSON_KEY_DATA_POINT_TYPE
#define FIELD_POINT_ADDRESS         JSON_KEY_DATA_POINT_ADDRESS
#define FIELD_POINT_VALUE_TYPE      JSON_KEY_DATA_POINT_VALUE_TYPE
#define FIELD_POINT_SCALE            JSON_KEY_DATA_POINT_SCALE
#define FIELD_POINT_EXPRESSION      JSON_KEY_DATA_POINT_EXPRESSION
#define FIELD_POINT_DESCRIPTION     JSON_KEY_DATA_POINT_DESCRIPTION
#define FIELD_POINT_CONTROL         JSON_KEY_DATA_POINT_CONTROL
#define FIELD_POINT_CREATED_AT      JSON_KEY_CREATED_AT
#define FIELD_POINT_UPDATED_AT      JSON_KEY_UPDATED_AT

// ==================== data_point_record 表字段 ====================
#define FIELD_RECORD_ID            JSON_KEY_ID
#define FIELD_RECORD_POINT_ID      JSON_KEY_RECORD_POINT_ID
#define FIELD_RECORD_VALUE          JSON_KEY_RECORD_VALUE
#define FIELD_RECORD_TIMESTAMP     JSON_KEY_RECORD_TIMESTAMP

// ==================== device_streams 表字段 (新增) ====================
#define FIELD_STREAM_ID             JSON_KEY_ID
#define FIELD_STREAM_DEVICE_ID      JSON_KEY_DEVICE_ID
#define FIELD_STREAM_NAME           "stream_name"      // 流名称
#define FIELD_STREAM_IS_ACTIVE      "is_active"        // 激活状态
#define FIELD_STREAM_CREATED_AT     JSON_KEY_CREATED_AT

// ==================== stream_components 表字段 (新增) ====================
#define FIELD_COMP_ID               JSON_KEY_ID
#define FIELD_COMP_STREAM_ID        "stream_id"        // 所属流ID
#define FIELD_COMP_ORDER_INDEX      "order_index"      // 执行顺序
#define FIELD_COMP_LIB_NAME         "lib_name"         // 动态库/插件名
#define FIELD_COMP_CONFIG           "comp_config"      // 私有配置(JSON)
#define FIELD_COMP_OUTPUT_NEXT      "output_to_next"   // Pipeline 开关

// ==================== 索引名 ====================
#define INDEX_DEVICE_PROTOCOL       "idx_device_protocol"
#define INDEX_POINT_DEVICE_ADDR     "idx_point_device_addr"
#define INDEX_RECORD_POINT_TIME     "idx_record_point_time"
#define INDEX_STREAM_DEVICE         "idx_stream_device"
#define INDEX_COMP_STREAM_ORDER     "idx_comp_stream_order"

// ==================== SQLite 内置函数和关键字 ====================
#define SQL_NOW_LOCALTIME        "(datetime('now', 'localtime'))"
#define SQL_PRIMARY_KEY_AUTOINCREMENT "INTEGER PRIMARY KEY AUTOINCREMENT"
#define SQL_FOREIGN_KEY_CASCADE  "FOREIGN KEY(%s) REFERENCES %s(%s) ON DELETE CASCADE"
#define SQL_CREATE_IF_NOT_EXISTS "CREATE TABLE IF NOT EXISTS"
#define SQL_CREATE_INDEX_IF_NOT_EXISTS "CREATE INDEX IF NOT EXISTS"

#endif // DATABASE_DEFINES_H
