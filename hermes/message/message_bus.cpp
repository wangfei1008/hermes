#include "message_bus.h"
#include "log/log.h"

MessageBus& MessageBus::instance()
{
    static MessageBus instance;
    return instance;
}

MessageBus::MessageBus()
{
}

MessageBus::~MessageBus()
{
    stop();
}

void MessageBus::start()
{
    if (m_running) return;
    m_running = true;
    m_worker_thread = std::thread(&MessageBus::work_thread_func, this);
    LOGINFO("Message Bus Started.");
}

void MessageBus::stop()
{
    m_running = false;
    m_cv.notify_all(); // 唤醒线程让它退出
    if (m_worker_thread.joinable())
        m_worker_thread.join();
}

void MessageBus::subscribe(MessageType type, IMessageObserver* observer)
{
    std::lock_guard<std::mutex> lock(m_sub_mutex);
    m_subscribers[type].push_back(observer);
}

void MessageBus::unsubscribe(MessageType type, IMessageObserver* observer)
{
	std::lock_guard<std::mutex> lock(m_sub_mutex);
	auto it = m_subscribers.find(type);
	if (it != m_subscribers.end())
	{
		auto& vec = it->second;
		vec.erase(std::remove(vec.begin(), vec.end(), observer), vec.end());
	}
}

void MessageBus::push(const std::shared_ptr<MessageEnvelope>& msg)
{
    if (!msg) return;

    // 1. 根据优先级放入对应的队列
    int prio_idx = 0;
    if (msg->priority() == MessagePriority::MP_NORMAL) prio_idx = 1;
    else if (msg->priority() == MessagePriority::EP_HIGH) prio_idx = 2;

    m_queues[prio_idx].enqueue(msg);
    m_pending_count++;

    // 2. 核心改进：立即唤醒消费者
    m_cv.notify_one();
}

void MessageBus::push(MessageType type, std::string info, MessagePriority priority)
{
    // 构造 Message 对象并封装进智能指针
    // 注意：这里需要根据你的 Message 构造函数适配
    MessagePayload pack;
    pack.set_type(type);
    pack.set_package(info);

    auto msg = std::make_shared<MessageEnvelope>(pack, priority);
    push(msg);
}

// 这是新的“心脏”，替代了 message_processor::notity
void MessageBus::work_thread_func()
{
    while (m_running) {
        std::shared_ptr<MessageEnvelope> msg = nullptr;
        bool found = false;

        // --- A. 优先级调度逻辑 ---
        // 总是先检查高优先级队列
        if (m_queues[2].try_dequeue(msg)) found = true;
        else if (m_queues[1].try_dequeue(msg)) found = true;
        else if (m_queues[0].try_dequeue(msg)) found = true;

        if (found && msg) 
        {
            m_pending_count--;

            // --- B. 分发逻辑 ---
            std::map<MessageType, std::vector<IMessageObserver*>> subscribers;
            {
                std::lock_guard<std::mutex> lock(m_sub_mutex);
                subscribers = m_subscribers;
            }
            auto it = subscribers.find(msg->type());
            if (it != subscribers.end()) 
            {
                // 遍历所有订阅者
                for (auto observer : it->second)
                {
                    try {
                        observer->on_message(msg);
                    }
                    catch (...) {
                        LOGERROR("Exception in observer: %s", observer->name().c_str());
                    }
                }
            }
            else {
                // 没订阅者的消息，可以选择打印警告
                LOGWARN("No subscribers for message type: %d", msg->type());
            }
        }
        else {
            // --- C. 等待逻辑 (核心优化) ---
            // 如果三个队列都空了，线程挂起。
            std::unique_lock<std::mutex> lock(m_cv_mutex);
            m_cv.wait(lock, [this] {
                return !m_running || m_pending_count > 0;
                });
        }
    }
}