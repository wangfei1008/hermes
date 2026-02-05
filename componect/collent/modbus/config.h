#ifndef CONFIG_H
#define CONFIG_H
#include <string>
#include <vector>
#include <memory>
#include "modbus_parameter.h"

// 数据结构体：单个命令参数
struct CommandParam
{
    // 操作控制：-1永久命令，0待清除，正数执行次数
    int opernum = -1;
    int processid = 0;
    std::string flowid;
    std::string type;

    // Modbus相关
    int address_code = 0;
    int function_code = 0;
    int address_start = 0;
    int read_length = 0;
    int bit_step_size = 0;
    std::string buffer;

    // 原始配置(用于追溯)
    std::string config;

    // 验证单个命令参数
    bool validate() const;
};

// 数据结构体：客户端配置
struct Config
{
    // 基础配置
    int interval = 0;

    // 连接信息
    modbus_conn conn_info;

    // 命令参数列表
    std::vector<std::shared_ptr<CommandParam>> commands;

    // 静态工厂方法：从JSON解析
    static std::unique_ptr<Config> load_from_json(const std::string& json_str);

    // 添加临时命令参数
    bool add_temp_command(const std::string& json);

    // 查询接口
    std::shared_ptr<CommandParam> get_param(uint8_t function_code, uint16_t start_address) const;
    std::vector<std::shared_ptr<CommandParam>> get_params_by_type(const std::string& type) const;
    std::vector<modbus_parameter> get_modbus_params(access_mode mode) const;

    // 清理过期命令(opernum==0的命令)
    void cleanup_expired_commands();

    // 校验配置完整性
    bool validate() const;

private:
    // 解析单个命令参数
    static std::shared_ptr<CommandParam> parse_command(const std::string& json, int opernum = -1);
};

#endif // CONFIG_H