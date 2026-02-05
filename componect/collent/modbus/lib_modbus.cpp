#include "lib_modbus.h"
#include "log/log.h"
#include "component_export.h"
#include "modbus_connection_factory.h"

extern "C"  COM_EXPORT bool create_lib(IComponent** new_component);
extern "C"  COM_EXPORT bool release_lib(IComponent** new_component);

bool create_lib(IComponent** new_component)
{
    LOGINFO("[LibModbus] create component_interface");
    IComponent* lib = new LibModbus();
    *new_component = (IComponent*)lib;
    return true;
}

bool release_lib(IComponent** new_component)
{
    LOGINFO("[LibModbus] release component_interface");
    LibModbus* component = (LibModbus*)*new_component;
    delete component;
    component = NULL;
    return true;
}

LibModbus::LibModbus()
{
}

LibModbus::~LibModbus() 
{
}

bool LibModbus::init(const DeviceContext& ctx, IDataHub* hub, const std::string& config)
{
    if (hub == nullptr) {
        LOGERROR("[LibModbus][%s][%d] init data hub is nullptr", ctx.device_name.c_str(), ctx.stream_id);
        return false;
    }
    if (config.empty()) {
        LOGERROR("[LibModbus][%s][%d] init config is empty", ctx.device_name.c_str(), ctx.stream_id);
        return false;
    }
    LOGINFO("[LibModbus][%s][%d]init start", ctx.device_name.c_str(), ctx.stream_id);
    m_device_context = ctx;
    m_data_hub = hub;

    // 1. 解析配置
    m_config = Config::load_from_json(config);
    if (!m_config) {
        LOGERROR("[LibModbus][%s][%d] init failed: invalid configuration", ctx.device_name.c_str(), ctx.stream_id);
        return false; // 配置加载失败
    }

    LOGINFO("[LibModbus][%s][%d] initialized success", ctx.device_name.c_str(), ctx.stream_id);
    return true;
}

void LibModbus::start()
{
	if (!m_config) return;

    //1、创建连接modbus连接
    if (!m_scheduler_client.connect(ModbusConnectionFactory::create(&m_config->conn_info)))
    {
        LOGERROR("");
        return;
    }

	//2、启动调度器
    m_scheduler_client.start_scheduler(m_config->interval);
    m_scheduler_client.set_data_callback(std::bind(&LibModbus::response_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
        std::bind(&LibModbus::error_callback, this, std::placeholders::_1, std::placeholders::_2));

    //3、管理参数
    manager_args();

	//4、启动循环定时器，每秒尝试判断连接一次
    m_sendlooptimer.SetInterval(1000000);
    QObject::connect(&m_sendlooptimer, &CCPUTimer::signal_timerout, [this]() {
        if (RES_SUCCESS != connect()) return;
        });
    m_sendlooptimer.Start();

    return RES_SUCCESS;
}

void LibModbus::pause()
{
}

void LibModbus::resume()
{
}

///////////////////////////////////////////////////////////////////
//函数名称： connect
//功能描述： 连接modbus
//参数说明： 无
//返回值：   返回值说明
//作者：wangfei
//时间：2023/11/17
///////////////////////////////////////////////////////////////////
bool LibModbus::connect()
{
	if (m_scheduler_client.is_connected()) return RES_SUCCESS;
	m_open = m_scheduler_client.reconnect();
	if (m_open)
	{
		LOGINFO("[%d][%f][%s] modbus reconnect success", m_compenenttype, GetTimeStamp(), get_name().c_str());
	}
	else
	{
		LOGERROR("[%d][%f][%s] modbus reconnect fail", m_compenenttype, GetTimeStamp(), get_name().c_str());
	}
	return m_open ? RES_SUCCESS : RES_ERR_OPEN_IO;
}
void LibModbus::stop()
{
    close();
    m_running = false;
}

void LibModbus::on_message(int type, const std::string& msg)
{
    LOGINFO("[libmodbus][%s][%d] on_message type=%d, msg=%s", m_device_context.device_name.c_str(), m_device_context.stream_id, type, msg.c_str());
}

bool LibModbus::process(DataContext::Ptr& pkg)
{
    LOGINFO("[libmodbus][%s][%d] process data frame_index=%lu", m_device_context.device_name.c_str(), m_device_context.stream_id, pkg->header.frame_index);
    return true;
}

///////////////////////////////////////////////////////////////////
//函数名称： close
//功能描述： 关闭modbus连接
//参数说明： 参数说明
//返回值：   返回值说明
//作者：wangfei
//时间：2023/11/17
///////////////////////////////////////////////////////////////////
void LibModbus::close()
{
    m_scheduler_client.stop_scheduler();
    m_scheduler_client.disconnect();
}

void LibModbus::write_targetdata(int value, double timestamp, const string& config)
{
    DataTarget data;
    data.set_timestamp(timestamp);
    data.set_data(list<wf::variant>(1, value));
    data.set_config(config);
    data.set_name(get_name());

    m_p_data->set_data_target(data);
    LOGINFO("[%d][%f][%s] abnormal, value = %f", m_compenenttype, timestamp, get_name().c_str(), value);
}

void LibModbus::write_rawdata(const std::vector<uint8_t>&buffer, double timestamp, const string &config)
{
    DataRaw raw;
    raw.set_config(config);
    raw.set_id(get_name());
    raw.set_buffer(buffer);
    raw.set_timestamp(timestamp);

    m_p_data->set_data_raw(raw);
}

void LibModbus::manager_args()
{
    //1、 读取配置参数
    std::vector<modbus_parameter> vs = m_config->get_modbus_params(access_mode::READ_ONLY);
    if (vs.empty()) return;

    //2、注册参数
    for (auto it : vs)
        m_scheduler_client.add_read(it.get_code(), it.get_address(), it.get_length());

    //3、优化读取组
    m_scheduler_client.optimize_read_groups();
}

void LibModbus::response_callback(uint8_t function_code, uint16_t start_address, uint16_t length, const std::vector<uint8_t>& data)
{
	auto node = m_config->get_param(function_code, start_address);
    double timestamp = GetTimeStamp();
    write_rawdata(data, timestamp, node->config);
    LOGINFO("[%d][%f][%s]read modbus code = %d, adress = %d, length = %d, buffer = %s", m_compenenttype, timestamp, get_name().c_str(), \
        node->function_code, node->address_start, node->read_length, rt_string(string(data.begin(), data.end())).dec_2_hex().c_str());
}

void LibModbus::error_callback(const modbus_request& req, const std::string& error_message)
{
    auto node = m_config->get_param(req.get_code(), req.get_address());
    double timestamp = GetTimeStamp();
    write_targetdata(RES_ERR_OPT_IO, timestamp, node->config);
    LOGERROR("[%d][%f][%s]read modbus fail, code = %d, address = %d, length = %d", m_compenenttype, timestamp, get_name().c_str(), \
        node->get_function_code(), node->get_address_start(), node->get_read_length());
}
