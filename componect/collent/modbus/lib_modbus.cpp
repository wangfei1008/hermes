#include "lib_modbus.h"
#include "log/log.h"
#include "component_export.h"
#include "modbus_connection_factory.h"
#include "error_code.h"
#include "time/CTimeStamp.h"
#include "string/rt_string.h"
#include "modbus_data_converter.h"

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
	: m_running(false)
	, m_frame_index(0)
{
}

LibModbus::~LibModbus() 
{
	stop();
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
	if (!m_config || m_running) return;

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

	//4、启动连接检查线程
    m_worker = std::thread(&LibModbus::worker_loop, this);
    m_running = true;
}

void LibModbus::pause()
{
    LOGINFO("[libmodbus][%s][%d] pause", m_device_context.device_name.c_str(), m_device_context.stream_id);
}

void LibModbus::resume()
{
	LOGINFO("[libmodbus][%s][%d] resume", m_device_context.device_name.c_str(), m_device_context.stream_id);
}

void LibModbus::worker_loop()
{
	while (m_running)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (!connect())
		{
			continue;
		}
	}
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
	if (m_scheduler_client.is_connected()) return true;

	if (m_scheduler_client.reconnect())
	{
		LOGINFO("[libmodbus][%s][%d] modbus reconnect success", m_device_context.device_name.c_str(), m_device_context.stream_id);
        return true;
	}

	LOGERROR("[libmodbus][%s][%d] modbus reconnect fail", m_device_context.device_name.c_str(), m_device_context.stream_id);
	
	return false;
}
void LibModbus::stop()
{
    m_running = false;
    m_scheduler_client.stop_scheduler();
    m_scheduler_client.disconnect();    
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
    //const std::string hex = rt_string(std::string(data.begin(), data.end())).dec_2_hex();
    //LOGINFO("[libmodbus][%s][%d] read modbus ok: code=%d address=%d length=%d buffer(hex)=%s",
    //    m_device_context.device_name.c_str(), m_device_context.stream_id,
    //    node ? node->function_code : function_code,
    //    node ? node->address_start : start_address,
    //    node ? node->read_length : (int)length,
    //    hex.c_str());

    DataContext::Ptr data_context = std::make_shared<DataContext>();
    data_context->header.source_device = m_device_context.device_name;
    data_context->header.stream_id = m_device_context.stream_id;
    data_context->header.timestamp_ms = static_cast<int64_t>(timestamp);
    data_context->header.frame_index = m_frame_index++;

    // 1）根据配置转换数值 → Variant
    wf::Variant value;

    if (node) {
        // 这里用 ModbusDataConverter 做数值解析
        // 你可以根据 node->type / node->bit_step_size / byte_order 等字段细化
        value = ModbusDataConverter::converter(function_code, start_address, static_cast<uint16_t>(node->read_length), data, node->value_type, node->byte_order);
    }
    else {
        // 找不到配置时：至少保留原始 HEX 字符串，便于排查
        std::string hex = rt_string(std::string(data.begin(), data.end())).dec_2_hex();
        value = wf::Variant(hex);
    }

    // 2）点名：优先用配置里的 “描述/变量名”，便于后续在流里用 key 访问
    std::string key;
    if (node && !node->description.empty())
        key = node->description;
    else if (node && !node->flowid.empty())
        key = node->flowid;
    else if (node && !node->type.empty())
        key = node->type;
    else
        key = "ModbusData";

    // 3）写入 DataContext（注意这里是右值，不要传 &value）
    ctx->push_result(key, std::move(value),
        DataDomain::TIME_SERIES,
        "LibModbus");

    // 4）发布到总线上
    m_data_hub->publish(std::to_string(m_device_context.stream_id), ctx);
}

void LibModbus::error_callback(const modbus_request& req, const std::string& error_message)
{
    auto node = m_config->get_param(req.get_code(), req.get_address());
    double timestamp = GetTimeStamp();
    LOGERROR("[libmodbus][%s][%d]read modbus fail, code = %d, address = %d, length = %d", m_device_context.device_name.c_str(), m_device_context.stream_id, \
        node->function_code, node->address_start, node->read_length);

    DataContext::Ptr data = std::make_shared<DataContext>();
	data->header.source_device = m_device_context.device_name;
	data->header.stream_id = m_device_context.stream_id;
	data->header.timestamp_ms = static_cast<int64_t>(timestamp);
	data->header.frame_index = m_frame_index++;

	data->push_result("ModbusError", error_message, DataDomain::ALARM, "LibModbus");

	m_data_hub->publish(DATA_HUB_TOPIC_FORWARD, data);
}
