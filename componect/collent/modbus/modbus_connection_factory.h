#ifndef MODBUS_CONNECTION_FACTORY_H
#define MODBUS_CONNECTION_FACTORY_H

#include "imodbus_connection.h"
#include <memory>

class ModbusConnectionCreator
{
public:
	ModbusConnectionCreator() = default;
	virtual ~ModbusConnectionCreator() = default;
	virtual std::unique_ptr<IModbusConnection> create(modbus_conn* conn) const = 0;
};

class ModbusConnectionConfigFactory : public ModbusConnectionCreator
{
public:
	ModbusConnectionConfigFactory() = default;
	virtual ~ModbusConnectionConfigFactory() = default;
	std::unique_ptr<IModbusConnection> create(modbus_conn* conn = nullptr) const override;
};

class ModbusConnectionParamFactory : public ModbusConnectionCreator
{
public:
	ModbusConnectionParamFactory() = default;
	virtual ~ModbusConnectionParamFactory() = default;
	std::unique_ptr<IModbusConnection> create(modbus_conn* conn = nullptr) const override;
};

class ModbusConnectionFactory
{
public:
    static std::unique_ptr<IModbusConnection> create(modbus_conn* conn = nullptr);

};

#endif // MODBUS_CONNECTION_FACTORY_H
