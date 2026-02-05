#ifndef MESSAGE_DISPATCHER_H
#define MESSAGE_DISPATCHER_H

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <memory>
#include "ReaderWriterQueue/readerwriterqueue.h"
#include "message_envelope.h"
#include "component_pipeline.h"

/**
 * MessageDispatcher: 消息分发器
 * 职责：
 * 1. 管理消息队列
 * 2. 独立线程处理消息
 * 3. 调用 ComponentPipeline 的 dispatch_message()
 */
class MessageDispatcher
{
public:
    MessageDispatcher(ComponentPipeline* pipeline);
    ~MessageDispatcher();

    // 禁止拷贝
    MessageDispatcher(const MessageDispatcher&) = delete;
    MessageDispatcher& operator=(const MessageDispatcher&) = delete;

    /**
     * 启动消息分发线程
     */
    void start();

    /**
     * 停止消息分发线程
     */
    void stop();

    /**
     * 推送消息到队列
     * @param msg 消息封装
     */
    void push_message(const std::shared_ptr<MessageEnvelope>& msg);

    bool is_running() const { return m_running; }

private:
    void message_loop();

private:
    int m_id;
    ComponentPipeline* m_pipeline;
    
    std::atomic<bool> m_running;
    std::thread m_thread;
    
    moodycamel::ReaderWriterQueue<std::shared_ptr<MessageEnvelope>> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};

#endif // MESSAGE_DISPATCHER_H
