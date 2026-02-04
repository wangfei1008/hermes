#ifndef LIB_MQTT_H
#define LIB_MQTT_H

#include "i_component.h"
#include "mqtt_client.h"
#include "ConCurrentQueue/concurrentqueue.h"
#include "config.h"
#include <thread>
#include <atomic>
#include <memory>

class LibMqtt: public IComponent
{
public:
	LibMqtt();
	~LibMqtt();
    bool init(const DeviceContext& ctx, IDataHub* hub, const std::string& config) override;

    void start() override;
    void pause() override;
    void resume() override;
    void stop() override;

    void on_message(int type, const std::string& msg) override;

    bool process(DataContext::Ptr& pkg) override;
private:
    void worker_loop();

private:
    DeviceContext m_device_context; //设备上下文
    IDataHub* m_data_hub; //数据hub

    MqttClient m_client;
    std::unique_ptr<Config> m_config;

    moodycamel::ConcurrentQueue<DataContext::Ptr> m_queue;
    std::thread m_worker;
    std::atomic<bool> m_running{ false };
    uint64_t m_sub_id = 0;
};


#endif // LIB_MQTT_H