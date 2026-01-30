#ifndef DEVICE_BUILDER_H
#define DEVICE_BUILDER_H

#include <memory>
#include "device_proxy.h"
#include "db_models.h"

/**
 * DeviceBuilder: 设备构建器
 * 职责：从数据库读取配置，构建完整的设备代理和执行流
 */

class DeviceBuilder
{
public:
    DeviceBuilder() = default;
    ~DeviceBuilder() = default;

    /**
     * 构建设备代理
     * @param device_info 设备信息
     * @param hub 数据中心（必须非空）
     * @return 设备代理智能指针
     */
    std::shared_ptr<DeviceProxy> build(const DeviceDTO& device_info);
private:
    /**
     * 组装单个执行流
     * @param s_info 流信息
     * @param hub 数据中心
     * @return 执行流智能指针
     */
    std::unique_ptr<ExecutionStream> assemble_stream(const DeviceDTO& device_info, const StreamDTO& stream_info);
};

#endif
