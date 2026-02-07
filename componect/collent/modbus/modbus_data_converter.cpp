#include "modbus_data_converter.h"
#include <cstring>
#include <stdexcept>

// 主转换入口
wf::Variant ModbusDataConverter::converter(
    uint8_t function_code,
    uint16_t start_address,
    uint16_t length,
    const std::vector<uint8_t>& data,
    const std::string& value_type,
    const std::string& byte_order)
{
    // 功能码 0x01, 0x02: 线圈/离散输入（位数据）
    if (function_code == 0x01 || function_code == 0x02) {
        return wf::Variant(bit_data(length, data));
    }

    // 功能码 0x03, 0x04: 保持寄存器/输入寄存器（字数据）
    if (function_code == 0x03 || function_code == 0x04) {
        return register_data(length, data, value_type, byte_order);
    }

    throw std::runtime_error("Unsupported function code");
}

// 处理位数据
std::vector<bool> ModbusDataConverter::bit_data( uint16_t length, const std::vector<uint8_t>& data)
{
    std::vector<bool> result;
    result.reserve(length);

    for (uint16_t i = 0; i < length; ++i) {
        uint16_t byte_index = i / 8;
        uint8_t bit_index = i % 8;

        if (byte_index < data.size()) {
            bool bit_value = (data[byte_index] >> bit_index) & 0x01;
            result.push_back(bit_value);
        }
    }

    return result;
}

// 处理寄存器数据
wf::Variant ModbusDataConverter::register_data( uint16_t length, const std::vector<uint8_t>& data, const std::string& value_type,const std::string& byte_order)
{
    // 将字节数据转换为 uint16 寄存器数组（大端序）
    std::vector<uint16_t> regs;
    regs.reserve(length);

    for (size_t i = 0; i < length && (i * 2 + 1) < data.size(); ++i) {
        // Modbus 标准：高字节在前（大端序）
        uint16_t reg = (static_cast<uint16_t>(data[i * 2]) << 8) |
            static_cast<uint16_t>(data[i * 2 + 1]);
        regs.push_back(reg);
    }

    // 根据类型转换
    if (value_type.empty() || value_type == "uint16") {
        return convert_to_uint16(regs);
    }
    else if (value_type == "int16") {
        return convert_to_int16(regs);
    }
    else if (value_type == "bool") {
        return convert_to_bool(regs);
    }
    else if (value_type == "uint32") {
        return convert_to_uint32(regs, byte_order);
    }
    else if (value_type == "int32") {
        return convert_to_int32(regs, byte_order);
    }
    else if (value_type == "uint64") {
        return convert_to_uint64(regs, byte_order);
    }
    else if (value_type == "int64") {
        return convert_to_int64(regs, byte_order);
    }
    else if (value_type == "float") {
        return convert_to_float(regs, byte_order);
    }
    else if (value_type == "double") {
        return convert_to_double(regs, byte_order);
    }

    throw std::runtime_error("Unsupported value type: " + value_type);
}

// ============ 类型转换函数 ============

wf::Variant ModbusDataConverter::convert_to_bool(const std::vector<uint16_t>& regs)
{
    std::vector<bool> result;
    result.reserve(regs.size());

    for (uint16_t reg : regs) {
		if (regs.size() == 1) return wf::Variant(reg != 0);
        result.push_back(reg != 0);
    }

    return wf::Variant(result);
}

wf::Variant ModbusDataConverter::convert_to_int16(const std::vector<uint16_t>& regs)
{
    std::vector<int16_t> result;
    result.reserve(regs.size());

    for (uint16_t reg : regs) {
        // 直接转换，保持二进制表示
        int16_t value;
        std::memcpy(&value, &reg, sizeof(int16_t));
		if (regs.size() == 1) return wf::Variant(value);
        result.push_back(value);
    }

    return wf::Variant(result);
}

wf::Variant ModbusDataConverter::convert_to_uint16(const std::vector<uint16_t>& regs)
{
	if (regs.size() == 1) {
		return wf::Variant(regs[0]);
	}
    return wf::Variant(regs);
}

wf::Variant ModbusDataConverter::convert_to_int32(const std::vector<uint16_t>& regs,  const std::string& byte_order)
{
    std::vector<int32_t> result;

    for (size_t i = 0; i + 1 < regs.size(); i += 2) {
        // 组合两个寄存器为 uint32（默认高字在前）
        uint32_t raw = (static_cast<uint32_t>(regs[i]) << 16) |
            static_cast<uint32_t>(regs[i + 1]);

        // 应用字节序
        raw = reorder_uint32(raw, byte_order);

        // 转换为 int32
        int32_t value;
        std::memcpy(&value, &raw, sizeof(int32_t));
		if (regs.size() == 2) return wf::Variant(value);
        result.push_back(value);
    }

    return wf::Variant(result);
}

wf::Variant ModbusDataConverter::convert_to_uint32(const std::vector<uint16_t>& regs, const std::string& byte_order)
{
    std::vector<uint32_t> result;

    for (size_t i = 0; i + 1 < regs.size(); i += 2) {
        uint32_t value = (static_cast<uint32_t>(regs[i]) << 16) |
            static_cast<uint32_t>(regs[i + 1]);

        value = reorder_uint32(value, byte_order);
		if (regs.size() == 2) return wf::Variant(value);
        result.push_back(value);
    }

    return wf::Variant(result);
}

