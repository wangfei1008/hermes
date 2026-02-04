#include "lib_mqtt.h"
#include <log4cplus/appender.h>
#include "log/log.h"
#include "component_export.h"
#include <iostream>

extern "C"  COM_EXPORT bool create_lib(IComponent** new_component);
extern "C"  COM_EXPORT bool release_lib(IComponent** new_component);

bool create_lib(IComponent** new_component)
{
    LOGINFO("LibMqtt create component_interface");
    IComponent* lib = new LibMqtt();
    *new_component = (IComponent*)lib;
    return true;
}

bool release_lib(IComponent** new_component)
{
    LOGINFO("LibMqtt release component_interface");
    LibMqtt* component = (LibMqtt*)*new_component;
    delete component;
    component = NULL;
    return true;
}

LibMqtt::LibMqtt()
    : m_running(false)
    , m_sub_id(0)
{
}

LibMqtt::~LibMqtt() 
{
    stop();
}

bool LibMqtt::init(const DeviceContext& ctx, IDataHub* hub, const std::string& config)
{
    if(hub == nullptr){
        LOGERROR("libmqtt init data hub is nullptr");
        return false;
    }
    if(config.empty()){
        LOGERROR("libmqtt init config is empty");
        return false;
    }
    LOGINFO("libmqtt init device name = %s, device id = %d, stream id = %d", ctx.device_name.c_str(), ctx.device_uuid, ctx.stream_id);
    m_device_context = ctx;
    m_data_hub = hub;

	// 1. 解析配置
	m_config = Config::load_from_json(config);
    if (!m_config) {
		LOGERROR("LibMqtt init failed: invalid configuration");
        return false; // 配置加载失败
    }

	// 2. 配置 MQTT 客户端
	m_client.configure(m_config->host, m_config->id);
	m_client.set_auth(m_config->user, m_config->password);
	if (m_config->tls) {
		m_client.set_tls(m_config->ca_crt, m_config->cln_crt, m_config->cln_key);
	}
    LOGINFO("LibMqtt initialized success");
    return true;

}

void LibMqtt::start()
{    
    if (m_running) return;
    m_running = true;
    // 1. 订阅总线数据：回调只负责入队，绝不阻塞
    m_sub_id = m_data_hub->subscribe(DATA_HUB_TOPIC_FORWARD, [this](DataContext::Ptr pkg) {
        if (pkg) m_queue.enqueue(pkg);
        });

    // 2. 启动异步转发线程
    m_worker = std::thread(&LibMqtt::worker_loop, this);

	// 3. 连接 MQTT 服务器
	if (MQTTCLIENT_SUCCESS != m_client.connect()) {
		LOGERROR("LibMqtt start failed: cannot connect to MQTT broker");
        stop();
		return;
	}

    LOGINFO("LibMqtt forward thread started");
}

void LibMqtt::pause()
{
    LOGINFO("libmqtt start");
}

void LibMqtt::resume()
{
    LOGINFO("libmqtt start");
}

void LibMqtt::stop()
{   
    m_client.disconnect();;

    m_running = false;
    if (m_worker.joinable()) {
        m_worker.join();
    }
    if (m_data_hub && m_sub_id > 0) {
        m_data_hub->unsubscribe(m_sub_id);
    }
    LOGINFO("LibMqtt stopped");
}

void LibMqtt::on_message(int type, const std::string& msg)
{
    LOGINFO("libmqtt on_message type=%d, msg=%s", type, msg.c_str());
 }

bool LibMqtt::process(DataContext::Ptr& pkg)
{
    LOGINFO("libmqtt process data frame_index=%lu", pkg->header.frame_index);
    return true;
}

void LibMqtt::worker_loop()
{
    DataContext::Ptr pkg;
    while (m_running) {
        // 2. 从无锁队列取出数据包
        if (m_queue.try_dequeue(pkg)) {
            // 执行 MQTT 发送
            std::string json_data;// = serialize(pkg);
            m_client.publish(m_config->topic, json_data);

            // 3. 将该数据通过流 ID 发布至总线
            // 使用 stream_id 作为 topic
            m_data_hub->publish(std::to_string(m_device_context.stream_id), pkg);
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}