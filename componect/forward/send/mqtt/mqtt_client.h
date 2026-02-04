#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "MQTTClient.h"
#include <string>
#include <functional>
#include <memory>

class MqttClient
{
public:
    // 定义现代 C++ 风格的回调
    using MessageHandler = std::function<int(std::string topic, std::string payload)>;
    using ConnectionLostHandler = std::function<void(const std::string& cause)>;

    MqttClient();
    ~MqttClient();

    // 严禁拷贝，防止 handle 重复销毁
    MqttClient(const MqttClient&) = delete;
    MqttClient& operator=(const MqttClient&) = delete;

    // 核心配置：通过 URI 自动判断是 TCP 还是 SSL
    void configure(const std::string& address, const std::string& client_id);
    void set_auth(const std::string& user, const std::string& pass);
    void set_tls(const std::string& trust_store, const std::string& key_store, const std::string& private_key);

    // 回调绑定
    void on_message(MessageHandler handler) { m_msg_handler = handler; }
    void on_connection_lost(ConnectionLostHandler handler) { m_lost_handler = handler; }

    // 操作接口
    int connect();
    void disconnect();
    int publish(const std::string& topic, const std::string& payload, int qos = 1, bool retained = false);
    int subscribe(const std::string& topic, int qos = 1);

    bool is_connected() const;

private:
    // Paho C 要求的静态回调中转
    static int on_msg_arrived(void* context, char* topicName, int topicLen, MQTTClient_message* message);
    static void on_conn_lost(void* context, char* cause);

private:
    MQTTClient m_handle = nullptr;
    std::string m_address;
    std::string m_client_id;

    MQTTClient_connectOptions m_conn_opts;
    MQTTClient_SSLOptions m_ssl_opts;

    // 用于持有 TLS 路径字符串的生命周期
    std::string m_trust_store, m_key_store, m_private_key;

    MessageHandler m_msg_handler;
    ConnectionLostHandler m_lost_handler;
};

#endif