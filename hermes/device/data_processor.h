#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <memory>
#include "ReaderWriterQueue/readerwriterqueue.h"
#include "component_pipeline.h"

/**
 * DataProcessor: 数据处理器
 * 职责：
 * 1. 管理数据队列
 * 2. 独立线程处理数据
 * 3. 调用 ComponentPipeline::execute()
 * 4. 将处理结果发布到 DataHub
 */
class DataProcessor
{
public:
    DataProcessor(int processor_id, ComponentPipeline* pipeline);
    ~DataProcessor();

    // 禁止拷贝
    DataProcessor(const DataProcessor&) = delete;
    DataProcessor& operator=(const DataProcessor&) = delete;

    /**
     * 启动数据处理线程
     */
    void start();

    /**
     * 停止数据处理线程
     */
    void stop();

    /**
     * 推送数据到队列
     * @param data 数据上下文
     */
    void push_data(DataContext::Ptr data);

    bool is_running() const { return m_running; }

private:
    void work_loop();

private:
    int m_id;
    ComponentPipeline* m_pipeline;
    
    std::atomic<bool> m_running;
    std::thread m_thread;
    
    moodycamel::ReaderWriterQueue<DataContext::Ptr> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};

#endif // DATA_PROCESSOR_H
