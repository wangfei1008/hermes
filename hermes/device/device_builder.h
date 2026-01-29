#ifndef DEVICE_BUILDER_H
#define DEVICE_BUILDER_H

#include <memory>
#include "device_proxy.h"
#include "db_models.h"

class DeviceBuilder
{
public:
    DeviceBuilder() = default;
    ~DeviceBuilder() = default;

    std::shared_ptr<DeviceProxy> build(DeviceDTO device_info);
private:
    std::unique_ptr<DeviceStream> assemble_stream(const StreamDTO& s_info);
};

#endif
