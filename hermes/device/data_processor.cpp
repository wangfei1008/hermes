#include "data_processor.h"
#include "log/log.h"
#include "data_models/data_hub.h"
#include <chrono>

DataProcessor::DataProcessor(int processor_id, ComponentPipeline* pipeline)
    : m_id(processor_id)
    , m_pipeline(pipeline)
    , m_running(false)
    , m_queue(1024)
{
    if (!m_pipeline) {
        throw std::invalid_argument("DataProcessor: pipeline cannot be null");
    }

    LOGINFO("DataProcessor[%d] created", m_id);
}

DataProcessor::~DataProcessor()
{
    stop();
    LOGINFO("DataProcessor[%d] destroyed", m_id);
}

void DataProcessor::start()
{
    if (m_running) {
        LOGWARN("DataProcessor[%d]: Already running", m_id);
        return;
    }

    m_running = true;
    m_thread = std::thread(&DataProcessor::work_loop, this);
    LOGINFO("DataProcessor[%d] started", m_id);
}

void DataProcessor::stop()
{
    if (!m_running) return;

    LOGINFO("DataProcessor[%d] stopping...", m_id);
    m_running = false;

    // 唤醒线程
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cond.notify_all();
    }

    if (m_thread.joinable()) {
        m_thread.join();
    }

    LOGINFO("DataProcessor[%d] stopped", m_id);
}

void DataProcessor::push_data(DataContext::Ptr data)
{
    if (!m_running) {
        LOGWARN("DataProcessor[%d]: Rejecting data (not running)", m_id);
        return;
    }

    if (!data) {
        LOGWARN("DataProcessor[%d]: Null data", m_id);
        return;
    }

    if (!m_queue.enqueue(data)) {
        LOGERROR("DataProcessor[%d]: Data queue full, dropping data", m_id);
        return;
    }

    // 唤醒数据处理线程
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cond.notify_one();
    }
}

void DataProcessor::work_loop()
{
    LOGINFO("DataProcessor[%d]: Work loop started", m_id);

    while (m_running)
    {
        DataContext::Ptr data;

        // 等待数据
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cond.wait(lock, [this] {
                return !m_running || m_queue.size_approx() > 0;
            });
        }

        // 检查 running 状态
        if (!m_running) break;

        // 批量处理数据
        while (m_queue.try_dequeue(data))
        {
            if (!data) continue;

            auto start_time = std::chrono::high_resolution_clock::now();

            try {
                // 执行 Pipeline
                bool success = m_pipeline->execute(data);

                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

                if (success) {
                    // Pipeline 成功；不在此发布，由结果组件按 topic "0" 发布给总转发
                    DataHub::instance().publish(DATA_HUB_TOPIC_FORWARD, data);
                    LOGDEBUG("DataProcessor[%d]: Pipeline executed successfully (cost: %ld us, frame: %lu)", m_id, duration, data->header.frame_index);
                } else {
                    LOGDEBUG("DataProcessor[%d]: Pipeline stopped (cost: %ld us, frame: %lu)", m_id, duration, data->header.frame_index);
                }

            } catch (const std::exception& e) {
                LOGERROR("DataProcessor[%d]: Pipeline execution failed: %s",
                         m_id, e.what());
            } catch (...) {
                LOGERROR("DataProcessor[%d]: Pipeline execution failed (unknown error)",                         m_id);
            }
        }
    }

    LOGINFO("DataProcessor[%d]: Work loop exited", m_id);
}
