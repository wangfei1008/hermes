#include "device_proxy.h"
#include "log/log.h"


DeviceProxy::DeviceProxy(const std::string& uuid, const std::string& name)
    : m_uuid(uuid)
    , m_name(name)
{
}

DeviceProxy::~DeviceProxy() 
{ 
    stop();
}

std::string DeviceProxy::name() const
{
    return m_name;
}

//1. 核心逻辑：添加添加一个完整的流，而不是零散的组件
void DeviceProxy::add_stream(std::unique_ptr<ExecutionStream> stream)
{
    if (stream) 
    {
        m_streams.push_back(std::move(stream));
    }
}

//2. 设备启动与停止
void DeviceProxy::start() 
{
    LOGINFO("Starting DeviceProxy: %s", m_name.c_str());
    for (auto& stream : m_streams)
    {
        stream->start();
    }
}

void DeviceProxy::stop()
{
    LOGINFO("Stopping DeviceProxy: %s", m_name.c_str());
    for (auto& stream : m_streams) 
    {
        stream->stop();
    }
}

//3. 消息路由逻辑
void DeviceProxy::on_message(const std::shared_ptr<MessageEnvelope>& msg)
{
    // 权限校验：只处理发给本设备或广播的消息
    if (msg->payload().to().uuid == m_uuid || msg->payload().to().uuid.empty())
    {
        // 核心改进：将消息推送到该设备下的每一个逻辑流中
        // 每个流有自己的 ReaderWriterQueue 和处理线程，互不干扰
        for (auto& stream : m_streams) 
        {
            stream->push_message(msg);
        }
    }
}