#include "config.h"
#include "json/json_read.h"
#include "log/log.h"
#include "error_code.h"

std::unique_ptr<Config> Config::load_from_json(const std::string& json_str) 
{
    json_read reader;
    if (reader.init(json_str) != RES_SUCCESS) {
        LOGERROR("MqttConfig parse failed: %s", json_str.c_str());
        return nullptr;
    }

    auto cfg = std::make_unique<Config>();

    // 批量填充数据
    cfg->id = reader.get_value<std::string>("id");
    cfg->host = reader.get_value<std::string>("host");
    cfg->port = reader.get_value<int>("port");
    cfg->is_user = reader.get_value<bool>("is_user");
    cfg->user = reader.get_value<std::string>("user");
    cfg->password = reader.get_value<std::string>("password");
    cfg->topic = reader.get_value<std::string>("topic");
    cfg->response_id = reader.get_value<int>("response_id");
    cfg->tls = reader.get_value<bool>("tls");
    cfg->ca_crt = reader.get_value<std::string>("ca_crt");
    cfg->cln_crt = reader.get_value<std::string>("cln_crt");
    cfg->cln_key = reader.get_value<std::string>("cln_key");

    if (!cfg->validate()) {
        return nullptr;
    }

    return cfg;
}

bool Config::validate() const 
{
    if (host.empty() || port <= 0) {
        LOGERROR("MqttConfig validation failed: invalid host or port");
        return false;
    }
    if (tls && ca_crt.empty()) {
        LOGERROR("MqttConfig validation failed: TLS enabled but no CA certificate");
        return false;
    }
    return true;
}