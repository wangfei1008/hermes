#ifndef MODBUS_SCHEDULER_CLIENT_H
#define MODBUS_SCHEDULER_CLIENT_H

#include <memory>
#include <functional>
#include "data/variant.h"
#include "modbus_parameter.h"

class IModbusConnection;

class ModbusSchedulerClient
{
public:
    using ResponseCallback = std::function<void(uint8_t function_code, uint16_t start_address, uint16_t length, const std::vector<uint8_t>& data)>;
    using ErrorCallback = std::function<void(const modbus_request&, const std::string&)>;
    
    ModbusSchedulerClient();

    // 连接管理
    bool connect(std::shared_ptr<IModbusConnection> connect);
	bool reconnect();
	bool is_connected() const;
    void disconnect();

    // 读取参数管理
    int add_read(uint8_t function_code, uint16_t start_address, uint16_t length);
    int optimize_read_groups();
    
    // 写入操作
    void write(uint8_t function_code, uint16_t address, uint16_t length, const uint8_t* values);

    // 回调设置
    void set_data_callback(ResponseCallback callback, ErrorCallback error_callback = nullptr);

    // 调度控制
    void start_scheduler(unsigned int interval_ms = 100);
    void stop_scheduler();
private:
    // 数据处理
	//void data_handler();

    // 回调设置
    //void set_error_callback(std::function<void(const std::string&)> callback);
private:
    std::shared_ptr<IModbusConnection> m_connection;
    std::shared_ptr<class ParameterManager> m_param_manager;
    std::shared_ptr<class DataHandler> m_data_handler;
    std::shared_ptr<class ReadScheduler> m_read_scheduler;
    std::shared_ptr<class WriteScheduler> m_write_scheduler;
};

#endif // MODBUS_SCHEDULER_CLIENT_H
