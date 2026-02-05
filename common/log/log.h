#ifndef LOG_WANGFEI_20180830_H_
#define LOG_WANGFEI_20180830_H_
#include <cstdlib>

#include <log4cplus/config.hxx>
#include <log4cplus/logger.h>
#include <log4cplus/clogger.h>
#include <log4cplus/configurator.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/helpers/socket.h>
#include <log4cplus/spi/loggerimpl.h>
#include <log4cplus/spi/loggingevent.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/initializer.h>

#define LOG_LOG4C 1

#if LOG_LOG4C

static std::atomic<bool> g_log_init{ false };
static std::atomic<bool> g_log_shutdown{ false };

inline void SafeLogInit(const char* path)
{
    bool expected = false;
    if (g_log_init.compare_exchange_strong(expected, true))
    {
        log4cplus_file_configure(reinterpret_cast<const log4cplus_char_t*>(path));
    }
}

inline void SafeShutdown()
{
    bool expected = false;
    if (g_log_shutdown.compare_exchange_strong(expected, true))
    {
        log4cplus_shutdown();
    }
}

// 初始化宏：建议在 main 函数第一行调用
#define LOG_INIT(path)      SafeLogInit(path);
#define LOG_SHUTDOWN()      SafeShutdown();
#define LOGDEBUG(...)		log4cplus_logger_log(__FUNCTION__, log4cplus::DEBUG_LOG_LEVEL, (const log4cplus_char_t *)__VA_ARGS__)
#define LOGERROR(...)		log4cplus_logger_log(__FUNCTION__, log4cplus::ERROR_LOG_LEVEL, (const log4cplus_char_t *)__VA_ARGS__)
#define LOGFATAL(...)		log4cplus_logger_log(__FUNCTION__, log4cplus::FATAL_LOG_LEVEL,(const log4cplus_char_t *)__VA_ARGS__)
#define LOGINFO(...)		log4cplus_logger_log(__FUNCTION__, log4cplus::INFO_LOG_LEVEL, (const log4cplus_char_t *)__VA_ARGS__)
#define LOGWARN(...)		log4cplus_logger_log(__FUNCTION__, log4cplus::WARN_LOG_LEVEL, (const log4cplus_char_t *)__VA_ARGS__)
#define LOGTRACE(...)		log4cplus_logger_log(__FUNCTION__, log4cplus::TRACE_LOG_LEVEL,(const log4cplus_char_t *)__VA_ARGS__)

#else

class LoggerManager {
public:
    /**
     * @brief 获取单例并初始化
     * @param config_path 配置文件路径，仅在第一次调用时生效
     */
    static LoggerManager& instance(const char* config_path = nullptr)
    {
        static LoggerManager instance(config_path);
        return instance;
    }

    /**
     * @brief 显式销毁日志系统
     */
    void destroy()
    {
        if (!m_is_shutdown.exchange(true))
        {
            log4cplus::Logger::shutdown();
        }
    }

    // 获取 Logger 实例供宏使用
    log4cplus::Logger& logger()
    {
        return m_logger;
    }

    // 禁止拷贝和移动
    LoggerManager(const LoggerManager&) = delete;
    LoggerManager& operator=(const LoggerManager&) = delete;

private:
    // 构造函数：处理初始化
    explicit LoggerManager(const char* config_path)
        : m_initializer()
        , m_is_shutdown(false)
    {
        if (config_path && strlen(config_path) > 0) {
            log4cplus::PropertyConfigurator::doConfigure(LOG4CPLUS_TEXT(config_path));
        }
        else {
            // 如果没提供配置文件，默认使用控制台基础配置，防止崩溃
            log4cplus::BasicConfigurator::doConfigure();
        }
        m_logger = log4cplus::Logger::getRoot();
    }

    // 析构函数：处理销毁
    ~LoggerManager()
    {
        m_logger.shutdown(); // 关闭所有 appender
    }

private:
    log4cplus::Initializer m_initializer; // 管理 log4cplus 库的全局环境
    log4cplus::Logger m_logger;
    std::atomic<bool> m_is_shutdown;
};

// 初始化宏：建议在 main 函数第一行调用
#define LOG_INIT(path) LoggerManager::instance(path)
#define LOG_SHUTDOWN() LoggerManager::instance().destroy()

// 内部通用打印宏
#define LOG_COMMON(level, ...) \
LOG4CPLUS_##level##_FMT(LoggerManager::instance().logger(), __VA_ARGS__)

#define LOGTRACE(...) LOG_COMMON(TRACE, __VA_ARGS__)
#define LOGDEBUG(...) LOG_COMMON(DEBUG, __VA_ARGS__)
#define LOGINFO(...)  LOG_COMMON(INFO,  __VA_ARGS__)
#define LOGWARN(...)  LOG_COMMON(WARN,  __VA_ARGS__)
#define LOGERROR(...) LOG_COMMON(ERROR, __VA_ARGS__)
#define LOGFATAL(...) LOG_COMMON(FATAL, __VA_ARGS__)

#endif

#endif//LOG_WANGFEI_20180830_H_
