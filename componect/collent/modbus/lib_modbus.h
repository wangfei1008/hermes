#ifndef LIB_MODBUS_H
#define LIB_MODBUS_H

#include "i_component.h"
#include "config.h"
#include "modbus_scheduler_client.h"

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
    //void worker_loop(); //工作线程

    int connect();
    int close();

    //往缓存写异常流程输出
    void write_targetdata(int value, double timestamp, const string& config);
    //往缓存写正常采集原始数据
    void write_rawdata(const std::vector<uint8_t>& buffer, double timestamp, const string& config);

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

    //std::thread m_worker; //工作线程
    //std::atomic<bool> m_running{ false }; //运行状态

    bool m_open;
    //
    //bool m_sendstartcommand;                       //执行服务器连接命令，即只执行一次部分，每次重新连接服务器需要再次发送

    //CConfig* m_p_config;                            //配置文件信息
    //ComponentType m_compenenttype;                 //插件类型
    //CCPUTimer m_sendlooptimer;                     //发送loop命令的定时器
    //data_flow* m_p_data;                            //流程内的数据缓存
};

#endif // LIB_MODBUS_H
