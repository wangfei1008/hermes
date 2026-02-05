#ifndef COMPONENT_PIPELINE_H
#define COMPONENT_PIPELINE_H

#include <vector>
#include <string>
#include <memory>
#include "i_component.h"

/**
 * ComponentPipeline: 组件管道（纯粹的组件编排器）
 * 职责：
 * 1. 管理组件列表（添加、删除、排序）
 * 2. 提供同步的 execute() 方法执行 Pipeline
 * 3. 不包含任何线程、队列
 */
class ComponentPipeline
{
public:
    ComponentPipeline(const std::string& device_name, int pipeline_id, const std::string& name);
    ~ComponentPipeline();

    // 禁止拷贝
    ComponentPipeline(const ComponentPipeline&) = delete;
    ComponentPipeline& operator=(const ComponentPipeline&) = delete;

    /**
     * 添加组件到管道
     * @param comp 组件指针（所有权转移）
     * @param lib_name 动态库名称
     * @note 必须在 start() 之前调用
     */
    void add_component(IComponent* comp, const std::string& lib_name);

    /**
     * 启动所有组件
     */
    bool start();

    /**
     * 暂停所有组件
     */
    void pause();

    /**
     * 恢复所有组件
     */
    void resume();

    /**
     * 停止所有组件
     */
    void stop();

    /**
     * 执行 Pipeline（同步调用）
     * @param data 输入数据
     * @return true=继续后续处理, false=中断
     */
    bool execute(DataContext::Ptr& data);

    /**
     * 分发消息到所有组件
     * @param type 消息类型
     * @param msg 消息内容
     */
    void dispatch_message(int type, const std::string& msg);

    int id() const { return m_id; }
    const std::string& name() const { return m_name; }
    size_t component_count() const;
    bool is_started() const { return m_started; }
    const std::string& device_name() const { return m_device_name; }

private:
    void cleanup_components();

private:
    struct ComponentInfo {
        IComponent* component;
        std::string lib_name;
    };
    std::string m_device_name;
    int m_id;
    std::string m_name;
    bool m_started;
    std::vector<ComponentInfo> m_components;
};

#endif // COMPONENT_PIPELINE_H
