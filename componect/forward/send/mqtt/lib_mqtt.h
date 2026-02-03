#ifndef LIB_MQTT_H
#define LIB_MQTT_H

#include "i_component.h"
#include "mqtt_client.h"
#include "data_buffer.h"
#include "bus_adapter.h"
#include <thread>
#include <atomic>
#include <memory>

class LibMqtt: public IComponent
{
public:
    bool init(const DeviceContext& ctx, IDataHub* hub, const std::string& config) override;

    void start() override;
    void pause() override;
    void resume() override;
    void stop() override;

    void on_message(int type, const std::string& msg) override;

    bool process(DataContext::Ptr& pkg) override;
private:
    DeviceContext m_device_context; //设备上下文
    IDataHub* m_data_hub; //数据hub
    std::string m_config; //配置
private:
    // 内部工作线程函数
    void processing_thread();
    void mqtt_publish_thread();
    
    // MQTT消息回调
    void on_mqtt_message(const std::string& topic, const std::string& payload);
    
private:
    DeviceContext m_device_context;
    std::string m_config;
    
    // 各独立模块
    std::unique_ptr<MqttClient> m_mqtt_client;
    std::unique_ptr<DataBuffer> m_receive_buffer;  // 接收数据缓冲
    std::unique_ptr<DataBuffer> m_send_buffer;     // 发送数据缓冲
    std::unique_ptr<BusAdapter> m_bus_adapter;
    
    // 工作线程
    std::thread m_processing_thread;
    std::thread m_mqtt_thread;
    std::atomic<bool> m_running{false};
    
    // 配置参数
    struct MqttConfig {
        std::string broker;
        int port{1883};
        std::string client_id;
        std::string publish_topic;
        std::string subscribe_topic;
        int qos{0};
    } m_mqtt_config;
};


#endif // LIB_MQTT_H