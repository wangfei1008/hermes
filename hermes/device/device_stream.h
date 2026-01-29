#ifndef DEVICE_STREAM_H
#define DEVICE_STREAM_H

#include <thread>
#include <vector>
#include <string>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "ReaderWriterQueue/readerwriterqueue.h"
#include "../core/i_component.h"
#include "message_envelope.h"
#include "thread/Condition.h"
#include "thread/MutexLock.h"

class DeviceStream
{
public:
    DeviceStream(int stream_id, const std::string& name);
    ~DeviceStream();

    void add_component(IComponent* comp);

    void start();

    void stop();

    // 接收来自设备的消息
    void push_message(const std::shared_ptr<MessageEnvelope>& msg);

private:
    void message_loop();
    void work_loop();

private:
    int m_id;
    std::string m_name;
    std::atomic<bool> m_running;
    std::thread m_msg_thread;
    std::thread m_work_thread;
    std::vector<IComponent*> m_pipeline;
    moodycamel::ReaderWriterQueue<std::shared_ptr<MessageEnvelope>> m_queue;
    MutexLock m_mutex;
    Condition m_cond;
};

#endif
