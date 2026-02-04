#include "mqtt_client.h"
#include <cstring>
#include "log/log.h"

MqttClient::MqttClient()
{
    m_conn_opts = MQTTClient_connectOptions_initializer;
    m_ssl_opts = MQTTClient_SSLOptions_initializer;

    m_conn_opts.keepAliveInterval = 20;
    m_conn_opts.cleansession = 1;
    m_conn_opts.connectTimeout = 10;
}

MqttClient::~MqttClient()
{
    disconnect();
}

void MqttClient::configure(const std::string& address, const std::string& client_id)
{
    m_address = address;
    m_client_id = client_id;
}

void MqttClient::set_auth(const std::string& user, const std::string& pass)
{
    static std::string u, p; // 保持 C 风格指针有效
    u = user; p = pass;
    m_conn_opts.username = u.c_str();
    m_conn_opts.password = p.c_str();
}

void MqttClient::set_tls(const std::string& trust_store, const std::string& key_store, const std::string& private_key)
{
    m_trust_store = trust_store;
    m_key_store = key_store;
    m_private_key = private_key;

    m_ssl_opts.trustStore = m_trust_store.c_str();
    m_ssl_opts.keyStore = m_key_store.c_str();
    m_ssl_opts.privateKey = m_private_key.c_str();
    m_ssl_opts.enableServerCertAuth = 1;
    m_conn_opts.ssl = &m_ssl_opts;
}

int MqttClient::connect()
{
    if (m_handle) return 0;

    int rc = MQTTClient_create(&m_handle, m_address.c_str(), m_client_id.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS){
        LOGERROR("Failed to create client, return code = %d,mqtt post url = %s", rc, m_address.c_str());
        return rc;
    }

    // 核心：注册静态转发函数
    MQTTClient_setCallbacks(m_handle, this, on_conn_lost, on_msg_arrived, NULL);

    rc = MQTTClient_connect(m_handle, &m_conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        LOGERROR("Failed to connect, return code = %d,mqtt post url = %s", rc, m_address.c_str());
        MQTTClient_destroy(&m_handle);
        m_handle = nullptr;
    }
    return rc;
}

// 静态回调：将 C 的调用转发给 C++ 对象的 std::function
int MqttClient::on_msg_arrived(void* context, char* topicName, int topicLen, MQTTClient_message* message)
{
    auto* self = static_cast<MqttClient*>(context);
    if (self && self->m_msg_handler) {
        std::string payload(static_cast<char*>(message->payload), message->payloadlen);
        std::string topic(topicName);
        self->m_msg_handler(topic, payload);
    }
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void MqttClient::on_conn_lost(void* context, char* cause)
{
    auto* self = static_cast<MqttClient*>(context);
    if (self && self->m_lost_handler) {
        self->m_lost_handler(cause ? cause : "connection dropped");
    }
}

int MqttClient::publish(const std::string& topic, const std::string& payload, int qos, bool retained)
{
    if (!m_handle) return -1;

    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = const_cast<char*>(payload.data());
    pubmsg.payloadlen = (int)payload.length();
    pubmsg.qos = qos;
    pubmsg.retained = retained ? 1 : 0;

    MQTTClient_deliveryToken token;
    int rc = MQTTClient_publishMessage(m_handle, topic.c_str(), &pubmsg, &token);
    if (rc == MQTTCLIENT_SUCCESS && qos > 0) {
        rc = MQTTClient_waitForCompletion(m_handle, token, 5000);
        if (rc != MQTTCLIENT_SUCCESS) {
            LOGERROR("Publish completion timeout, return code = %d", rc);
            return rc;
        }
        LOGINFO("MQTT publish message successful, length = %d", payload.length());
    }
    LOGERROR("MQTT publish message = %s fail, return code = %d", payload.data(), rc);
    return rc;
}

int MqttClient::subscribe(const std::string& topic, int qos)
{
    return MQTTClient_subscribe(m_handle, topic.c_str(), qos);
}

void MqttClient::disconnect()
{
    if (m_handle) {
        MQTTClient_disconnect(m_handle, 100);
        MQTTClient_destroy(&m_handle);
        m_handle = nullptr;
    }
}

bool MqttClient::is_connected() const
{
    return m_handle && MQTTClient_isConnected(m_handle);
}