#ifndef DATA_HUB_H
#define DATA_HUB_H

#include "i_data_hub.h"
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <vector>

/**
 * DataHub: 数据中心实现
 * 职责：
 * 1. 数据发布/订阅
 * 2. 数据缓存查询
 * 3. 全局属性管理
 */
class DataHub : public IDataHub
{
public:
    DataHub();
    ~DataHub() override;

    // 单例获取
    static DataHub& instance();

    void create();

    DataHub* get();

    // --- 1. 生产者接口：组件流将结果"推"给总线 ---
    // topic: 流 ID 发给对应流；"0" 发给总转发
    void publish(const std::string& topic, DataContext::Ptr pkg) override;
    void publish(DataContext::Ptr pkg) override;

    // --- 2. 消费者接口：UI 或转发组件"订阅"数据 ---
    uint64_t subscribe(const std::string& topic, DataCallback cb) override;
    void unsubscribe(uint64_t sub_id) override;

    // --- 3. 同步查询：算法组件查询"其它设备"的最近快照 ---
    DataContext::Ptr get_latest(const std::string& device_uuid) override;

    // --- 4. 系统全局变量共享（如系统状态、配置快照） ---
    void set_global_attr(const std::string& key, const wf::Variant& val) override;
    wf::Variant get_global_attr(const std::string& key) override;

private:
    struct Subscription {
        uint64_t id;
        std::string topic;
        DataCallback callback;
    };

    std::atomic<uint64_t> m_next_sub_id;

    // 订阅管理
    std::shared_mutex m_sub_mutex;
    std::unordered_map<uint64_t, Subscription> m_subscriptions;
    std::unordered_multimap<std::string, uint64_t> m_topic_to_subs;

    // 最新数据缓存（每个设备保留最新一帧）
    std::shared_mutex m_cache_mutex;
    std::unordered_map<std::string, DataContext::Ptr> m_latest_cache;

    // 全局属性
    std::shared_mutex m_global_mutex;
    std::unordered_map<std::string, wf::Variant> m_global_attrs;
};

#endif // DATA_HUB_H
