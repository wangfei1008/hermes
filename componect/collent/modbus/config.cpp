#include "config.h"
#include "json/json_read.h"
#include "log/log.h"
#include "error_code.h"
#include <algorithm>

// ============== CommandParam 实现 ==============
bool CommandParam::validate() const
{
    if (type.empty()) {
        LOGERROR("CommandParam validation failed: type is empty");
        return false;
    }
    if (function_code < 0 || function_code > 255) {
        LOGERROR("CommandParam validation failed: invalid function_code %d", function_code);
        return false;
    }
    if (address_start < 0 || read_length <= 0) {
        LOGERROR("CommandParam validation failed: invalid address or length");
        return false;
    }
    return true;
}

// ============== ClientConfig 实现 ==============
std::unique_ptr<Config> Config::load_from_json(const std::string& json_str)
{
    json_read reader;
    if (reader.init(json_str) != RES_SUCCESS) {
        LOGERROR("ClientConfig parse failed: %s", json_str.c_str());
        return nullptr;
    }

    auto cfg = std::make_unique<Config>();

    // 解析基础配置
    cfg->interval = reader.get_value<int>("interval");

    // 解析连接信息
    memset(&cfg->conn_info, 0, sizeof(modbus_conn));
    cfg->conn_info.type = reader.get_value<int>("connect_type");
    cfg->conn_info.slave = reader.get_value<int>("slave_id");

    if (cfg->conn_info.type == modbus_conntype::IP_TCP) {
        std::string ip = reader.get_value<std::string>("ip");
        memcpy(cfg->conn_info.info.tcp.ip, ip.c_str(),
            std::min(ip.length(), sizeof(cfg->conn_info.info.tcp.ip) - 1));
        cfg->conn_info.info.tcp.port = reader.get_value<int>("port");
    }
    else if (cfg->conn_info.type == modbus_conntype::RTU) {
        std::string name = reader.get_value<std::string>("name");
        memcpy(cfg->conn_info.info.rtu.device, name.c_str(),
            std::min(name.length(), sizeof(cfg->conn_info.info.rtu.device) - 1));
        cfg->conn_info.info.rtu.baud = reader.get_value<int>("baud");
        cfg->conn_info.info.rtu.parity = reader.get_value<std::string>("parity")[0];
        cfg->conn_info.info.rtu.data_bit = reader.get_value<int>("databit");
        cfg->conn_info.info.rtu.stop_bit = reader.get_value<int>("stopbit");
    }

    // 解析命令数组
    auto command_array = reader.get_value_pointer("command");
    if (!command_array || !command_array->IsArray()) {
        LOGERROR("ClientConfig parse failed: command array not found or invalid");
        return nullptr;
    }

    auto array = command_array->GetArray();
    for (auto it = array.Begin(); it != array.End(); ++it) {
        std::string cmd_json = reader.to_string(&(*it));
        auto cmd = parse_command(cmd_json);
        if (cmd && cmd->validate()) {
            cfg->commands.push_back(cmd);
        }
        else {
            LOGERROR("ClientConfig: skip invalid command: %s", cmd_json.c_str());
        }
    }

    if (!cfg->validate()) {
        return nullptr;
    }

    return cfg;
}

std::shared_ptr<CommandParam> Config::parse_command(const std::string& json, int opernum)
{
    json_read reader;
    if (reader.init(json) != RES_SUCCESS) {
        return nullptr;
    }

    auto cmd = std::make_shared<CommandParam>();
    cmd->opernum = opernum;
    cmd->processid = reader.get_value<int>("processid");
    cmd->flowid = reader.get_value<std::string>("flowid");
    cmd->type = reader.get_value<std::string>("type");
    cmd->function_code = reader.get_value<int>("function_code");
    cmd->address_start = reader.get_value<int>("address_start");
    cmd->read_length = reader.get_value<int>("read_length");
    cmd->bit_step_size = reader.get_value<int>("bit_step_size");
    cmd->description = reader.get_value<std::string>("description");
	cmd->value_type = reader.get_value<std::string>("value_type");
	cmd->byte_order = reader.get_value<std::string>("byte_order");
    cmd->buffer = reader.get_value<std::string>("buffer");
    cmd->config = json;

    return cmd;
}

bool Config::add_temp_command(const std::string& json)
{
    auto cmd = parse_command(json, 1); // opernum=1表示临时命令
    if (!cmd || !cmd->validate()) {
        LOGERROR("ClientConfig: failed to add temp command: %s", json.c_str());
        return false;
    }
    commands.push_back(cmd);
    return true;
}

std::shared_ptr<CommandParam> Config::get_param(uint8_t function_code,
    uint16_t start_address) const
{
    for (const auto& cmd : commands) {
        if (cmd->function_code == function_code &&
            cmd->address_start == start_address) {
            return cmd;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<CommandParam>>
Config::get_params_by_type(const std::string& type) const
{
    std::vector<std::shared_ptr<CommandParam>> result;
    for (const auto& cmd : commands) {
        if (cmd->type == type) {
            result.push_back(cmd);
        }
    }
    return result;
}

std::vector<modbus_parameter> Config::get_modbus_params(access_mode mode) const
{
    std::vector<modbus_parameter> params;
    for (const auto& cmd : commands) {
        modbus_function_code code = static_cast<modbus_function_code>(cmd->function_code);
        access_mode cmd_mode = code < modbus_function_code::SINGLE_COIL ?
            access_mode::READ_ONLY : access_mode::WRITE_ONLY;

        if (cmd_mode == mode) {
            modbus_parameter p;
            p.set_code(code);
            p.set_address(cmd->address_start);
            p.set_length(cmd->read_length);
            params.push_back(p);
        }
    }
    return params;
}

void Config::cleanup_expired_commands()
{
    commands.erase(
        std::remove_if(commands.begin(), commands.end(),
            [](const std::shared_ptr<CommandParam>& cmd) {
                return cmd->opernum == 0;
            }),
        commands.end()
    );
}

bool Config::validate() const
{
    if (interval < 0) {
        LOGERROR("ClientConfig validation failed: invalid interval %d", interval);
        return false;
    }

    if (conn_info.type != modbus_conntype::IP_TCP &&
        conn_info.type != modbus_conntype::RTU) {
        LOGERROR("ClientConfig validation failed: invalid connection type");
        return false;
    }

    if (commands.empty()) {
        LOGERROR("ClientConfig validation failed: no commands defined");
        return false;
    }

    return true;
}