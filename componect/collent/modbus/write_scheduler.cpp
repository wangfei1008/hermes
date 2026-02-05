#include "write_scheduler.h"
#include "parameter_manager.h"
#include "imodbus_connection.h"
#include "data_handler.h"

WriteScheduler::WriteScheduler(std::shared_ptr<IModbusConnection> connection, std::shared_ptr<ParameterManager> param_manager,std::shared_ptr<DataHandler> data_handler)
    : m_connection(connection)
    , m_param_manager(param_manager)
    , m_data_handler(data_handler)
    , m_running(false)
{
}

WriteScheduler::~WriteScheduler()
{
    stop();
}

void WriteScheduler::start() 
{
    if (m_running) return;
    m_running = true;
    m_write_thread = std::thread(&WriteScheduler::write_loop, this);
}

void WriteScheduler::stop() 
{
    m_running = false;
    m_param_manager->notify_write_thread(); // 唤醒等待的写线程
    if (m_write_thread.joinable()) {
        m_write_thread.join();
    }
}

void WriteScheduler::write_loop()
{
    while (m_running) {
        modbus_parameter_request write_req;
        // 等待写请求，最多等待100ms
        if (m_param_manager->get_next_write_request(write_req, true)) {
            int success = m_connection->write(write_req.get_code(), write_req.get_address(), write_req.get_length(), write_req.get_data());
            if (0 != success) {
                std::string error = "Write failed: FC=" + std::to_string(write_req.get_code()) +  ", Addr=" + std::to_string(write_req.get_address());
                m_data_handler->handle_error(write_req, error);
            }
        }
    }
}
