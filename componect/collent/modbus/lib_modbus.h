#ifndef LIB_MODBUS_H
#define LIB_MODBUS_H

#include "i_component.h"
#include "config.h"
#include "modbus_scheduler_client.h"
#include <thread>
#include <atomic>

class LibModbus : public IComponent
{
public:
    LibModbus();
     ~LibModbus();

    bool init(const DeviceContext& ctx, IDataHub* hub, const std::string& config) override;

    void start() override;
    void pause() override;
    void resume() override;
    void stop() override;

    void on_message(int type, const std::string& msg) override;
    bool process(DataContext::Ptr& pkg) override;

private:
    void worker_loop(); //工作线程

    bool connect();

    //参数管理
    void manager_args();

    // 响应回调
    void response_callback(uint8_t function_code, uint16_t start_address, uint16_t length, const std::vector<uint8_t>& data);
    // 错误回调
    void error_callback(const modbus_request& req, const std::string& error_message);
private:
    DeviceContext m_device_context; //设备上下文
    IDataHub* m_data_hub; //数据hub

    ModbusSchedulerClient m_scheduler_client;     //modbus连接
    std::unique_ptr<Config> m_config; //配置

    std::thread m_worker; //工作线程
    std::atomic<bool> m_running{ false }; //运行状态

    std::atomic<unsigned long> m_frame_index;               //数据帧索引
};

#endif // LIB_MODBUS_H
