#ifndef PARAMETER_MANAGER_H
#define PARAMETER_MANAGER_H

#include <vector>
#include <set>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "modbus_parameter.h"


class ParameterManager
{
public:
    ParameterManager();

    // 读取参数管理 //
	// 添加读取参数
    void add_read_parameter(uint8_t function_code, uint16_t start_address, uint16_t length);
	// 删除读取参数
    void remove_read_parameter(uint8_t function_code, uint16_t start_address);
	// 优化读取组
    int optimize_read_groups();
    // 获取读取组
    std::vector<modbus_group_request> read_groups();

    // 写入参数管理 //
    void queue_write_request(uint8_t function_code, uint16_t address, uint16_t length, const uint8_t* values);
    bool get_next_write_request(modbus_parameter_request& request, bool wait = false);
    void notify_write_thread();

private:
    // 合并相邻的读取请求
    void merge_requests(const std::set<modbus_parameter_request>& requests);


private:
    std::vector<modbus_group_request> m_read_groups;

    std::set<modbus_parameter_request> m_read_parameters;
    std::mutex m_read_params_mutex;

    std::queue<modbus_parameter_request> m_write_queue;
    std::mutex m_write_queue_mutex;
    std::condition_variable m_write_cv;
};

#endif // PARAMETER_MANAGER_H
