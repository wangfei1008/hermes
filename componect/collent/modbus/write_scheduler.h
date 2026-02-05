#ifndef WRITE_SCHEDULER_H
#define WRITE_SCHEDULER_H

#include <atomic>
#include <thread>
#include <memory>

class IModbusConnection;
class ParameterManager;
class DataHandler;

class WriteScheduler 
{
public:
    WriteScheduler(std::shared_ptr<IModbusConnection> connection, std::shared_ptr<ParameterManager> param_manager, std::shared_ptr<DataHandler> data_handler);
    ~WriteScheduler();

    void start();
    void stop();

private:
    void write_loop();

    std::shared_ptr<IModbusConnection> m_connection;
    std::shared_ptr<ParameterManager> m_param_manager;
    std::shared_ptr<DataHandler> m_data_handler;

    std::atomic<bool> m_running;
    std::thread m_write_thread;
};

#endif // WRITE_SCHEDULER_H
