#include "modbus_data_converter.h"
#include "modbus_parameter.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>

namespace {

// 解析 value_type 字符串为内部枚举
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
    RawRegisters, // 回退：按寄存器数组返回
};

std::string to_lower(const std::string& s)
{
    std::string r(s.size(), '\0');
    std::transform(s.begin(), s.end(), r.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return r;
}

ValueTypeKind parse_value_type(const std::string& value_type)
{
    std::string vt = to_lower(value_type);

    if (vt == "bool" || vt == "bit")               return ValueTypeKind::Bool;
    if (vt == "int16" || vt == "i16")              return ValueTypeKind::Int16;
    if (vt == "uint16" || vt == "u16")             return ValueTypeKind::UInt16;
    if (vt == "int32" || vt == "i32")              return ValueTypeKind::Int32;
    if (vt == "uint32" || vt == "u32")             return ValueTypeKind::UInt32;
    if (vt == "int64" || vt == "i64")              return ValueTypeKind::Int64;
    if (vt == "uint64" || vt == "u64")             return ValueTypeKind::UInt64;
    if (vt == "float" || vt == "float32")          return ValueTypeKind::Float32;
    if (vt == "double" || vt == "float64")         return ValueTypeKind::Float64;

    return ValueTypeKind::RawRegisters;
}

// 将两个 16 位寄存器按字节序组合为 32 位无符号整数
uint32_t build_u32_from_regs(const uint16_t* regs, const std::string& byte_order)
{
    uint8_t A = static_cast<uint8_t>(regs[0] >> 8);
    uint8_t B = static_cast<uint8_t>(regs[0] & 0xFF);
    uint8_t C = static_cast<uint8_t>(regs[1] >> 8);
    uint8_t D = static_cast<uint8_t>(regs[1] & 0xFF);

    uint8_t src[4] = { A, B, C, D };
    uint8_t dst[4] = { A, B, C, D };

    std::string ord = to_lower(byte_order);
    if (ord.size() != 4) {
        ord = "abcd";
    }

    auto idx_of = [](char ch) -> int {
        switch (ch) {
        case 'a': return 0;
        case 'b': return 1;
        case 'c': return 2;
        case 'd': return 3;
        default:  return -1;
        }
    };

    for (int i = 0; i < 4; ++i) {
        int src_idx = idx_of(ord[static_cast<size_t>(i)]);
        if (src_idx >= 0)
            dst[i] = src[src_idx];
    }

    uint32_t raw = 0;
    raw |= static_cast<uint32_t>(dst[0]) << 24;
    raw |= static_cast<uint32_t>(dst[1]) << 16;
    raw |= static_cast<uint32_t>(dst[2]) << 8;
    raw |= static_cast<uint32_t>(dst[3]);
    return raw;
}

} // namespace

// 主入口：根据功能码 + 类型信息，转换为 wf::Variant
wf::Variant ModbusDataConverter::converter(uint8_t function_code,
                                           uint16_t start_address,
                                           uint16_t length,
                                           const std::vector<uint8_t>& data,
                                           const std::string& value_type,
                                           const std::string& byte_order)
{
    (void)start_address; // 当前实现未用到起始地址

    // 线圈 / 离散输入
    if (function_code == modbus_function_code::COILS ||
        function_code == modbus_function_code::DISCRETE_INPUTS)
    {
        auto bits = bit_data(start_address, length, data);
        ValueTypeKind kind = parse_value_type(value_type);

        if (kind == ValueTypeKind::Bool && !bits.empty()) {
            // 单个 bool（典型场景：length == 1）
            return wf::Variant(bits[0]);
        }

        // 否则：返回 0/1 的 int32 数组
        std::vector<int32_t> vals;
        vals.reserve(bits.size());
        for (bool b : bits) {
            vals.push_back(b ? 1 : 0);
        }
        return wf::Variant(vals);
    }

    // 寄存器区
    if (function_code == modbus_function_code::HOLDING_REGISTERS ||
        function_code == modbus_function_code::INPUT_REGISTERS)
    {
        return register_data(start_address, length, data, value_type, byte_order);
    }

    // 未支持的功能码：返回空 Variant
    return wf::Variant();
}