wf::Variant ModbusDataConverter::convert_to_int64(const std::vector<uint16_t>& regs, const std::string& byte_order)
{
    std::vector<int64_t> result;

    for (size_t i = 0; i + 3 < regs.size(); i += 4) {
        // 组合四个寄存器为 uint64（默认高字在前）
        uint64_t raw = (static_cast<uint64_t>(regs[i]) << 48) |
            (static_cast<uint64_t>(regs[i + 1]) << 32) |
            (static_cast<uint64_t>(regs[i + 2]) << 16) |
            static_cast<uint64_t>(regs[i + 3]);

        // 应用字节序
        raw = reorder_uint64(raw, byte_order);

        // 转换为 int64
        int64_t value;
        std::memcpy(&value, &raw, sizeof(int64_t));
		if (regs.size() == 4) return wf::Variant(value);
        result.push_back(value);
    }

    return wf::Variant(result);
}

wf::Variant ModbusDataConverter::convert_to_uint64(const std::vector<uint16_t>& regs, const std::string& byte_order)
{
    std::vector<uint64_t> result;

    for (size_t i = 0; i + 3 < regs.size(); i += 4) {
        uint64_t value = (static_cast<uint64_t>(regs[i]) << 48) |
            (static_cast<uint64_t>(regs[i + 1]) << 32) |
            (static_cast<uint64_t>(regs[i + 2]) << 16) |
            static_cast<uint64_t>(regs[i + 3]);

        value = reorder_uint64(value, byte_order);
		if (regs.size() == 4) return wf::Variant(value);
        result.push_back(value);
    }

    return wf::Variant(result);
}

wf::Variant ModbusDataConverter::convert_to_float(const std::vector<uint16_t>& regs,const std::string& byte_order)
{
    std::vector<float> result;

    for (size_t i = 0; i + 1 < regs.size(); i += 2) {
        // 组合两个寄存器为 uint32
        uint32_t raw = (static_cast<uint32_t>(regs[i]) << 16) |
            static_cast<uint32_t>(regs[i + 1]);

        // 应用字节序
        raw = reorder_uint32(raw, byte_order);

        // 转换为 float
        float value;
        std::memcpy(&value, &raw, sizeof(float));
		if (regs.size() == 2) return wf::Variant(value);
        result.push_back(value);
    }

    return wf::Variant(result);
}

wf::Variant ModbusDataConverter::convert_to_double(const std::vector<uint16_t>& regs,const std::string& byte_order)
{
    std::vector<double> result;

    for (size_t i = 0; i + 3 < regs.size(); i += 4) {
        // 组合四个寄存器为 uint64
        uint64_t raw = (static_cast<uint64_t>(regs[i]) << 48) |
            (static_cast<uint64_t>(regs[i + 1]) << 32) |
            (static_cast<uint64_t>(regs[i + 2]) << 16) |
            static_cast<uint64_t>(regs[i + 3]);

        // 应用字节序
        raw = reorder_uint64(raw, byte_order);

        // 转换为 double
        double value;
        std::memcpy(&value, &raw, sizeof(double));
        if (regs.size() == 4) return wf::Variant(value);
        result.push_back(value);
    }

    return wf::Variant(result);
}

// ============ 字节序处理 ============

uint32_t ModbusDataConverter::reorder_uint32(uint32_t value, const std::string& byte_order)
{
    if (byte_order == "ABCD") {
        return value;  // 大端，无需转换
    }
    else if (byte_order == "DCBA") {
        // 完全反转
        return ((value & 0x000000FF) << 24) |
            ((value & 0x0000FF00) << 8) |
            ((value & 0x00FF0000) >> 8) |
            ((value & 0xFF000000) >> 24);
    }
    else if (byte_order == "BADC") {
        // 字交换
        return ((value & 0x0000FFFF) << 16) |
            ((value & 0xFFFF0000) >> 16);
    }
    else if (byte_order == "CDAB") {
        // 字节对交换
        return ((value & 0x00FF00FF) << 8) |
            ((value & 0xFF00FF00) >> 8);
    }

    return value;  // 未知字节序，返回原值
}

uint64_t ModbusDataConverter::reorder_uint64(uint64_t value, const std::string& byte_order)
{
    if (byte_order == "ABCD") {
        return value;  // 大端，无需转换
    }
    else if (byte_order == "DCBA") {
        // 完全反转
        return ((value & 0x00000000000000FFULL) << 56) |
            ((value & 0x000000000000FF00ULL) << 40) |
            ((value & 0x0000000000FF0000ULL) << 24) |
            ((value & 0x00000000FF000000ULL) << 8) |
            ((value & 0x000000FF00000000ULL) >> 8) |
            ((value & 0x0000FF0000000000ULL) >> 24) |
            ((value & 0x00FF000000000000ULL) >> 40) |
            ((value & 0xFF00000000000000ULL) >> 56);
    }
    else if (byte_order == "BADC") {
        // 字交换（16位为单位）
        return ((value & 0x000000000000FFFFULL) << 48) |
            ((value & 0x00000000FFFF0000ULL) << 16) |
            ((value & 0x0000FFFF00000000ULL) >> 16) |
            ((value & 0xFFFF000000000000ULL) >> 48);
    }
    else if (byte_order == "CDAB") {
        // 字节对交换
        return ((value & 0x00FF00FF00FF00FFULL) << 8) |
            ((value & 0xFF00FF00FF00FF00ULL) >> 8);
    }

    return value;  // 未知字节序，返回原值
}