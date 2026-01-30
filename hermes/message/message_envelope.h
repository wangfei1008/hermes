#ifndef MESSAGE_ENVELOPE_H
#define MESSAGE_ENVELOPE_H

#include "message_payload.h"

class MessageEnvelope 
{
public:
    // 构造函数通过 std::move 接收载荷，实现“零拷贝”入队
    MessageEnvelope(MessagePayload payload, MessagePriority priority = MP_NORMAL)
        : m_payload(std::move(payload))
        , m_priority(priority)
    {
    }

    // 默认析构和拷贝，保持简洁
    ~MessageEnvelope() = default;

    MessageType type() const { return static_cast<MessageType>(m_payload.type()); }
    MessagePriority priority() const { return m_priority; }

    // 获取载荷引用
    const MessagePayload& payload() const { return m_payload; }

private:
    MessagePayload m_payload;
    MessagePriority m_priority;
};

#endif