// 处理位数据（保持为 std::vector<bool>，由上层选择如何封装）
std::vector<bool> ModbusDataConverter::bit_data(uint16_t /*start_address*/,
                                                uint16_t length,
                                                const std::vector<uint8_t>& data)
{
    std::vector<bool> result;
    result.reserve(length);

    for (uint16_t i = 0; i < length; ++i) {
        size_t byte_idx = static_cast<size_t>(i / 8);
        size_t bit_idx  = static_cast<size_t>(i % 8);
        if (byte_idx >= data.size()) {
            result.push_back(false);
            continue;
        }
        bool value = ((data[byte_idx] >> bit_idx) & 0x01) != 0;
        result.push_back(value);
    }
    return result;
}

// 处理寄存器数据：根据 value_type / byte_order 解析为具体数值
wf::Variant ModbusDataConverter::register_data(uint16_t /*start_address*/,
                                               uint16_t length,
                                               const std::vector<uint8_t>& data,
                                               const std::string& value_type,
                                               const std::string& byte_order)
{
    // 将字节流切分为 16bit 寄存器（Modbus：高字节在前）
    size_t max_regs_from_data = data.size() / 2;
    size_t reg_count = std::min(static_cast<size_t>(length),
                                max_regs_from_data);

    std::vector<uint16_t> regs;
    regs.reserve(reg_count);
    for (size_t i = 0; i < reg_count; ++i) {
        size_t idx = i * 2;
        uint16_t reg = static_cast<uint16_t>(
            (static_cast<uint16_t>(data[idx]) << 8) |
            static_cast<uint16_t>(data[idx + 1]));
        regs.push_back(reg);
    }

    if (regs.empty()) {
        return wf::Variant(); // 空
    }

    ValueTypeKind kind = parse_value_type(value_type);

    switch (kind) {
    case ValueTypeKind::Bool:
        // 寄存器区的 bool：约定使用寄存器低位
    {
        bool b = (regs[0] & 0x0001u) != 0;
        return wf::Variant(b);
    }

    case ValueTypeKind::Int16:
    {
        int16_t v = static_cast<int16_t>(regs[0]);
        // 直接提升为 int32 存入 Variant
        return wf::Variant(static_cast<int32_t>(v));
    }

    case ValueTypeKind::UInt16:
    {
        uint16_t v = regs[0];
        return wf::Variant(static_cast<uint32_t>(v));
    }

    case ValueTypeKind::Int32:
    {
        if (regs.size() < 2) return wf::Variant();
        uint32_t raw_u32 = build_u32_from_regs(regs.data(), byte_order);
        int32_t v = static_cast<int32_t>(raw_u32);
        return wf::Variant(v);
    }

    case ValueTypeKind::UInt32:
    {
        if (regs.size() < 2) return wf::Variant();
        uint32_t raw_u32 = build_u32_from_regs(regs.data(), byte_order);
        return wf::Variant(raw_u32);
    }

    case ValueTypeKind::Float32:
    {
        if (regs.size() < 2) return wf::Variant();
        uint32_t raw_u32 = build_u32_from_regs(regs.data(), byte_order);
        float f = 0.0f;
        std::memcpy(&f, &raw_u32, sizeof(f));
        return wf::Variant(f);
    }

    case ValueTypeKind::Int64:
    case ValueTypeKind::UInt64:
    case ValueTypeKind::Float64:
    {
        // 简单支持：按 Modbus 默认寄存器顺序 ABCD EFGH 组合为 64 位
        if (regs.size() < 4) return wf::Variant();

        uint8_t bytes[8];
        for (int i = 0; i < 4; ++i) {
            bytes[i * 2]     = static_cast<uint8_t>(regs[static_cast<size_t>(i)] >> 8);
            bytes[i * 2 + 1] = static_cast<uint8_t>(regs[static_cast<size_t>(i)] & 0xFF);
        }

        uint64_t raw = 0;
        for (int i = 0; i < 8; ++i) {
            raw = (raw << 8) | static_cast<uint64_t>(bytes[i]);
        }

        if (kind == ValueTypeKind::Int64) {
            int64_t v = static_cast<int64_t>(raw);
            return wf::Variant(v);
        }
        if (kind == ValueTypeKind::UInt64) {
            return wf::Variant(static_cast<uint64_t>(raw));
        }

        // Float64 / double
        double d = 0.0;
        std::memcpy(&d, &raw, sizeof(d));
        return wf::Variant(d);
    }

    case ValueTypeKind::RawRegisters:
    default:
        break;
    }

    // 默认：返回寄存器值数组（提升为 int32）
    std::vector<int32_t> values;
    values.reserve(regs.size());
    for (uint16_t r : regs) {
        values.push_back(static_cast<int32_t>(r));
    }
    return wf::Variant(values);
}

