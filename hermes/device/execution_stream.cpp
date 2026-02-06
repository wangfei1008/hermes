#include "execution_stream.h"
#include "log/log.h"
#include "data_models/data_hub.h"
#include <string>

ExecutionStream::ExecutionStream(const std::string& device_name, int stream_id, const std::string& name, const std::string& subscribe_topic)
    : m_device_name(device_name)
    , m_id(stream_id)
    , m_name(name)
    , m_subscribe_topic(subscribe_topic.empty() ? std::to_string(stream_id) : subscribe_topic)
{
    // 创建三个子系统
    m_pipeline = std::make_unique<ComponentPipeline>(device_name, stream_id, name);
    m_msg_dispatcher = std::make_unique<MessageDispatcher>(m_pipeline.get());
    m_data_processor = std::make_unique<DataProcessor>(m_pipeline.get());

    // 订阅指定 topic（默认为 stream_id，虚拟设备可配置为 "0" 订阅结果数据）
    m_data_hub_sub_id = DataHub::instance().subscribe(m_subscribe_topic, [this](DataContext::Ptr data) {
        this->push_data(data);
    });

    LOGINFO("[ExecutionStream][%s][%d] stream = %s created, subscribing topic [%s]", m_device_name.c_str(), m_id, m_name.c_str(), m_subscribe_topic.c_str());
}

ExecutionStream::~ExecutionStream()
{
    DataHub::instance().unsubscribe(m_data_hub_sub_id);
    stop();
    LOGINFO("[ExecutionStream][%s][%d] stream = %s destroyed", m_device_name.c_str(), m_id, m_name.c_str());
}

void ExecutionStream::add_component(IComponent* comp, const std::string& lib_name, int order_index, int output_to_next)
{
    m_pipeline->add_component(comp, lib_name, order_index, output_to_next);
}

void ExecutionStream::start()
{
    LOGINFO("[ExecutionStream][%s][%d] stream = %s starting...", m_device_name.c_str(), m_id, m_name.c_str());

    // 1. 启动组件
    m_pipeline->start();

    // 2. 启动消息分发器
    m_msg_dispatcher->start();

    // 3. 启动数据处理器
    m_data_processor->start();

    LOGINFO("[ExecutionStream][%s][%d] stream = %s started with %zu components", m_device_name.c_str(), m_id, m_name.c_str(), m_pipeline->component_count());
}

void ExecutionStream::stop()
{
    LOGINFO("[ExecutionStream][%s][%d] stream = %s stopping...", m_device_name.c_str(), m_id, m_name.c_str());

    // 按相反顺序停止
    // 1. 停止数据处理器
    m_data_processor->stop();

    // 2. 停止消息分发器
    m_msg_dispatcher->stop();

    // 3. 停止组件
    m_pipeline->stop();

    LOGINFO("[ExecutionStream][%s][%d] stream = %s stopped", m_device_name.c_str(), m_id, m_name.c_str());
}

void ExecutionStream::pause()
{
    m_pipeline->pause();
    LOGINFO("[ExecutionStream][%s][%d] stream = %s paused", m_device_name.c_str(), m_id, m_name.c_str());
}

void ExecutionStream::resume()
{
    m_pipeline->resume();
    LOGINFO("[ExecutionStream][%s][%d] stream = %s resumed", m_device_name.c_str(), m_id, m_name.c_str());
}

void ExecutionStream::push_message(const std::shared_ptr<MessageEnvelope>& msg)
{
    m_msg_dispatcher->push_message(msg);
}

void ExecutionStream::push_data(DataContext::Ptr data)
{
    m_data_processor->push_data(data);
}

size_t ExecutionStream::component_count() const
{
    return m_pipeline->component_count();
}
