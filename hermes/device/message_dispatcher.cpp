#include "message_dispatcher.h"
#include "log/log.h"

MessageDispatcher::MessageDispatcher(int dispatcher_id, ComponentPipeline* pipeline)
    : m_id(dispatcher_id)
    , m_pipeline(pipeline)
    , m_running(false)
    , m_queue(1024)
{
    if (!m_pipeline) {
        throw std::invalid_argument("MessageDispatcher: pipeline cannot be null");
    }
    LOGINFO("MessageDispatcher[%d] created", m_id);
}

MessageDispatcher::~MessageDispatcher()
{
    stop();
    LOGINFO("MessageDispatcher[%d] destroyed", m_id);
}

void MessageDispatcher::start()
{
    if (m_running) {
        LOGWARN("MessageDispatcher[%d]: Already running", m_id);
        return;
    }

    m_running = true;
    m_thread = std::thread(&MessageDispatcher::message_loop, this);
    LOGINFO("MessageDispatcher[%d] started", m_id);
}

void MessageDispatcher::stop()
{
    if (!m_running) return;

    LOGINFO("MessageDispatcher[%d] stopping...", m_id);
    m_running = false;

    // 唤醒线程
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cond.notify_all();
    }

    if (m_thread.joinable()) {
        m_thread.join();
    }

    LOGINFO("MessageDispatcher[%d] stopped", m_id);
}

void MessageDispatcher::push_message(const std::shared_ptr<MessageEnvelope>& msg)
{
    if (!m_running) {
        LOGWARN("MessageDispatcher[%d]: Rejecting message (not running)", m_id);
        return;
    }

    if (!msg) {
        LOGWARN("MessageDispatcher[%d]: Null message", m_id);
        return;
    }

    if (!m_queue.enqueue(msg)) {
        LOGERROR("MessageDispatcher[%d]: Message queue full, dropping message", m_id);
        return;
    }

    // 唤醒消息处理线程
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cond.notify_one();
    }
}

void MessageDispatcher::message_loop()
{
    LOGINFO("MessageDispatcher[%d]: Message loop started", m_id);

    while (m_running)
    {
        std::shared_ptr<MessageEnvelope> msg;

        // 等待消息
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cond.wait(lock, [this] {
                return !m_running || m_queue.size_approx() > 0;
            });
        }

        // 检查 running 状态
        if (!m_running) break;

        // 批量处理消息
        while (m_queue.try_dequeue(msg))
        {
            if (!msg) continue;

            try {
                // 分发到 Pipeline
                m_pipeline->dispatch_message(msg->type(), msg->payload().body());
                
                LOGDEBUG("MessageDispatcher[%d]: Dispatched message type=%d", m_id, msg->type());
            } catch (const std::exception& e) {
                LOGERROR("MessageDispatcher[%d]: Failed to dispatch message: %s", m_id, e.what());
            } catch (...) {
                LOGERROR("MessageDispatcher[%d]: Failed to dispatch message (unknown error)", m_id);
            }
        }
    }

    LOGINFO("MessageDispatcher[%d]: Message loop exited", m_id);
}
