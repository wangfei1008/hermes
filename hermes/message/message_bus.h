#ifndef MESSAGE_BUS_H
#define MESSAGE_BUS_H

#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include "i_message_observer.h"
#include "ConCurrentQueue/concurrentqueue.h" 

class MessageBus 
{
public:
    // 单例获取
    static MessageBus& instance();

    // 1. 订阅消息
    // observer: 谁想听
    // type: 想听哪种类型的消息
    void subscribe(MessageType type, IMessageObserver* observer);

    // 2. 取消订阅
    void unsubscribe(MessageType type, IMessageObserver* observer);

    // 3. 发布消息 (线程安全，可在任何地方调用)
    void push(const std::shared_ptr<MessageEnvelope>& msg);

    // 简便重载
    void push(MessageType type, std::string info, MessagePriority priority = MessagePriority::MP_NORMAL);

    // 4. 启动/停止 总线分发线程
    void start();
    void stop();

private:
    MessageBus();
    ~MessageBus();

    // 内部工作线程：替代了你原来的 message_processor
    void work_thread_func();

private:
    // 优先级队列组：为了实现真正的优先级，我们使用三个独立的无锁队列
    // index 0: LOW, 1: NORMAL, 2: HIGH
    moodycamel::ConcurrentQueue<std::shared_ptr<MessageEnvelope>> m_queues[3];

    // 订阅者列表：Key=消息类型, Value=观察者列表
    std::map<MessageType, std::vector<IMessageObserver*>> m_subscribers;
    std::mutex m_sub_mutex; // 保护 m_subscribers 的读写

    // 线程同步机制
    std::thread m_worker_thread;
    std::atomic<bool> m_running{ false };
    std::condition_variable m_cv;
    std::mutex m_cv_mutex;
    std::atomic<int> m_pending_count{ 0 }; // 待处理消息总数
};

#endif