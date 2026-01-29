#ifndef I_DATA_HUB_H
#define I_DATA_HUB_H

#include <string>
#include <functional>
#include "data_context.h"

// 订阅者回调定义：当有新的 DataContext 产生时通知
using DataCallback = std::function<void(DataContext::Ptr)>;

class IDataHub 
{
public:
    virtual ~IDataHub() = default;

    // --- 1. 生产者接口：组件流将结果“推”给总线 ---
    virtual void publish(DataContext::Ptr pkg) = 0;

    // --- 2. 消费者接口：UI 或 转发组件“订阅”数据 ---
    // topic 可以是设备 UUID，或者是特定的 DataDomain (如 ALARM)
    virtual uint64_t subscribe(const std::string& topic, DataCallback cb) = 0;
    virtual void unsubscribe(uint64_t sub_id) = 0;

    // --- 3. 同步查询：算法组件查询“其他设备”的最近快照 ---
    virtual DataContext::Ptr get_latest(const std::string& device_uuid) = 0;

    // --- 4. 系统全局变量共享（如系统状态、配置快照） ---
    virtual void set_global_attr(const std::string& key, const wf::Variant& val) = 0;
    virtual wf::Variant get_global_attr(const std::string& key) = 0;
};

#endif