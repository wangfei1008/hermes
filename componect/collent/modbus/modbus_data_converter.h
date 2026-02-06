#pragma once

#include "data/variant.h"
#include <cstdint>
#include <string>
#include <vector>


/**
    * @brief Modbus 数据转换器
    *
    * 负责将 Modbus 原始数据（字节流）转换为结构化的 Variant 对象
    * 支持所有标准 Modbus 功能码和多种数据类型
    */
class ModbusDataConverter 
{
public:
    /**
        * @brief 主转换入口
        *
        * @param function_code Modbus 功能码 (0x01-0x04)
        * @param start_address 起始地址
        * @param length 数据长度（线圈/寄存器数量）
        * @param data 原始字节数据
        * @param value_type 数值类型字符串 (bool, int16, uint16, int32, uint32, int64, uint64, float, double)
        * @param byte_order 字节序 (ABCD, DCBA, BADC, CDAB)，默认 "ABCD"
        * @return wf::Variant 转换后的数据
        */
    static wf::Variant converter(uint8_t function_code,
        uint16_t start_address,
        uint16_t length,
        const std::vector<uint8_t>& data,
        const std::string& value_type = "",
        const std::string& byte_order = "ABCD");

private:
    /**
        * @brief 处理位数据（线圈/离散输入）
        *
        * @param length 位数量
        * @param data 字节数据
        * @return std::vector<bool> 位值数组
        */
    static std::vector<bool> bit_data(uint16_t length, const std::vector<uint8_t>& data);

    /**
        * @brief 处理寄存器数据（保持寄存器/输入寄存器）
        *
        * @param length 寄存器数量
        * @param data 字节数据
        * @param value_type 数值类型
        * @param byte_order 字节序
        * @return wf::Variant 转换后的数据
        */
    static wf::Variant register_data(uint16_t length, const std::vector<uint8_t>& data, const std::string& value_type, const std::string& byte_order);

    // 类型转换辅助函数
    static wf::Variant convert_to_bool(const std::vector<uint16_t>& regs);
    static wf::Variant convert_to_int16(const std::vector<uint16_t>& regs);
    static wf::Variant convert_to_uint16(const std::vector<uint16_t>& regs);
    static wf::Variant convert_to_int32(const std::vector<uint16_t>& regs,
        const std::string& byte_order);
    static wf::Variant convert_to_uint32(const std::vector<uint16_t>& regs,
        const std::string& byte_order);
    static wf::Variant convert_to_int64(const std::vector<uint16_t>& regs,
        const std::string& byte_order);
    static wf::Variant convert_to_uint64(const std::vector<uint16_t>& regs,
        const std::string& byte_order);
    static wf::Variant convert_to_float(const std::vector<uint16_t>& regs,
        const std::string& byte_order);
    static wf::Variant convert_to_double(const std::vector<uint16_t>& regs,
        const std::string& byte_order);

    // 字节序处理
    static uint32_t reorder_uint32(uint32_t value, const std::string& byte_order);
    static uint64_t reorder_uint64(uint64_t value, const std::string& byte_order);
};
