#include "modbus_connection_factory.h"
#include "modbus_connection_rtu.h"
#include "modbus_connection_tcp.h"

#ifdef USING_CONFIG_FILE_TO_CREAT_MODBUS
#include "config_path.h"
#endif // USING_CONFIG_FILE_TO_CREAT_MODBUS

#ifdef USING_PARAM_TO_CREAT_MODBUS
#include "modbus_parameter_manager.h"
#endif

#define USING_TRANS_PARAM_TO_CREAT_MODBUS 1

std::unique_ptr<IModbusConnection> ModbusConnectionConfigFactory::create(modbus_conn* conn) const
{
#ifdef USING_CONFIG_FILE_TO_CREAT_MODBUS
    if (CConfig::instance()->get_connectType() == modbus_conntype::IP_TCP)
        return std::make_unique<ModbusConnectionTCP>(CConfig::instance()->get_deviceIP().c_str(), CConfig::instance()->get_devicePort(), CConfig::instance()->get_deviceSlaveID());

    if (CConfig::instance()->get_connectType() == modbus_conntype::RTU)
        return std::make_unique<ModbusConnectionRTU>(CConfig::instance()->get_deviceName().c_str(), CConfig::instance()->get_deviceBaud(), \
            (char)*(CConfig::instance()->get_deviceParity().c_str()), CConfig::instance()->get_deviceDatabit(), \
            CConfig::instance()->get_deviceStopbit(), CConfig::instance()->get_deviceSlaveID());
#endif
    throw std::runtime_error("Unsupported connection type");
}

std::unique_ptr<IModbusConnection> ModbusConnectionParamFactory::create(modbus_conn* conn) const
{
#ifdef USING_PARAM_TO_CREAT_MODBUS
    if (ModbusParameterManager::instance()->conn_info().type == modbus_conntype::IP_TCP)
        return std::make_unique<ModbusConnectionTCP>(  ModbusParameterManager::instance()->conn_info().info.tcp.ip\
                                                     , ModbusParameterManager::instance()->conn_info().info.tcp.port\
                                                     , ModbusParameterManager::instance()->conn_info().slave);
    if (ModbusParameterManager::instance()->conn_info().type == modbus_conntype::RTU)
        return std::make_unique<ModbusConnectionRTU>(  ModbusParameterManager::instance()->conn_info().info.rtu.device\
                                                     , ModbusParameterManager::instance()->conn_info().info.rtu.baud\
                                                     , ModbusParameterManager::instance()->conn_info().info.rtu.parity\
                                                     , ModbusParameterManager::instance()->conn_info().info.rtu.data_bit\
                                                     , ModbusParameterManager::instance()->conn_info().info.rtu.stop_bit\
                                                     , ModbusParameterManager::instance()->conn_info().slave);
#endif
    throw std::runtime_error("Unsupported connection type");
}

std::unique_ptr<IModbusConnection> ModbusConnectionFactory::create(modbus_conn* conn)
{
#ifdef USING_CONFIG_FILE_TO_CREAT_MODBUS
    if (CConfig::instance()->get_connectType() == modbus_conntype::IP_TCP)
        return std::make_unique<ModbusConnectionTCP>(CConfig::instance()->get_deviceIP().c_str(), CConfig::instance()->get_devicePort(), CConfig::instance()->get_deviceSlaveID());

    if (CConfig::instance()->get_connectType() == modbus_conntype::RTU)
        return std::make_unique<ModbusConnectionRTU>(CConfig::instance()->get_deviceName().c_str(), CConfig::instance()->get_deviceBaud(), \
            (char)*(CConfig::instance()->get_deviceParity().c_str()), CConfig::instance()->get_deviceDatabit(), \
            CConfig::instance()->get_deviceStopbit(), CConfig::instance()->get_deviceSlaveID());
#elif USING_PARAM_TO_CREAT_MODBUS
    if (ModbusParameterManager::instance()->conn_info().type == modbus_conntype::IP_TCP)
        return std::make_unique<ModbusConnectionTCP>(  ModbusParameterManager::instance()->conn_info().info.tcp.ip\
                                                     , ModbusParameterManager::instance()->conn_info().info.tcp.port\
                                                     , ModbusParameterManager::instance()->conn_info().slave);
    if (ModbusParameterManager::instance()->conn_info().type == modbus_conntype::RTU)
        return std::make_unique<ModbusConnectionRTU>(  ModbusParameterManager::instance()->conn_info().info.rtu.device\
                                                     , ModbusParameterManager::instance()->conn_info().info.rtu.baud\
                                                     , ModbusParameterManager::instance()->conn_info().info.rtu.parity\
                                                     , ModbusParameterManager::instance()->conn_info().info.rtu.data_bit\
                                                     , ModbusParameterManager::instance()->conn_info().info.rtu.stop_bit\
                                                     , ModbusParameterManager::instance()->conn_info().slave);
#elif USING_TRANS_PARAM_TO_CREAT_MODBUS
	if (conn == nullptr)
		throw std::invalid_argument("Connection parameters cannot be null");
	if (conn->type == modbus_conntype::IP_TCP)
		return std::make_unique<ModbusConnectionTCP>(conn->info.tcp.ip, conn->info.tcp.port, conn->slave);
	if (conn->type == modbus_conntype::RTU)
		return std::make_unique<ModbusConnectionRTU>(conn->info.rtu.device, conn->info.rtu.baud, conn->info.rtu.parity, conn->info.rtu.data_bit, conn->info.rtu.stop_bit, conn->slave);
#endif
    throw std::runtime_error("Unsupported connection type");
}
