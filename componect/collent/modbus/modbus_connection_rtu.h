#ifndef MODBUS_CONNECTION_RTU_H
#define MODBUS_CONNECTION_RTU_H

#include "imodbus_connection.h"
#include <mutex>

class ModbusConnectionRTU : public IModbusConnection
{
public:
    ModbusConnectionRTU(const std::string& device, int baud, char parity, int data_bit, int stop_bit, int slave_id);

    int read(modbus_function_code code, uint16_t address, uint16_t nb, uint8_t* buf) override;

    int write(modbus_function_code code, uint16_t address, uint16_t nb, uint8_t* buf) override;

protected:
    bool create_contexts() override;
    modbus_t* get_read_context()  const override;
    modbus_t* get_write_context()  const override;
    void disconnect() override;
    bool check_connection_active(modbus_t* ctx) override;

private:
    bool check_serial_device_exists();
#ifdef _WIN32
    bool check_serial_device_windows();
#else
    bool check_serial_device_unix();
#endif
    bool test_rtu_communication(modbus_t* ctx);
    bool test_with_diagnostic_command(modbus_t* ctx);
    bool test_with_read_command(modbus_t* ctx);
    bool test_with_write_command(modbus_t* ctx);

private:
    mutable std::mutex m_mutex;
    modbus_t* m_read_ctx;
    modbus_t* m_write_ctx;

    std::string m_serial_port;
    int m_consecutive_failures;
    static const int MAX_CONSECUTIVE_FAILURES = 3;
};

#endif // MODBUS_CONNECTION_RTU_H
