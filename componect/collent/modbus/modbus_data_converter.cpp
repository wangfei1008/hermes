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