#include "modbus_data_converter.h"
#include "modbus.h"
#include "modbus_parameter.h"

wf::Variant ModbusDataConverter::converter(uint8_t function_code, uint16_t start_address, uint16_t length, const std::vector<uint8_t>& data, const std::string& value_type, const std::string& byte_order)
{
    if (function_code == modbus_function_code::COILS || function_code == modbus_function_code::DISCRETE_INPUTS)
        bit_data(start_address, length, data);
    if (function_code == modbus_function_code::HOLDING_REGISTERS || function_code == modbus_function_code::INPUT_REGISTERS)
        return register_data(start_address, length, data);
    return wf::Variant();
}

// 处理位数据
std::vector<bool> ModbusDataConverter::bit_data(uint16_t start_address, uint16_t length, const std::vector<uint8_t>& data)
{
    std::vector<bool> result;
    for (uint16_t i = 0; i < length; i++) {
        // 从字节数组中提取位值
        int byte_idx = i / 8;
        int bit_idx = i % 8;
        bool value = (data[byte_idx] >> bit_idx) & 0x01;
        result.push_back(value);
    }
    return result;
}

// 处理寄存器数据
wf::Variant ModbusDataConverter::register_data(uint16_t start_address, uint16_t length, const std::vector<uint8_t>& data)
{
    std::vector<int32_t> values;
    if (data.empty()) {
        return wf::Variant(values);
    }

    // 将字节数组转换为uint16_t数组
    std::vector<uint16_t> registers;
    for (size_t i = 0; i < data.size(); i += 2) {
        if (i + 1 < data.size()) {
            uint16_t reg = (data[i] << 8) | data[i + 1];
            registers.push_back(reg);
        }
    }

    // 遍历每个寄存器
    for (uint16_t i = 0; i < registers.size(); ) {
        uint16_t addr = start_address + i;

        if (data_points_.find(addr) == data_points_.end()) {
            i++;
            continue;
        }

        const DataPoint& point = data_points_[addr];

        switch (point.type) {
        case DataType::UINT16:
            to_uint16(point, registers, i);
            i += 1;
            break;

        case DataType::INT16:
            to_int16(point, registers, i);
            i += 1;
            break;

        case DataType::UINT32:
            to_uint32(point, registers, i);
            i += 2;
            break;

        case DataType::INT32:
            to_int32(point, registers, i);
            i += 2;
            break;

        case DataType::UINT64:
            to_uint64(point, registers, i);
            i += 4;
            break;

        case DataType::INT64:
            to_int64(point, registers, i);
            i += 4;
            break;

        case DataType::FLOAT_ABCD:
        case DataType::FLOAT_DCBA:
        case DataType::FLOAT_BADC:
        case DataType::FLOAT_CDAB:
            to_float(point, registers, i);
            i += 2;
            break;

        default:
            i++;
            break;
        }
    }
}

// 处理UINT16
inline void ModbusDataConverter::to_uint16(const DataPoint& point, const std::vector<uint16_t>& regs, size_t idx)
{
    if (idx >= regs.size()) return;

    uint16_t raw_value = regs[idx];
    double value = raw_value * point.scale + point.offset;
}

// 处理INT16
inline void ModbusDataConverter::to_int16(const DataPoint& point, const std::vector<uint16_t>& regs, size_t idx)
{
    if (idx >= regs.size()) return;

    int16_t raw_value = static_cast<int16_t>(regs[idx]);
    double value = raw_value * point.scale + point.offset;
}

