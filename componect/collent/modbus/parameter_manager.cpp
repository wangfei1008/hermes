#include "parameter_manager.h"
#include <algorithm>
#include <map>

ParameterManager::ParameterManager() = default;

void ParameterManager::add_read_parameter(uint8_t function_code, uint16_t start_address, uint16_t length) 
{
    std::lock_guard<std::mutex> lock(m_read_params_mutex);
    m_read_parameters.insert(modbus_request((modbus_function_code)function_code, start_address, length));
}

void ParameterManager::remove_read_parameter(uint8_t function_code, uint16_t start_address) 
{
    std::lock_guard<std::mutex> lock(m_read_params_mutex);
    // 查找并删除指定参数
    auto it = std::find_if(m_read_parameters.begin(), m_read_parameters.end(),
                           [&](const modbus_parameter_request& req) {
                               return req.get_code() == function_code &&
                                      req.get_address() == start_address;
                           });
    if (it != m_read_parameters.end()) {
        m_read_parameters.erase(it);
    }
}

void ParameterManager::merge_requests( const std::set<modbus_parameter_request>& requests)
{
    if (requests.empty()) return;

    const int MAX_READ_LENGTH = 125;
	
    std::vector<modbus_group_request> groups;
	for (auto it = requests.begin(); it != requests.end(); ++it) {
        // 检查是否可以添加到最后一个组
        if (!groups.empty()){
            modbus_group_request& last_group = groups.back();

            // 检查是否连续(可间隔)且不超过最大组大小
            uint16_t p_end_pos = it->get_address() + it->get_length() - 1;
            if (p_end_pos - last_group.get_address() < MAX_READ_LENGTH)
            {
                last_group.add_parameter(*it);
                continue;
            }
        }

        // 创建新组
        groups.emplace_back().add_parameter(*it);
	}

    m_read_groups.insert(m_read_groups.end(), groups.begin(), groups.end());
}

int ParameterManager::optimize_read_groups()
{
    std::lock_guard<std::mutex> lock(m_read_params_mutex);

    std::map<modbus_function_code, std::set<modbus_parameter_request>> grouped_params;

    //1、将读取参数按功能码分组
    for (const auto& param : m_read_parameters)
        grouped_params[param.get_code()].insert(param);

    //2、合并相邻的读取请求
    m_read_groups.clear();
    for (auto& [func_code, params] : grouped_params)
        merge_requests(params);

    return 0;
}

std::vector<modbus_group_request> ParameterManager::read_groups()
{
    return m_read_groups;
}

void ParameterManager::queue_write_request(uint8_t function_code, uint16_t address, uint16_t length, const uint8_t* values)
{
    std::lock_guard<std::mutex> lock(m_write_queue_mutex);

    modbus_parameter_request parameter(modbus_request((modbus_function_code)function_code, address, length));
    parameter.malloc_data();
    memcpy(parameter.get_data(), values, parameter.get_length() * parameter.get_bit_size());
    m_write_queue.push(parameter);
    parameter.free_data();

    m_write_cv.notify_one(); // 通知写线程有新任务
}

bool ParameterManager::get_next_write_request(modbus_parameter_request& request, bool wait)
{
    std::unique_lock<std::mutex> lock(m_write_queue_mutex);

    if (wait && m_write_queue.empty())        
        m_write_cv.wait_for(lock, std::chrono::milliseconds(100));// 等待最多100ms

    if (m_write_queue.empty()) return false;

    request = m_write_queue.front();
    m_write_queue.pop();

    return true;
}

void ParameterManager::notify_write_thread()
{
    m_write_cv.notify_one(); // 唤醒等待的写线程
}
