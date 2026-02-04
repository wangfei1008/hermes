#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

#include <string>
#include <memory>

// 使用结构体存储配置，符合数据载体职责单一性
struct Config 
{
    // 基础连接信息
    std::string id;
    std::string host;
    int port = 1883;

    // 认证信息
    bool is_user = false;
    std::string user;
    std::string password;

    // 业务信息
    std::string topic;
    int response_id = 0;

    // TLS/SSL 信息
    bool tls = false;
    std::string ca_crt;
    std::string cln_crt;
    std::string cln_key;

    // 静态工厂方法：集中处理解析逻辑
    static std::unique_ptr<Config> load_from_json(const std::string& json_str);

    // 校验逻辑
    bool validate() const;
};

#endif