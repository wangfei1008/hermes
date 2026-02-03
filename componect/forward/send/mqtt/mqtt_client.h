#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <string>
#include <functional>

class MqttClient {
public:
    using MessageCallback = std::function<void(const std::string& topic, const std::string& payload)>;
    
    MqttClient() = default;
    ~MqttClient();
    
    bool connect(const std::string& broker, int port = 1883);
    void disconnect();
    bool publish(const std::string& topic, const std::string& payload, int qos = 0);
    bool subscribe(const std::string& topic, MessageCallback callback, int qos = 0);
    bool is_connected() const { return m_connected; }
    
private:
    void* m_client{nullptr};  // 实际MQTT客户端指针
    bool m_connected{false};
    std::string m_client_id;
};

#endif // MQTT_CLIENT_H