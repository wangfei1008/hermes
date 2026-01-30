#include "device_builder.h"
#include "database/sqlite_repository.h"
#include "library/plugin_loader.h"
#include <string>
#include "log/log.h"
#include "data_models/data_hub.h"

std::shared_ptr<DeviceProxy> DeviceBuilder::build(const DeviceDTO& dev_info)
{
    auto proxy = std::make_shared<DeviceProxy>(std::to_string(dev_info.id), dev_info.name);

    // 查询所有流
    auto streams = SQLiteRepository::query_streams_by_device(dev_info.id);
    LOGINFO("DeviceBuilder: Building device [%s] with %d streams", dev_info.name.c_str(), streams.size());
    for (auto& s : streams)
    {
        auto stream = assemble_stream(dev_info, s);
        if (stream) {
            proxy->add_stream(std::move(stream));
        }
    }
    return proxy;
}

std::unique_ptr<ExecutionStream> DeviceBuilder::assemble_stream(const DeviceDTO& device_info, const StreamDTO& stream_info)
{
    auto stream = std::make_unique<ExecutionStream>(stream_info.id, stream_info.stream_name);
    auto comps = SQLiteRepository::query_components_by_stream(stream_info.id);

    for (auto& c : comps) 
    {
        auto* comp = PluginLoader::instance().create(c.lib_name);
        if (comp && comp->init(DeviceContext{ device_info.id, device_info.name, stream_info.id }
            , DataHub::instance().get()
            , c.comp_config))
        {
            stream->add_component(comp, c.lib_name);
        }
    }
    return stream;
}
