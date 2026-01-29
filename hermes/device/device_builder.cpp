#include "device_builder.h"
#include "database/sqlite_repository.h"
#include "library/plugin_loader.h"
#include <string>

std::shared_ptr<DeviceProxy> DeviceBuilder::build(DeviceDTO dev_info)
{
    auto proxy = std::make_shared<DeviceProxy>(std::to_string(dev_info.id), dev_info.name);

    // 内部去处理流的组装
    auto streams = SQLiteRepository::query_streams_by_device(dev_info.id);
    for (auto& s : streams)
    {
        proxy->add_stream(assemble_stream(s));
    }
    return proxy;
}

std::unique_ptr<DeviceStream> DeviceBuilder::assemble_stream(const StreamDTO& s_info)
{
    auto stream = std::make_unique<DeviceStream>(s_info.id, s_info.stream_name);
    auto comps = SQLiteRepository::query_components_by_stream(s_info.id);

    for (auto& c : comps) 
    {
        auto* comp = PluginLoader::instance().create(c.lib_name);
        if (comp && comp->init(DeviceContext(), NULL, c.comp_config))
        {
            stream->add_component(comp);
        }
    }
    return stream;
}
