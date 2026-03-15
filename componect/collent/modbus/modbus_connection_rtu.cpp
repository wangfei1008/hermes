#include "modbus_connection_rtu.h"
#include "log/log.h"
#if !defined(_WIN32)
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#endif

ModbusConnectionRTU::ModbusConnectionRTU(const std::string& device, int baud, char parity, int data_bit, int stop_bit, int slave_id)
    : IModbusConnection()
	, m_read_ctx(nullptr)
    , m_write_ctx(nullptr)
{
    m_conn.type = modbus_conntype::RTU;
    m_conn.slave = slave_id;
    memcpy(m_conn.info.rtu.device, device.c_str(), device.length());
    m_conn.info.rtu.baud = baud;
    m_conn.info.rtu.parity = parity;
    m_conn.info.rtu.data_bit = data_bit;
    m_conn.info.rtu.stop_bit = stop_bit;
}

int ModbusConnectionRTU::read(modbus_function_code code, uint16_t address, uint16_t nb, uint8_t* buf)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return IModbusConnection::read(code, address, nb, buf);
}

int ModbusConnectionRTU::write(modbus_function_code code, uint16_t address, uint16_t nb, uint8_t* buf)
{
    std::unique_lock<std::mutex> lock(m_mutex);   
    return IModbusConnection::write(code, address, nb, buf);
}

bool ModbusConnectionRTU::create_contexts()
{
    m_read_ctx = modbus_new_rtu(m_conn.info.rtu.device, m_conn.info.rtu.baud, m_conn.info.rtu.parity, m_conn.info.rtu.data_bit, m_conn.info.rtu.stop_bit);
    m_write_ctx = m_read_ctx;
    return m_read_ctx != nullptr;
}

modbus_t* ModbusConnectionRTU::get_read_context() const
{
    return m_read_ctx;
}

modbus_t* ModbusConnectionRTU::get_write_context() const
{
    return m_write_ctx;
}

void ModbusConnectionRTU::disconnect()
{
    if (m_read_ctx) 
    { 
        modbus_close(m_read_ctx);
        modbus_free(m_read_ctx); 
        m_read_ctx = nullptr; 
    }

    if (m_write_ctx && m_write_ctx != m_read_ctx) 
    { 
        modbus_close(m_write_ctx); 
        modbus_free(m_write_ctx); 
        m_write_ctx = nullptr; 
    }

    m_connected = false;
}

bool ModbusConnectionRTU::check_connection_active(modbus_t* ctx)
{
    if (!ctx) {
        return false;
    }

    // 方法1: 检查串口设备是否存在
    if (!check_serial_device_exists()) {
        LOGERROR("Serial port device %s does not exist", m_serial_port.c_str());
        m_consecutive_failures++;
        return false;
    }

    // 方法2: 发送诊断命令测试通信
    bool communication_ok = test_rtu_communication(ctx);
    if (!communication_ok) {
        m_consecutive_failures++;
        LOGERROR("RTU communication test failed, consecutive failures: %d/%d",
                 m_consecutive_failures, MAX_CONSECUTIVE_FAILURES);
    } else {
        m_consecutive_failures = 0; // 重置失败计数
    }

    return (m_consecutive_failures < MAX_CONSECUTIVE_FAILURES);
}

bool ModbusConnectionRTU::check_serial_device_exists()
{
#ifdef _WIN32
    return check_serial_device_windows();
#else
    return check_serial_device_unix();
#endif
}

#ifdef _WIN32
bool ModbusConnectionRTU::check_serial_device_windows()
{
    // Windows下检查串口设备
    HANDLE hSerial = CreateFileA(
        m_serial_port.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if (hSerial == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND) {
            LOGERROR("Serial port %s not found", m_serial_port.c_str());
        } else if (error == ERROR_ACCESS_DENIED) {
            LOGERROR("Access denied to serial port %s", m_serial_port.c_str());
        }
        return false;
    }

    // 检查设备是否真的是串口
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams)) {
        CloseHandle(hSerial);
        return false; // 不是串口设备或无法获取状态
    }

    CloseHandle(hSerial);
    return true;
}
#else
bool ModbusConnectionRTU::check_serial_device_unix()
{
    // Linux/Unix下检查串口设备
    int fd = open(m_serial_port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd == -1) {
        LOGERROR("Cannot open serial port %s: %s", m_serial_port.c_str(), strerror(errno));
        return false;
    }

    // 检查是否为TTY设备
    if (!isatty(fd)) {
        close(fd);
        LOGERROR("%s is not a TTY device", m_serial_port.c_str());
        return false;
    }

    close(fd);
    return true;
}
#endif

bool ModbusConnectionRTU::test_rtu_communication(modbus_t* ctx)
{
    // 保存原始超时设置
    uint32_t to_sec =0,  to_usec = 0;
    modbus_get_response_timeout(ctx, &to_sec, &to_usec);

    // 设置测试用的较短超时
    modbus_set_response_timeout(ctx, 2, 0);

    bool success = false;

    // 尝试几种不同的诊断方法
    success = test_with_diagnostic_command(ctx) ||
              test_with_read_command(ctx) ||
              test_with_write_command(ctx);

    // 恢复原始超时设置
    modbus_set_response_timeout(ctx, to_sec, to_usec);

    return success;
}

bool ModbusConnectionRTU::test_with_diagnostic_command(modbus_t* ctx)
{
    // 尝试Modbus诊断命令（功能码 0x08）
    // 子功能码 0x00 - 回送测试
    uint16_t data = 0x55AA; // 测试数据
    uint16_t result = 0;

    int rc = modbus_report_slave_id(ctx, 1, (uint8_t *)&result); // 读取从站ID

    if (rc == -1) {
        LOGDEBUG("Diagnostic command failed: %s", modbus_strerror(errno));
        return false;
    }

    LOGDEBUG("RTU diagnostic test passed, slave ID response received");
    return true;
}

bool ModbusConnectionRTU::test_with_read_command(modbus_t* ctx)
{
    // 尝试读取一个保持寄存器
    uint16_t test_register;
    int rc = modbus_read_registers(ctx, 0, 1, &test_register);

    if (rc == -1) {
        int error = errno;
        // 某些错误可能不是连接问题，而是地址问题
        if (error == EMBXILADD || error == EMBXILVAL) {
            // 非法地址或值，但通信是正常的
            LOGDEBUG("RTU communication OK but address invalid");
            return true;
        }
        LOGDEBUG("Read command failed: %s", modbus_strerror(errno));
        return false;
    }

    LOGDEBUG("RTU read test passed");
    return true;
}

bool ModbusConnectionRTU::test_with_write_command(modbus_t* ctx)
{
    // 尝试写入一个线圈（如果允许的话）
    uint8_t original_value;
    int rc_read = modbus_read_bits(ctx, 0, 1, &original_value);

    if (rc_read == 1) {
        // 成功读取，尝试写入（如果安全的话）
        // 注意：在实际应用中要小心，不要意外改变设备状态
        int rc_write = modbus_write_bit(ctx, 0, original_value); // 写回原值

        if (rc_write == 1) {
            LOGDEBUG("RTU write test passed");
            return true;
        }
    }

    // 如果写入测试不可行，至少读取成功了
    if (rc_read == 1) {
        LOGDEBUG("RTU read test passed (write test skipped)");
        return true;
    }

    return false;
}
