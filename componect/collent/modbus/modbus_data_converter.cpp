#include "modbus_data_converter.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <limits>


namespace {

    // 值类型枚举
    enum class ValueTypeKind {
        Bool,
        Int16,
        UInt16,
        Int32,
        UInt32,
        Int64,
        UInt64,
        Float32,
        Float64,
        RawRegisters, // 默认：返回寄存器数组
    };

    // 字符串转小写
    std::string to_lower(const std::string& s) {
        std::string result;
        result.reserve(s.size());
        for (char c : s) {
            result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
        return result;
    }

    // 解析值类型字符串
    ValueTypeKind parse_value_type(const std::string& value_type) {
        if (value_type.empty()) {
            return ValueTypeKind::RawRegisters;
        }

        std::string vt = to_lower(value_type);

        if (vt == "bool" || vt == "bit")           return ValueTypeKind::Bool;
        if (vt == "int16" || vt == "i16")          return ValueTypeKind::Int16;
        if (vt == "uint16" || vt == "u16")         return ValueTypeKind::UInt16;
        if (vt == "int32" || vt == "i32")          return ValueTypeKind::Int32;
        if (vt == "uint32" || vt == "u32")         return ValueTypeKind::UInt32;
        if (vt == "int64" || vt == "i64")          return ValueTypeKind::Int64;
        if (vt == "uint64" || vt == "u64")         return ValueTypeKind::UInt64;
        if (vt == "float" || vt == "float32")      return ValueTypeKind::Float32;
        if (vt == "double" || vt == "float64")     return ValueTypeKind::Float64;

        return ValueTypeKind::RawRegisters;
    }

    // 标准化字节序字符串
    std::string normalize_byte_order(const std::string& byte_order) {
        std::string order = to_lower(byte_order);

        // 移除空格
        order.erase(std::remove_if(order.begin(), order.end(), ::isspace), order.end());

        // 验证字节序格式
        if (order.size() == 4) {
            bool valid = true;
            int counts[4] = { 0, 0, 0, 0 }; // a, b, c, d

            for (char c : order) {
                if (c >= 'a' && c <= 'd') {
                    counts[c - 'a']++;
                }
                else {
                    valid = false;
                    break;
                }
            }

            // 每个字母应该出现且只出现一次
            for (int count : counts) {
                if (count != 1) {
                    valid = false;
                    break;
                }
            }

            if (valid) {
                return order;
            }
        }

        // 默认大端序
        return "abcd";
    }

} // anonymous namespace

// ========== 主转换入口 ==========

wf::Variant ModbusDataConverter::converter(uint8_t function_code,
    uint16_t start_address,
    uint16_t length,
    const std::vector<uint8_t>& data,
    const std::string& value_type,
    const std::string& byte_order)
{
    // 功能码定义（来自 modbus.h）
    constexpr uint8_t FC_READ_COILS = 0x01;
    constexpr uint8_t FC_READ_DISCRETE_INPUTS = 0x02;
    constexpr uint8_t FC_READ_HOLDING_REGISTERS = 0x03;
    constexpr uint8_t FC_READ_INPUT_REGISTERS = 0x04;

    // 处理线圈和离散输入（位数据）
    if (function_code == FC_READ_COILS || function_code == FC_READ_DISCRETE_INPUTS) {
        auto bits = bit_data(length, data);

        ValueTypeKind kind = parse_value_type(value_type);

        // 如果指定为 bool 且只有一个位，返回单个 bool
        if (kind == ValueTypeKind::Bool && bits.size() == 1)
            return wf::Variant(bits[0]);

        return wf::Variant(bits);
    }

    // 处理保持寄存器和输入寄存器
    if (function_code == FC_READ_HOLDING_REGISTERS || function_code == FC_READ_INPUT_REGISTERS)
        return register_data(length, data, value_type, byte_order);

    // 不支持的功能码
    return wf::Variant();
}

// ========== 位数据处理 ==========

std::vector<bool> ModbusDataConverter::bit_data(uint16_t length, const std::vector<uint8_t>& data)
{
    std::vector<bool> result;
    result.reserve(length);

    for (uint16_t i = 0; i < length; ++i) {
        size_t byte_idx = i / 8;
        size_t bit_idx = i % 8;

        bool value = false;
        if (byte_idx < data.size()) {
            value = ((data[byte_idx] >> bit_idx) & 0x01) != 0;
        }

        result.push_back(value);
    }

    return result;
}

// ========== 寄存器数据处理 ==========

wf::Variant ModbusDataConverter::register_data(uint16_t length,const std::vector<uint8_t>& data,const std::string& value_type,const std::string& byte_order)
{
    // 将字节数组转换为寄存器数组（Modbus 大端序：高字节在前）
    size_t max_regs = data.size() / 2;
    size_t reg_count = std::min(static_cast<size_t>(length), max_regs);

    std::vector<uint16_t> regs;
    regs.reserve(reg_count);

    for (size_t i = 0; i < reg_count; ++i) {
        size_t idx = i * 2;
        if (idx + 1 < data.size()) {
            uint16_t reg = static_cast<uint16_t>(
                (static_cast<uint16_t>(data[idx]) << 8) |
                static_cast<uint16_t>(data[idx + 1])
                );
            regs.push_back(reg);
        }
    }

    if (regs.empty()) {
        return wf::Variant();
    }

    // 根据值类型转换
    ValueTypeKind kind = parse_value_type(value_type);
    std::string normalized_order = normalize_byte_order(byte_order);

    switch (kind) {
    case ValueTypeKind::Bool:
        return convert_to_bool(regs);

    case ValueTypeKind::Int16:
        return convert_to_int16(regs);

    case ValueTypeKind::UInt16:
        return convert_to_uint16(regs);

    case ValueTypeKind::Int32:
        return convert_to_int32(regs, normalized_order);

    case ValueTypeKind::UInt32:
        return convert_to_uint32(regs, normalized_order);

    case ValueTypeKind::Int64:
        return convert_to_int64(regs, normalized_order);

    case ValueTypeKind::UInt64:
        return convert_to_uint64(regs, normalized_order);

    case ValueTypeKind::Float32:
        return convert_to_float(regs, normalized_order);

    case ValueTypeKind::Float64:
        return convert_to_double(regs, normalized_order);

    case ValueTypeKind::RawRegisters:
    default:
        // 返回寄存器值数组（转为 int32）
        std::vector<int32_t> values;
        values.reserve(regs.size());
        for (uint16_t r : regs) {
            values.push_back(static_cast<int32_t>(r));
        }
        return wf::Variant(values);
    }
}

// ========== 类型转换实现 ==========

wf::Variant ModbusDataConverter::convert_to_bool(const std::vector<uint16_t>& regs) {
    if (regs.empty()) {
        return wf::Variant();
    }
    // 使用寄存器最低位
    bool value = (regs[0] & 0x0001) != 0;
    return wf::Variant(value);
}

wf::Variant ModbusDataConverter::convert_to_int16(const std::vector<uint16_t>& regs) {
    if (regs.empty()) {
        return wf::Variant();
    }
    // 转换为有符号 int16，然后提升为 int32 存储
    int16_t value = static_cast<int16_t>(regs[0]);
    return wf::Variant(static_cast<int32_t>(value));
}

wf::Variant ModbusDataConverter::convert_to_uint16(const std::vector<uint16_t>& regs) {
    if (regs.empty()) {
        return wf::Variant();
    }
    // 提升为 uint32 存储
    return wf::Variant(static_cast<uint32_t>(regs[0]));
}

wf::Variant ModbusDataConverter::convert_to_int32(const std::vector<uint16_t>& regs,
    const std::string& byte_order) {
    if (regs.size() < 2) {
        return wf::Variant();
    }

    // 使用 MODBUS_GET_INT32_FROM_INT16 宏的逻辑
    // #define MODBUS_GET_INT32_FROM_INT16(tab_int16, index) \
    //   (((int32_t) tab_int16[(index)] << 16) | (int32_t) tab_int16[(index) + 1])

    int32_t value = (static_cast<int32_t>(regs[0]) << 16) |
        static_cast<int32_t>(regs[1]);

    // 应用字节序转换
    if (byte_order != "abcd") {
        uint32_t temp;
        std::memcpy(&temp, &value, sizeof(uint32_t));
        temp = reorder_uint32(temp, byte_order);
        std::memcpy(&value, &temp, sizeof(int32_t));
    }

    return wf::Variant(value);
}

wf::Variant ModbusDataConverter::convert_to_uint32(const std::vector<uint16_t>& regs,
    const std::string& byte_order) {
    if (regs.size() < 2) {
        return wf::Variant();
    }

    uint32_t value = (static_cast<uint32_t>(regs[0]) << 16) |
        static_cast<uint32_t>(regs[1]);

    // 应用字节序转换
    value = reorder_uint32(value, byte_order);

    return wf::Variant(value);
}

wf::Variant ModbusDataConverter::convert_to_int64(const std::vector<uint16_t>& regs,
    const std::string& byte_order) {
    if (regs.size() < 4) {
        return wf::Variant();
    }

    // 使用 MODBUS_GET_INT64_FROM_INT16 宏的逻辑
    // #define MODBUS_GET_INT64_FROM_INT16(tab_int16, index) \
    //   (((int64_t) tab_int16[(index)] << 48) | ((int64_t) tab_int16[(index) + 1] << 32) | \
    //    ((int64_t) tab_int16[(index) + 2] << 16) | (int64_t) tab_int16[(index) + 3])

    int64_t value = (static_cast<int64_t>(regs[0]) << 48) |
        (static_cast<int64_t>(regs[1]) << 32) |
        (static_cast<int64_t>(regs[2]) << 16) |
        static_cast<int64_t>(regs[3]);

    // 应用字节序转换
    if (byte_order != "abcd") {
        uint64_t temp;
        std::memcpy(&temp, &value, sizeof(uint64_t));
        temp = reorder_uint64(temp, byte_order);
        std::memcpy(&value, &temp, sizeof(int64_t));
    }

    return wf::Variant(value);
}

wf::Variant ModbusDataConverter::convert_to_uint64(const std::vector<uint16_t>& regs,
    const std::string& byte_order) {
    if (regs.size() < 4) {
        return wf::Variant();
    }

    uint64_t value = (static_cast<uint64_t>(regs[0]) << 48) |
        (static_cast<uint64_t>(regs[1]) << 32) |
        (static_cast<uint64_t>(regs[2]) << 16) |
        static_cast<uint64_t>(regs[3]);

    // 应用字节序转换
    value = reorder_uint64(value, byte_order);

    return wf::Variant(value);
}

wf::Variant ModbusDataConverter::convert_to_float(const std::vector<uint16_t>& regs,
    const std::string& byte_order) {
    if (regs.size() < 2) {
        return wf::Variant();
    }

    // 根据字节序使用相应的 modbus_get_float_xxxx 函数逻辑
    // 这些函数的实现通常是对两个寄存器进行字节重排后转换为 float

    uint32_t raw_u32 = (static_cast<uint32_t>(regs[0]) << 16) |
        static_cast<uint32_t>(regs[1]);

    // 应用字节序
    raw_u32 = reorder_uint32(raw_u32, byte_order);

    // 转换为 float
    float value;
    std::memcpy(&value, &raw_u32, sizeof(float));

    return wf::Variant(value);
}

wf::Variant ModbusDataConverter::convert_to_double(const std::vector<uint16_t>& regs,
    const std::string& byte_order) {
    if (regs.size() < 4) {
        return wf::Variant();
    }

    // 将 4 个寄存器组合为 double
    uint64_t raw_u64 = (static_cast<uint64_t>(regs[0]) << 48) |
        (static_cast<uint64_t>(regs[1]) << 32) |
        (static_cast<uint64_t>(regs[2]) << 16) |
        static_cast<uint64_t>(regs[3]);

    // 应用字节序
    raw_u64 = reorder_uint64(raw_u64, byte_order);

    // 转换为 double
    double value;
    std::memcpy(&value, &raw_u64, sizeof(double));

    return wf::Variant(value);
}

// ========== 字节序转换 ==========

uint32_t ModbusDataConverter::reorder_uint32(uint32_t value, const std::string& byte_order) {
    if (byte_order == "abcd") {
        // 默认大端序，无需转换
        return value;
    }

    // 提取 4 个字节
    uint8_t A = static_cast<uint8_t>((value >> 24) & 0xFF);
    uint8_t B = static_cast<uint8_t>((value >> 16) & 0xFF);
    uint8_t C = static_cast<uint8_t>((value >> 8) & 0xFF);
    uint8_t D = static_cast<uint8_t>(value & 0xFF);

    // 创建查找表
    uint8_t bytes[4] = { A, B, C, D };
    uint8_t result[4];

    // 根据字节序重新排列
    for (int i = 0; i < 4; ++i) {
        char ch = byte_order[i];
        int src_idx = (ch >= 'a' && ch <= 'd') ? (ch - 'a') : 0;
        result[i] = bytes[src_idx];
    }

    // 重新组合
    return (static_cast<uint32_t>(result[0]) << 24) |
        (static_cast<uint32_t>(result[1]) << 16) |
        (static_cast<uint32_t>(result[2]) << 8) |
        static_cast<uint32_t>(result[3]);
}

uint64_t ModbusDataConverter::reorder_uint64(uint64_t value, const std::string& byte_order) {
    if (byte_order == "abcd") {
        // 默认大端序，无需转换（对于 64 位，扩展为 "abcdefgh"）
        return value;
    }

    // 对于 64 位数据，字节序可能需要扩展
    // 这里简化处理：将 4 字节字节序应用到高低两个 32 位部分

    if (byte_order == "dcba") {
        // 完全反转字节序
        return ((value & 0x00000000000000FFULL) << 56) |
            ((value & 0x000000000000FF00ULL) << 40) |
            ((value & 0x0000000000FF0000ULL) << 24) |
            ((value & 0x00000000FF000000ULL) << 8) |
            ((value & 0x000000FF00000000ULL) >> 8) |
            ((value & 0x0000FF0000000000ULL) >> 24) |
            ((value & 0x00FF000000000000ULL) >> 40) |
            ((value & 0xFF00000000000000ULL) >> 56);
    }

    if (byte_order == "badc") {
        // 交换每对字节
        return ((value & 0xFF00FF00FF00FF00ULL) >> 8) |
            ((value & 0x00FF00FF00FF00FFULL) << 8);
    }

    if (byte_order == "cdab") {
        // 交换每个 16 位字
        return ((value & 0xFFFF000000000000ULL) >> 48) |
            ((value & 0x0000FFFF00000000ULL) >> 16) |
            ((value & 0x00000000FFFF0000ULL) << 16) |
            ((value & 0x000000000000FFFFULL) << 48);
    }

    // 其他情况：保持不变
    return value;
}