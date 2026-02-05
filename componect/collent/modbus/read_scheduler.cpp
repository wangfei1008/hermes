#include "read_scheduler.h"
#include "parameter_manager.h"
#include "data_handler.h"
#include "imodbus_connection.h"
#include "log/log.h"

ReadScheduler::ReadScheduler(std::shared_ptr<IModbusConnection> connection, std::shared_ptr<ParameterManager> param_manager, std::shared_ptr<DataHandler> data_handler)
    : m_connection(connection)
    , m_param_manager(param_manager)
    , m_data_handler(data_handler)
    , m_running(false)
    , m_read_interval(100)
{
}

ReadScheduler::~ReadScheduler()
{
    stop();
}

void ReadScheduler::start() 
{
    if (m_running) return;
    m_running = true;
    m_read_thread = std::thread(&ReadScheduler::read_loop, this);
}

void ReadScheduler::stop() 
{
    m_running = false;
    if (m_read_thread.joinable()) {
        m_read_thread.join();
    }
}

void ReadScheduler::set_read_interval(unsigned int milliseconds)
{
    m_read_interval = milliseconds;
}

void ReadScheduler::read_loop() 
{
    while (m_running) {
        // 获取合并后的读取请求
        auto requests = m_param_manager->read_groups();

        for (auto req : requests) {
            // 执行读取操作
            req.malloc_data();
 
            if (0 == m_connection->read(req.get_code(), req.get_address(), req.get_length(), req.get_data())) {
                // 处理成功响应
                m_data_handler->process_response(req.splite_to_parameters());
            } else {
                // 处理错误
                std::string error = "Read failed: FC=" + std::to_string(req.get_code()) + ", Addr=" + std::to_string(req.get_address());
                m_data_handler->handle_error(req, error);
            }
			req.free_data();
        }

        // 等待下一个读取周期
        std::this_thread::sleep_for(std::chrono::milliseconds(m_read_interval));
    }
}
