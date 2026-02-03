#include "lib_mqtt.h"
#include <log4cplus/appender.h>
#include "log/log.h"
#include "component_export.h"
#include <iostream>



void DebugModbusLogger() {
    // 获取当前 modbus 看到的单例
    auto& mgr = LoggerManager::instance();
    log4cplus::Logger logger = mgr.logger();

    std::cout << "\n=== Modbus Logger Debug Info ===" << std::endl;
    std::cout << "Logger Name: " << logger.getName() << std::endl;

    // 检查直接挂载的 Appender
    log4cplus::SharedAppenderPtrList appenders = logger.getAllAppenders();
    std::cout << "Direct Appender Count: " << appenders.size() << std::endl;

    // 检查是否允许向上层（Root）传递日志
    std::cout << "Additivity: " << (logger.getAdditivity() ? "True" : "False") << std::endl;

    // 重点：检查 Root Logger，因为你的单例初始化通常是针对 Root 的
    log4cplus::Logger root = log4cplus::Logger::getRoot();
    log4cplus::SharedAppenderPtrList rootAppenders = root.getAllAppenders();
    std::cout << "Root Appender Count: " << rootAppenders.size() << std::endl;

    if (rootAppenders.empty() && appenders.empty()) {
        std::cout << "RESULT: [FAIL] No appenders found in Modbus context!" << std::endl;
    }
    else {
        std::cout << "RESULT: [OK] Appenders are present." << std::endl;
    }
    std::cout << "================================\n" << std::endl;
}

extern "C"  COM_EXPORT bool create_lib(IComponent** new_component);
extern "C"  COM_EXPORT bool release_lib(IComponent** new_component);

bool create_lib(IComponent** new_component)
{
    LOGINFO("LibMqtt create component_interface");
    IComponent* lib = new LibMqtt();
    *new_component = (IComponent*)lib;
    return true;
}

bool release_lib(IComponent** new_component)
{
    LOGINFO("LibMqtt release component_interface");
    LibMqtt* component = (LibMqtt*)*new_component;
    delete component;
    component = NULL;
    return true;
}

bool LibMqtt::init(const DeviceContext& ctx, IDataHub* hub, const std::string& config)
{
    DebugModbusLogger();
    LOGINFO("libmqtt init device name = %s, device id = %d, stream id = %d", ctx.device_name.c_str(), ctx.device_uuid, ctx.stream_id);
    return true;
}

void LibMqtt::start()
{
    LOGINFO("libmqtt start");
}

void LibMqtt::pause()
{
    LOGINFO("libmqtt start");
}

void LibMqtt::resume()
{
    LOGINFO("libmqtt start");
}

void LibMqtt::stop()
{
    LOGINFO("libmqtt stop");
}

void LibMqtt::on_message(int type, const std::string& msg)
{
    LOGINFO("libmqtt on_message type=%d, msg=%s", type, msg.c_str());
}

bool LibMqtt::process(DataContext::Ptr& pkg)
{
    LOGINFO("libmqtt process data frame_index=%lu", pkg->header.frame_index);
    return false;
}
