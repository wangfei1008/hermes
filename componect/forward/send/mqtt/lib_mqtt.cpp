#include "lib_mqtt.h"
#include <log4cplus/appender.h>
#include "log/log.h"
#include "component_export.h"
#include <iostream>



void DebugModbusLogger() {
    // ��ȡ��ǰ modbus �����ĵ���
    auto& mgr = LoggerManager::instance();
    log4cplus::Logger logger = mgr.logger();

    std::cout << "\n=== Modbus Logger Debug Info ===" << std::endl;
    std::cout << "Logger Name: " << logger.getName() << std::endl;

    // ���ֱ�ӹ��ص� Appender
    log4cplus::SharedAppenderPtrList appenders = logger.getAllAppenders();
    std::cout << "Direct Appender Count: " << appenders.size() << std::endl;

    // ����Ƿ��������ϲ㣨Root��������־
    std::cout << "Additivity: " << (logger.getAdditivity() ? "True" : "False") << std::endl;

    // �ص㣺��� Root Logger����Ϊ��ĵ�����ʼ��ͨ������� Root ��
    log4cplus::Logger root = log4cplus::Logger::getRoot();
    log4cplus::SharedAppenderPtrList rootAppenders = root.getAllAppenders();
    std::cout << "Root Appender Count: " << rootAppenders.size() << std::endl;

    if (rootAppenders.empty() && appenders.empty()) {
        std::cout << "RESULT: [FAIL] No appenders found in Modbus context!" << std::endl;
    }
    else {
        std::cout << "RESULT: [OK] Appenders are present." << std::endl;
    }
    std::cout << "================================\n" << std::endl;
}

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
{
    m_mqtt_client = std::make_unique<MqttClient>();
    m_receive_buffer = std::make_unique<DataBuffer>();
    m_send_buffer = std::make_unique<DataBuffer>();
    m_bus_adapter = std::make_unique<BusAdapter>();
}

LibMqtt::~LibMqtt() 
{
    stop();
}
bool LibMqtt::init(const DeviceContext& ctx, IDataHub* hub, const std::string& config)
{
    DebugModbusLogger();
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
    m_config = config;

        // 1. 解析配置
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(config, root)) {
        LOGERROR("Failed to parse MQTT config");
        return false;
    }
    
    m_mqtt_config.broker = root["broker"].asString();
    m_mqtt_config.port = root["port"].asInt();
    m_mqtt_config.client_id = root["client_id"].asString();
    m_mqtt_config.publish_topic = root["publish_topic"].asString();
    m_mqtt_config.subscribe_topic = root["subscribe_topic"].asString();
    m_mqtt_config.qos = root["qos"].asInt();
    
    // 2. 初始化总线适配器
    if (!m_bus_adapter->initialize(hub, ctx.stream_id)) {
        LOGERROR("Failed to initialize bus adapter");
        return false;
    }
    
    // 3. 订阅总线数据（使用新的回调定义）
    m_bus_adapter->subscribe("data_forward", this);
    
    LOGINFO("LibMqtt initialized: device=%s, broker=%s:%d",  ctx.device_name.c_str(),  m_mqtt_config.broker.c_str(), m_mqtt_config.port);
    return true;
}

void LibMqtt::start()
{
        if (m_running) return;
    
    m_running = true;
    
    // 1. 连接MQTT服务器
    if (!m_mqtt_client->connect(m_mqtt_config.broker, m_mqtt_config.port)) {
        LOGERROR("Failed to connect to MQTT broker");
        return;
    }
    
    // 2. 订阅MQTT主题
    m_mqtt_client->subscribe(m_mqtt_config.subscribe_topic,
                            [this](const std::string& topic, const std::string& payload) {
                                this->on_mqtt_message(topic, payload);
                            },
                            m_mqtt_config.qos);
    
    // 3. 启动处理线程
    m_processing_thread = std::thread(&LibMqtt::processing_thread, this);
    m_mqtt_thread = std::thread(&LibMqtt::mqtt_publish_thread, this);
    
    LOGINFO("LibMqtt started");
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
    if (!m_running) return;
    
    m_running = false;
    
    // 停止线程
    if (m_processing_thread.joinable()) {
        m_processing_thread.join();
    }
    if (m_mqtt_thread.joinable()) {
        m_mqtt_thread.join();
    }
    
    // 断开MQTT连接
    m_mqtt_client->disconnect();
    
    LOGINFO("LibMqtt stopped");
}

void LibMqtt::on_message(int type, const std::string& msg)
{
    LOGINFO("libmqtt on_message type=%d, msg=%s", type, msg.c_str());
    // 轻量级处理：仅做数据转换和入队
    auto data = m_bus_adapter->deserialize_data(msg);
    if (data) {
        // 放入发送缓冲队列（无锁，不影响总线性能）
        m_send_buffer->push(data);
    }
}

bool LibMqtt::process(DataContext::Ptr& pkg)
{
    LOGINFO("libmqtt process data frame_index=%lu", pkg->header.frame_index);
    return true;
}

// MQTT消息回调
void LibMqtt::on_mqtt_message(const std::string& topic, const std::string& payload) {
    // 将MQTT消息转换为DataContext
    DataContext::Ptr data = std::make_shared<DataContext>();
    data->data = payload;
    
    // 放入接收缓冲队列
    m_receive_buffer->push(data);
}

// 数据处理线程
void LibMqtt::processing_thread() {
    while (m_running) {
        DataContext::Ptr data;
        if (m_receive_buffer->try_pop(data)) {
            // 处理接收到的数据（从MQTT来的）
            if (process(data)) {
                // 处理成功后发布到总线
                m_bus_adapter->publish_to_stream(data);
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

// MQTT发布线程
void LibMqtt::mqtt_publish_thread() {
    while (m_running) {
        DataContext::Ptr data;
        if (m_send_buffer->try_pop(data)) {
            // 发布到MQTT
            std::string payload = data->data; // 简化处理
            m_mqtt_client->publish(m_mqtt_config.publish_topic, payload, m_mqtt_config.qos);
            
            // 同时发布到总线（可选）
            m_bus_adapter->publish_to_stream(data);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}