#ifndef EXECUTION_STREAM_H
#define EXECUTION_STREAM_H

#include <memory>
#include <string>
#include "component_pipeline.h"
#include "message_dispatcher.h"
#include "data_processor.h"
#include "i_data_hub.h"
#include "message_envelope.h"

/**
 * ExecutionStream: 执行流（重构后的 DeviceStream）
 * 职责：
 * 1. 组合三个独立的类：ComponentPipeline、MessageDispatcher、DataProcessor
 * 2. 提供统一的对外接口
 * 3. 协调三者的生命周期
 */
class ExecutionStream
{
public:
    ExecutionStream(int stream_id, const std::string& name);
    ~ExecutionStream();

    // 禁止拷贝
    ExecutionStream(const ExecutionStream&) = delete;
    ExecutionStream& operator=(const ExecutionStream&) = delete;

    /**
     * 添加组件到管道
     * @param comp 组件指针（所有权转移）
     * @param lib_name 动态库名称
     * @note 必须在 start() 之前调用
     */
    void add_component(IComponent* comp, const std::string& lib_name);

    /**
     * 启动执行流
     * 启动消息分发器和数据处理器
     */
    void start();

    /**
     * 停止执行流
     */
    void stop();

    /**
     * 暂停组件处理
     */
    void pause();

    /**
     * 恢复组件处理
     */
    void resume();

    /**
     * 推送消息
     * @param msg 消息封装
     */
    void push_message(const std::shared_ptr<MessageEnvelope>& msg);

    /**
     * 推送数据
     * @param data 数据上下文
     */
    void push_data(DataContext::Ptr data);

    int id() const { return m_id; }
    const std::string& name() const { return m_name; }
    size_t component_count() const;

private:
    int m_id;
    std::string m_name;

    // 三个独立的子系统
    std::unique_ptr<ComponentPipeline> m_pipeline;
    std::unique_ptr<MessageDispatcher> m_msg_dispatcher;
    std::unique_ptr<DataProcessor> m_data_processor;
};

#endif // EXECUTION_STREAM_H
