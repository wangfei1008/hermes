#pragma once

#include <vector>
#include <cstdint>
#include "data/variant.h"

/**
 * ModbusDataConverter
 *
 * 职责：
 *  - 将 Modbus 原始响应数据转换为通用的 wf::Variant 类型，便于上层 DataContext 使用
 *  - 当前实现只做基础解析：
 *      * bit_data()  -> std::vector<bool>
 *      * register_data() -> 将寄存器值打包为 Int32 数组（wf::Variant::Int32Array）
 *  - 更细粒度的“数值类型/字节序/缩放”等，可在上层结合 CommandParam 再二次加工
 */
class ModbusDataConverter
{
public:
    static wf::Variant converter(uint8_t function_code, uint16_t start_address, uint16_t length, const std::vector<uint8_t>& data, const std::string& value_type, const std::string& byte_order = "abcd");
    // 线圈/离散输入：按位展开
    static std::vector<bool> bit_data(uint16_t start_address, uint16_t length, const std::vector<uint8_t>& data);

    // 寄存器区：解析为 16bit 寄存器数组并转为 Variant(int32 数组)
    static wf::Variant register_data(uint16_t start_address, uint16_t length, const std::vector<uint8_t>& data);
};
