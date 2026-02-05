#ifndef IMODBUS_CONNECTION_H
#define IMODBUS_CONNECTION_H

#include "modbus_parameter.h"
#include <modbus.h>

class IModbusConnection
{
public:
    IModbusConnection();
    ~IModbusConnection();

    bool connect();
    
    virtual void disconnect() = 0;

    virtual bool is_connected() const;

    virtual int read(modbus_function_code code, uint16_t address, uint16_t nb, uint8_t* buf);

    virtual int write(modbus_function_code code, uint16_t address, uint16_t nb, uint8_t* buf);

protected:
    virtual bool create_contexts() = 0;
    virtual modbus_t* get_read_context() const = 0;
    virtual modbus_t* get_write_context() const = 0;
    virtual bool check_connection_active(modbus_t* ctx) = 0;

private:
    int reconnect();
    void set_slave();

protected:
    bool m_connected;
    modbus_conn m_conn;
};

#endif // IMODBUS_CONNECTION_H
