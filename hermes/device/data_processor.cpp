#include "data_processor.h"
#include "log/log.h"
#include "data_models/data_hub.h"
#include <chrono>

DataProcessor::DataProcessor(ComponentPipeline* pipeline)
    : m_pipeline(pipeline)
    , m_running(false)
    , m_queue(1024)
{
    if (!m_pipeline) {
        throw std::invalid_argument("DataProcessor: pipeline cannot be null");
    }

    LOGINFO("[DataProcessor][%s][%d] created", m_pipeline->device_name().c_str(), m_pipeline->id());
}

DataProcessor::~DataProcessor()
{
    stop();
    LOGINFO("[DataProcessor][%s][%d] destroyed", m_pipeline->device_name().c_str(), m_pipeline->id());
}

void DataProcessor::start()
{
    if (m_running) {
        LOGWARN("[DataProcessor][%s][%d] Already running", m_pipeline->device_name().c_str(), m_pipeline->id());
        return;
    }

    m_running = true;
    m_thread = std::thread(&DataProcessor::work_loop, this);
    LOGINFO("[DataProcessor][%s][%d] started", m_pipeline->device_name().c_str(), m_pipeline->id());
}

void DataProcessor::stop()
{
    if (!m_running) return;

    LOGINFO("[DataProcessor][%s][%d] stopping...", m_pipeline->device_name().c_str(), m_pipeline->id());
    m_running = false;

    // 唤醒线程
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cond.notify_all();
    }

    if (m_thread.joinable()) {
        m_thread.join();
    }

    LOGINFO("[DataProcessor][%s][%d] stopped", m_pipeline->device_name().c_str(), m_pipeline->id());
}

void DataProcessor::push_data(DataContext::Ptr data)
{
    if (!m_running) {
        LOGWARN("[DataProcessor][%s][%d] Rejecting data (not running)", m_pipeline->device_name().c_str(), m_pipeline->id());
        return;
    }

    if (!data) {
        LOGWARN("[DataProcessor][%s][%d] Null data", m_pipeline->device_name().c_str(), m_pipeline->id());
        return;
    }

    if (!m_queue.enqueue(data)) {
        LOGERROR("[DataProcessor][%s][%d] Data queue full, dropping data", m_pipeline->device_name().c_str(), m_pipeline->id());
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
    LOGINFO("[DataProcessor][%s][%d] Work loop started", m_pipeline->device_name().c_str(), m_pipeline->id());

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
                    LOGDEBUG("[DataProcessor][%s][%d] Pipeline executed successfully (cost: %ld us, frame: %lu)", m_pipeline->device_name().c_str(), m_pipeline->id(), duration, data->header.frame_index);
                } else {
                    LOGDEBUG("[DataProcessor][%s][%d] Pipeline stopped (cost: %ld us, frame: %lu)", m_pipeline->device_name().c_str(), m_pipeline->id(), duration, data->header.frame_index);
                }

            } catch (const std::exception& e) {
                LOGERROR("[DataProcessor][%s][%d] Pipeline execution failed: %s",
                    m_pipeline->device_name().c_str(), m_pipeline->id(), e.what());
            } catch (...) {
                LOGERROR("[DataProcessor][%s][%d] Pipeline execution failed (unknown error)",
                    m_pipeline->device_name().c_str(), m_pipeline->id());
            }
        }
    }

    LOGINFO("[DataProcessor][%s][%d] Work loop exited", m_pipeline->device_name().c_str(), m_pipeline->id());
}
