#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include "data/variant.h"

/**
 * ModbusDataConverter
 *
 * 职责：
 *  - 将 Modbus 原始响应字节流转换为通用的 `wf::Variant`
 *  - 根据功能码选择解析方式（线圈/离散量/寄存器）
 *  - 根据 `value_type`（数值类型）与 `byte_order`（字节序）解析为具体数值
 *
 * 约定（可根据实际 JSON 配置扩展）：
 *  - value_type 支持（大小写不敏感）：
 *      "bool"
 *      "int16"  / "uint16"
 *      "int32"  / "uint32"
 *      "int64"  / "uint64"
 *      "float"  / "float32"
 *      "double" / "float64"
 *  - byte_order（针对 32 位数值/浮点）：
 *      "ABCD"（默认，大端，寄存器高字在前）
 *      "DCBA"、"BADC"、"CDAB"
 */
class ModbusDataConverter
{
public:
    static wf::Variant converter(uint8_t function_code, uint16_t start_address, uint16_t length, const std::vector<uint8_t>& data, const std::string& value_type, const std::string& byte_order = "abcd");
    // 线圈/离散输入：按位展开
    static std::vector<bool> bit_data(uint16_t start_address, uint16_t length, const std::vector<uint8_t>& data);

    // 寄存器区：根据 value_type / byte_order 转为具体数值
    static wf::Variant register_data(uint16_t start_address,
                                     uint16_t length,
                                     const std::vector<uint8_t>& data,
                                     const std::string& value_type,
                                     const std::string& byte_order = "abcd");
};
