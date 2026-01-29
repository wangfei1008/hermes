#include "data_hub.h"
#include "log/log.h"
#include <algorithm>

DataHub::DataHub()
    : m_next_sub_id(1)
{
    LOGINFO("DataHub: Created");
}

DataHub::~DataHub()
{
    std::unique_lock<std::shared_mutex> lock(m_sub_mutex);
    m_subscriptions.clear();
    m_topic_to_subs.clear();
    LOGINFO("DataHub: Destroyed");
}

DataHub& DataHub::instance()
{
    static DataHub instance;
    return instance;
}

void DataHub::create()
{
	LOGINFO("DataHub: Instance created");
}

DataHub* DataHub::get()
{
    return this;
}

void DataHub::publish(DataContext::Ptr pkg)
{
    if (!pkg) {
        LOGWARN("DataHub: Attempting to publish null data");
        return;
    }

    const std::string& device_uuid = pkg->header.source_device;

    // 1. 更新缓存
    {
        std::unique_lock<std::shared_mutex> lock(m_cache_mutex);
        m_latest_cache[device_uuid] = pkg;
    }

    LOGDEBUG("DataHub: Published data from device [%s], frame [%lu]", device_uuid.c_str(), pkg->header.frame_index);

    // 2. 通知订阅者
    std::vector<DataCallback> callbacks;
    {
        std::shared_lock<std::shared_mutex> lock(m_sub_mutex);

        // 查找订阅该设备的回调
        auto range = m_topic_to_subs.equal_range(device_uuid);
        for (auto it = range.first; it != range.second; ++it)
        {
            auto sub_it = m_subscriptions.find(it->second);
            if (sub_it != m_subscriptions.end()) {
                callbacks.push_back(sub_it->second.callback);
            }
        }

        // 查找订阅特定域的回调（如 "ALARM"）
        for (const auto& [key, item] : pkg->items)
        {
            std::string domain_topic = "domain:" + std::to_string(static_cast<int>(item.domain));
            auto domain_range = m_topic_to_subs.equal_range(domain_topic);
            for (auto it = domain_range.first; it != domain_range.second; ++it)
            {
                auto sub_it = m_subscriptions.find(it->second);
                if (sub_it != m_subscriptions.end()) {
                    callbacks.push_back(sub_it->second.callback);
                }
            }
        }
    }

    // 3. 执行回调（在锁外执行，避免死锁）
    for (auto& cb : callbacks)
    {
        try {
            cb(pkg);
        } catch (const std::exception& e) {
            LOGERROR("DataHub: Subscriber callback exception: %s", e.what());
        } catch (...) {
            LOGERROR("DataHub: Subscriber callback unknown exception");
        }
    }

    LOGDEBUG("DataHub: Notified %zu subscribers for device [%s]", callbacks.size(), device_uuid.c_str());
}

uint64_t DataHub::subscribe(const std::string& topic, DataCallback cb)
{
    if (!cb) {
        LOGERROR("DataHub: Cannot subscribe with null callback");
        return 0;
    }

    std::unique_lock<std::shared_mutex> lock(m_sub_mutex);

    uint64_t sub_id = m_next_sub_id++;
    Subscription sub{sub_id, topic, cb};

    m_subscriptions[sub_id] = sub;
    m_topic_to_subs.emplace(topic, sub_id);

    LOGINFO("DataHub: Subscription [%lu] added for topic [%s]", sub_id, topic.c_str());
    return sub_id;
}

void DataHub::unsubscribe(uint64_t sub_id)
{
    std::unique_lock<std::shared_mutex> lock(m_sub_mutex);

    auto it = m_subscriptions.find(sub_id);
    if (it == m_subscriptions.end()) {
        LOGWARN("DataHub: Subscription [%lu] not found", sub_id);
        return;
    }

    const std::string& topic = it->second.topic;

    // 从 topic 映射中移除
    auto range = m_topic_to_subs.equal_range(topic);
    for (auto topic_it = range.first; topic_it != range.second;)
    {
        if (topic_it->second == sub_id) {
            topic_it = m_topic_to_subs.erase(topic_it);
        } else {
            ++topic_it;
        }
    }

    m_subscriptions.erase(it);

    LOGINFO("DataHub: Subscription [%lu] removed from topic [%s]", sub_id, topic.c_str());
}

DataContext::Ptr DataHub::get_latest(const std::string& device_uuid)
{
    std::shared_lock<std::shared_mutex> lock(m_cache_mutex);

    auto it = m_latest_cache.find(device_uuid);
    if (it != m_latest_cache.end()) {
        LOGDEBUG("DataHub: Retrieved latest data for device [%s]", device_uuid.c_str());
        return it->second;
    }

    LOGDEBUG("DataHub: No cached data for device [%s]", device_uuid.c_str());
    return nullptr;
}

void DataHub::set_global_attr(const std::string& key, const wf::Variant& val)
{
    std::unique_lock<std::shared_mutex> lock(m_global_mutex);
    m_global_attrs[key] = val;
    LOGDEBUG("DataHub: Global attribute [%s] set", key.c_str());
}

wf::Variant DataHub::get_global_attr(const std::string& key)
{
    std::shared_lock<std::shared_mutex> lock(m_global_mutex);

    auto it = m_global_attrs.find(key);
    if (it != m_global_attrs.end()) {
        return it->second;
    }

    LOGDEBUG("DataHub: Global attribute [%s] not found", key.c_str());
    return wf::Variant();
}
