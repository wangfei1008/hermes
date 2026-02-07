#include "device_builder.h"
#include "database/sqlite_repository.h"
#include "library/plugin_loader.h"
#include <algorithm>
#include <string>
#include "log/log.h"
#include "data_models/data_hub.h"

std::shared_ptr<DeviceProxy> DeviceBuilder::build(const DeviceDTO& dev_info)
{
    // 查询所有流
    auto streams = SQLiteRepository::query_streams_by_device(dev_info.id);
    LOGINFO("[DeviceBuilder][%s] Building with %zu streams", dev_info.name.c_str(), streams.size());
    if (streams.size() > 0) {
        auto proxy = std::make_shared<DeviceProxy>(std::to_string(dev_info.id), dev_info.name);
        for (auto& s : streams)
        {
            auto stream = assemble_stream(dev_info, s);
            if (stream) {
                proxy->add_stream(std::move(stream));
            }
            else {
				LOGERROR("[DeviceBuilder][%s] Failed to assemble stream[% s]", dev_info.name.c_str(), s.stream_name.c_str());
            }
        }
        return proxy;
    }
	return nullptr;
}

std::unique_ptr<ExecutionStream> DeviceBuilder::assemble_stream(const DeviceDTO& device_info, const StreamDTO& stream_info)
{
    auto comps = SQLiteRepository::query_components_by_stream(stream_info.id);
    if (comps.empty()){
        return nullptr;
    }

    // 强制按 order_index 升序组装（即使 SQL 未排序也能保证顺序正确）
    std::stable_sort(comps.begin(), comps.end(), [](const ComponectDTO& a, const ComponectDTO& b) {
        return a.order_index < b.order_index;
    });

    // 传入 subscribe_topic：空则默认订阅 stream_id，虚拟设备配置为 "0" 订阅结果数据
    auto stream = std::make_unique<ExecutionStream>(
        device_info.name,
        stream_info.id, 
        stream_info.stream_name
    );

    for (auto& c : comps) {
        auto* comp = PluginLoader::instance().create(c.lib_name);
        if (comp && comp->init(DeviceContext{ device_info.id, device_info.name, stream_info.id }
            , DataHub::instance().get()
            , c.comp_config)){
            // 按数据库顺序添加，并携带 output_to_next 控制 pipeline 结束/继续
            stream->add_component(comp, c.lib_name, c.order_index, c.output_to_next);
        }
        else {
            LOGERROR("[DeviceBuilder][%s] Failed to create or init component [%s] for stream [%s]", device_info.name.c_str(), c.lib_name.c_str(), stream_info.stream_name.c_str());
        }
    }
    return stream;
}
