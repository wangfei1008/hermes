#ifndef READ_SCHEDULER_H
#define READ_SCHEDULER_H

#include <atomic>
#include <thread>
#include <memory>
#include <chrono>

class IModbusConnection;
class ParameterManager;
class DataHandler;

class ReadScheduler {
public:
    ReadScheduler(std::shared_ptr<IModbusConnection> connection, std::shared_ptr<ParameterManager> param_manager, std::shared_ptr<DataHandler> data_handler);
    ~ReadScheduler();

    void start();
    void stop();
    void set_read_interval(unsigned int milliseconds);

private:
    void read_loop();

    std::shared_ptr<IModbusConnection> m_connection;
    std::shared_ptr<ParameterManager> m_param_manager;
    std::shared_ptr<DataHandler> m_data_handler;

    std::atomic<bool> m_running;    
    unsigned int m_read_interval;
    std::thread m_read_thread;
};

#endif // READ_SCHEDULER_H
