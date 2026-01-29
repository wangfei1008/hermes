#include "device_stream.h"

DeviceStream::DeviceStream(int stream_id, const std::string& name)
    : m_id(stream_id)
    , m_name(name)
    , m_running(false)
    , m_mutex()
    , m_cond(m_mutex)
{
}

DeviceStream::~DeviceStream()
{
    stop();
}

void DeviceStream::add_component(IComponent* comp) 
{
    if (!m_running) return;

    m_pipeline.push_back(comp);
}


void DeviceStream::start() 
{
    if (m_running) return;
    m_running = true;
    // 启动独立线程：处理该设备所有的逻辑
    m_msg_thread = std::thread(&DeviceStream::message_loop, this);
    m_work_thread = std::thread(&DeviceStream::work_loop, this);
    // 启动各组件业务
    for (auto* comp : m_pipeline)
        comp->start();
}


void DeviceStream::stop() 
{
    if (!m_running) return;

    m_running = false;
    {
        MutexLockGuard lock(m_mutex);
        m_cond.notify();
    }
    if (m_msg_thread.joinable())
        m_msg_thread.join();
    if (m_work_thread.joinable())
        m_work_thread.join();

    for (auto* comp : m_pipeline)
        comp->stop();
}

// 接收来自设备的消息
void DeviceStream::push_message(const std::shared_ptr<MessageEnvelope>& msg) 
{
    if (!m_running) return;

    m_queue.enqueue(msg);
    MutexLockGuard lock(m_mutex);
    m_cond.notify();
}

void DeviceStream::message_loop()
{
    while (m_running) 
    {
        std::shared_ptr<MessageEnvelope> msg;
        {
            MutexLockGuard autoLock(m_mutex);
            // 使用 while 防止虚假唤醒，并增加 !m_running 的退出条件
            while (m_running && m_queue.size_approx() == 0) {
                m_cond.wait();
            }
        }

        // 再次检查 running 状态，因为可能是被 stop 唤醒的
        if (!m_running) break;

        // 处理消息逻辑...
        while (m_queue.try_dequeue(msg))
        {
            for (auto* comp : m_pipeline)
                comp->on_message(msg->type(), msg->payload().body());
        }
    }
}

void DeviceStream::work_loop()
{
    while (m_running)
    {   
        // Pipeline 模式：串行加工
        std::shared_ptr<DataContext> data = std::make_shared<DataContext>();
        // 初始填充（根据 msg 内容转换）
        for (auto* comp : m_pipeline)
        {
            try
            {
                if (!comp->process(data))
                {
                    // 中断流，把数据写数据总线

                    break; 
                } 
            }
            catch (...)
            {
            }               
        }
    } 
}
