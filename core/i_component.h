#ifndef ICOMPONENT_H
#define ICOMPONENT_H
#include "i_data_hub.h"
#include "data_context.h"

struct DeviceContext {
    int device_uuid;   // 设备的唯一ID，对应 MessagePayload 中的 from/to uuid
    std::string device_name;
    int stream_id;             // 关联的任务流ID
    // 该设备特有的静态配置信息
};

class IComponent 
{
public:
    virtual bool init(const DeviceContext& ctx, IDataHub* hub, const std::string& config) = 0;

    virtual void start() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void stop() = 0;    

    virtual void on_message(int type, const std::string& msg) = 0;

    virtual bool process(DataContext::Ptr& pkg) = 0;
};

#endif
