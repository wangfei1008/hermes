#ifndef IMESSAGE_OBSERVER_H
#define IMESSAGE_OBSERVER_H

#include <memory>
#include "message_envelope.h"

class IMessageObserver
{
public:
    virtual ~IMessageObserver() = default;

    // 当消息到达时，Bus 会调用这个函数
    // 返回值：保留给未来扩展（例如是否消费掉了消息），目前可忽略
    virtual void on_message(const std::shared_ptr<MessageEnvelope>& msg) = 0;

    // 获取观察者名称（用于调试日志）
    virtual std::string name() const { return "UnknownObserver"; }
};

#endif