#ifndef DEVICE_PROXY_H
#define DEVICE_PROXY_H

#include "message/i_message_observer.h"
#include "device_stream.h"

class DeviceProxy: public IMessageObserver
{
public:
    DeviceProxy(const std::string& uuid, const std::string& name);

    ~DeviceProxy();

    // 1.核心逻辑：添加添加一个完整的流，而不是零散的组件
    void add_stream(std::unique_ptr<DeviceStream> stream);

    // 2. 设备启动与停止
    void start();
    void stop();

    //3. 消息路由逻辑
    virtual void on_message(const std::shared_ptr<MessageEnvelope>& msg) override;

    virtual std::string name() const override;

private:
    std::string m_uuid;
    std::string m_name;
    std::vector<std::unique_ptr<DeviceStream>> m_streams;
};

#endif //DEVICE_PROXY_H