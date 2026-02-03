#ifndef DATA_BUFFER_H
#define DATA_BUFFER_H
#include <memory>
#include <concurrentqueue.h>
#include <atomic>
#include "i_data_hub.h"

class DataContext;

class DataBuffer {
public:
    using DataCallback = std::function<void(DataContext::Ptr)>;
    
    DataBuffer(size_t max_size = 10000);
    ~DataBuffer() = default;
    
    // 生产数据（入队）
    bool push(DataContext::Ptr data);
    
    // 消费数据（出队）
    DataContext::Ptr pop();
    
    // 尝试消费数据（非阻塞）
    bool try_pop(DataContext::Ptr& data);
    
    size_t size() const { return m_queue.size_approx(); }
    bool empty() const { return size() == 0; }
    bool full() const { return size() >= m_max_size; }
    
    // 设置回调（当队列非空时触发）
    void set_callback(DataCallback callback) { m_callback = callback; }
    
private:
    moodycamel::ConcurrentQueue<DataContext::Ptr> m_queue;
    std::atomic<size_t> m_max_size;
    DataCallback m_callback;
};
#endif // DATA_BUFFER_H