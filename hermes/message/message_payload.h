#ifndef MESSAGE_PAYLOAD_H
#define MESSAGE_PAYLOAD_H

#include <string>
#include <utility>
#include "message_bus_types.h"

// 更加直观的端点定义
struct Endpoint {
    std::string uuid;
    std::string name;

    // 使用编译器默认生成的拷贝和移动
};

class MessagePayload {
public:
    // 1. 使用 Struct 风格还是 Class 风格？
     // 对于这种只存数据的类，公开成员并不羞耻，反而更清晰。
     // 但为了保持兼容性，我们优化其内部实现。

    MessagePayload() = default; // 默认构造
    ~MessagePayload() = default; // 默认析构

    // 2. 关键优化：编译器自动生成的 拷贝 和 移动
    // 只要成员变量都是可拷贝/可移动的（string 是），编译器生成的版本是最优的。
    MessagePayload(const MessagePayload&) = default;
    MessagePayload& operator=(const MessagePayload&) = default;

    // 显式声明支持移动语义 (Move Semantics) !!! 
    // 这是性能优化的核心
    MessagePayload(MessagePayload&&) noexcept = default;
    MessagePayload& operator=(MessagePayload&&) noexcept = default;

    // 辅助构造函数（支持移动字符串，避免拷贝）
    MessagePayload(MessageType t, std::string pkg_data)
        : m_type(t), m_package(std::move(pkg_data)) {
    }

    // Getters
    const Endpoint& from() const { return m_from; }
    const Endpoint& to() const { return m_to; }
    unsigned long id() const { return m_id; }
    MessageType type() const { return m_type; } // 直接返回 Enum
    const std::string& body() const { return m_package; }

    // Setters (支持链式调用，支持移动优化)
    MessagePayload& set_from(Endpoint ep) { m_from = std::move(ep); return *this; }
    MessagePayload& set_to(Endpoint ep) { m_to = std::move(ep); return *this; }
    MessagePayload& set_id(unsigned long id) { m_id = id; return *this; }
    MessagePayload& set_type(MessageType t) { m_type = t; return *this; }

    // 针对大数据的 Setter 优化
    MessagePayload& set_package(std::string pkg) { m_package = std::move(pkg); return *this; }
    // 返回引用避免拷贝
    const std::string& get_package() const { return m_package; }
private:
    Endpoint m_from;
    Endpoint m_to;
    unsigned long m_id = 0;
    MessageType m_type = MESSAGE_NONE; // 使用强类型，初始化
    std::string m_package;
};

#endif