// 处理UINT32
inline void ModbusDataConverter::to_uint32(const DataPoint& point, const std::vector<uint16_t>& regs, size_t idx)
{
    if (idx + 1 >= regs.size()) return;

    uint32_t raw_value;
    switch (point.byte_order) {
    case ByteOrder::ABCD:  // Big-endian
        raw_value = ((uint32_t)regs[idx] << 16) | regs[idx + 1];
        break;
    case ByteOrder::DCBA:  // Little-endian
        raw_value = ((uint32_t)regs[idx + 1] << 16) | regs[idx];
        break;
    case ByteOrder::BADC:
        raw_value = ((uint32_t)regs[idx + 1] << 16) | regs[idx];
        raw_value = ((raw_value & 0xFF00FF00) >> 8) | ((raw_value & 0x00FF00FF) << 8);
        break;
    case ByteOrder::CDAB:
        raw_value = ((uint32_t)regs[idx] << 16) | regs[idx + 1];
        raw_value = ((raw_value & 0xFF00FF00) >> 8) | ((raw_value & 0x00FF00FF) << 8);
        break;
    default:
        raw_value = ((uint32_t)regs[idx] << 16) | regs[idx + 1];
    }

    double value = raw_value * point.scale + point.offset;
}

// 处理INT32
inline void ModbusDataConverter::to_int32(const DataPoint& point, const std::vector<uint16_t>& regs, size_t idx)
{
    if (idx + 1 >= regs.size()) return;

    int32_t raw_value = MODBUS_GET_INT32_FROM_INT16(regs.data(), idx);

    // 如果需要其他字节序，可以先转换
    if (point.byte_order != ByteOrder::ABCD) {
        uint32_t temp;
        std::memcpy(&temp, &raw_value, sizeof(uint32_t));

        switch (point.byte_order) {
        case ByteOrder::DCBA:
            temp = ((temp & 0xFF000000) >> 24) |
                ((temp & 0x00FF0000) >> 8) |
                ((temp & 0x0000FF00) << 8) |
                ((temp & 0x000000FF) << 24);
            break;
        case ByteOrder::BADC:
            temp = ((temp & 0xFF00FF00) >> 8) | ((temp & 0x00FF00FF) << 8);
            break;
        case ByteOrder::CDAB:
            temp = ((temp & 0xFFFF0000) >> 16) | ((temp & 0x0000FFFF) << 16);
            break;
        default:
            break;
        }

        std::memcpy(&raw_value, &temp, sizeof(int32_t));
    }

    double value = raw_value * point.scale + point.offset;
}

// 处理INT64
inline void ModbusDataConverter::to_int64(const DataPoint& point, const std::vector<uint16_t>& regs, size_t idx)
{
    if (idx + 3 >= regs.size()) return;

    int64_t raw_value = MODBUS_GET_INT64_FROM_INT16(regs.data(), idx);
    double value = raw_value * point.scale + point.offset;
}

// 处理UINT64
inline void ModbusDataConverter::to_uint64(const DataPoint& point, const std::vector<uint16_t>& regs, size_t idx)
{
    if (idx + 3 >= regs.size()) return;

    uint64_t raw_value = static_cast<uint64_t>(
        MODBUS_GET_INT64_FROM_INT16(regs.data(), idx));
    double value = raw_value * point.scale + point.offset;
}

// 处理浮点数
inline void ModbusDataConverter::to_float(const DataPoint& point, const std::vector<uint16_t>& regs, size_t idx)
{
    if (idx + 1 >= regs.size()) return;

    float raw_value;
    switch (point.type) {
    case DataType::FLOAT_ABCD:
        raw_value = modbus_get_float_abcd(&regs[idx]);
        break;
    case DataType::FLOAT_DCBA:
        raw_value = modbus_get_float_dcba(&regs[idx]);
        break;
    case DataType::FLOAT_BADC:
        raw_value = modbus_get_float_badc(&regs[idx]);
        break;
    case DataType::FLOAT_CDAB:
        raw_value = modbus_get_float_cdab(&regs[idx]);
        break;
    default:
        raw_value = modbus_get_float(&regs[idx]);
    }

    double value = raw_value * point.scale + point.offset;
}
