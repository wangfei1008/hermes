#ifndef MODBUS_CONNECTION_TCP_H
#define MODBUS_CONNECTION_TCP_H
#include "imodbus_connection.h"

class ModbusConnectionTCP : public IModbusConnection
{
public:
    ModbusConnectionTCP(const std::string& ip, int port, int slave_id);

protected:
    bool create_contexts() override;
    modbus_t* get_read_context() const override;
    modbus_t* get_write_context() const override;
    void disconnect() override;
    bool check_connection_active(modbus_t* ctx) override;

private:
    void setup_keepalive(modbus_t* ctx);
    void set_reuse_addr(modbus_t* ctx);
    void close_sokcet(int socket_fd);
#ifdef _WIN32
    bool is_windows_connection_error(int error_code);
#else
    bool is_unix_connection_error(int error_code);
#endif

private:
    modbus_t* m_read_ctx;
    modbus_t* m_write_ctx;
};

#endif // MODBUS_CONNECTION_TCP_H
