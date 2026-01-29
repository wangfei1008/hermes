#include "execution_stream.h"
#include "log/log.h"
#include "data_models/data_hub.h"

ExecutionStream::ExecutionStream(int stream_id, const std::string& name)
    : m_id(stream_id)
    , m_name(name)
{
    // 创建三个子系统
    m_pipeline = std::make_unique<ComponentPipeline>(stream_id, name);
    m_msg_dispatcher = std::make_unique<MessageDispatcher>(stream_id, m_pipeline.get());
    m_data_processor = std::make_unique<DataProcessor>(stream_id, m_pipeline.get());

    DataHub::instance().subscribe(stream_id, [this](DataContext::Ptr data) {
        this->push_data(data); 
        });

    LOGINFO("ExecutionStream[%d:%s] created", m_id, m_name.c_str());
}

ExecutionStream::~ExecutionStream()
{
    stop();
    LOGINFO("ExecutionStream[%d:%s] destroyed", m_id, m_name.c_str());
}

void ExecutionStream::add_component(IComponent* comp, const std::string& lib_name)
{
    m_pipeline->add_component(comp, lib_name);
}

void ExecutionStream::start()
{
    LOGINFO("ExecutionStream[%d:%s] starting...", m_id, m_name.c_str());

    // 1. 启动组件
    m_pipeline->start();

    // 2. 启动消息分发器
    m_msg_dispatcher->start();

    // 3. 启动数据处理器
    m_data_processor->start();

    LOGINFO("ExecutionStream[%d:%s] started with %zu components", m_id, m_name.c_str(), m_pipeline->component_count());
}

void ExecutionStream::stop()
{
    LOGINFO("ExecutionStream[%d:%s] stopping...", m_id, m_name.c_str());

    // 按相反顺序停止
    // 1. 停止数据处理器
    m_data_processor->stop();

    // 2. 停止消息分发器
    m_msg_dispatcher->stop();

    // 3. 停止组件
    m_pipeline->stop();

    LOGINFO("ExecutionStream[%d:%s] stopped", m_id, m_name.c_str());
}

void ExecutionStream::pause()
{
    m_pipeline->pause();
    LOGINFO("ExecutionStream[%d:%s] paused", m_id, m_name.c_str());
}

void ExecutionStream::resume()
{
    m_pipeline->resume();
    LOGINFO("ExecutionStream[%d:%s] resumed", m_id, m_name.c_str());
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
