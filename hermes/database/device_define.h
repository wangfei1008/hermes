#ifndef DEVICE_DEFINE_H
#define DEVICE_DEFINE_H

// ==================== JSON 通用键值 ====================
#define JSON_KEY_ID                              "id"
#define JSON_KEY_NAME                            "name"
#define JSON_KEY_PROTOCOL                        "protocol"
#define JSON_KEY_CREATED_AT                      "created_at"
#define JSON_KEY_UPDATED_AT                      "updated_at"
#define JSON_KEY_DEVICE_ID                       "device_id"

// ==================== Protocol 配置相关 ====================
#define JSON_KEY_PROTOCOL_ID                     "protocol_id"
#define JSON_KEY_PROTOCOL_CFG                    "protocol_cfg"
#define JSON_KEY_PROTOCOL_CFG_HOST               "host"
#define JSON_KEY_PROTOCOL_CFG_PORT               "port"
#define JSON_KEY_PROTOCOL_CFG_STATION            "station"
#define JSON_KEY_PROTOCOL_CFG_BAUD               "baud"
#define JSON_KEY_PROTOCOL_CFG_DATA               "data"
#define JSON_KEY_PROTOCOL_CFG_STOP               "stop"
#define JSON_KEY_PROTOCOL_CFG_PARITY             "parity"
#define JSON_KEY_PROTOCOL_CFG_TIMEOUT            "timeout_ms"
#define JSON_KEY_PROTOCOL_CFG_RETRY              "retry"

// ==================== Data Point 相关 ====================
#define JSON_KEY_DATA_POINTS                     "data_points"
#define JSON_KEY_DATA_POINT_TYPE                 "type"
#define JSON_KEY_DATA_POINT_ADDRESS              "address"
#define JSON_KEY_DATA_POINT_VALUE_TYPE           "value_type"
#define JSON_KEY_DATA_POINT_SCALE                "scale"
#define JSON_KEY_DATA_POINT_EXPRESSION           "expression"
#define JSON_KEY_DATA_POINT_DESCRIPTION          "description"
#define JSON_KEY_DATA_POINT_CONTROL              "control"

// ==================== Data Point Record 相关 ====================
#define JSON_KEY_RECORDS                         "records"
#define JSON_KEY_RECORD_VALUE                    "value"
#define JSON_KEY_RECORD_TIMESTAMP                "timestamp"
#define JSON_KEY_RECORD_POINT_ID                 "point_id"

// ==================== 通用常量 ====================
#define JSON_VALUE_PROTOCOL_MODBUS_TCP           "modbus_tcp"
#define JSON_VALUE_PROTOCOL_MODBUS_RTU           "modbus_rtu"
#define JSON_VALUE_DATA_TYPE_COIL                "coil"
#define JSON_VALUE_DATA_TYPE_REGISTER            "register"
#define JSON_VALUE_TYPE_BOOL                     "bool"
#define JSON_VALUE_TYPE_INT16                    "int16"
#define JSON_VALUE_TYPE_UINT16                   "uint16"
#define JSON_VALUE_TYPE_INT32                    "int32"
#define JSON_VALUE_TYPE_UINT32                   "uint32"
#define JSON_VALUE_TYPE_FLOAT                    "float"
#define JSON_PARITY_NONE                         "N"
#define JSON_PARITY_EVEN                         "E"
#define JSON_PARITY_ODD                          "O"

#endif // DEVICE_DEFINE_H
