#include "imodbus_connection.h"
#include "log/log.h"
#include "error_code.h"
#include "string/rt_string.h"

#define FAIL_MAX_NUMBER      3

IModbusConnection::IModbusConnection() 
	: m_connected(false) 
{ 
	memset(&m_conn, '\0', sizeof(modbus_conn)); 
}

IModbusConnection::~IModbusConnection()
{
}

bool IModbusConnection::connect()
{
	disconnect();
	if (!create_contexts()) return false;

    if (modbus_connect(get_read_context()) != 0) return false;

    if (get_write_context() != get_read_context() && modbus_connect(get_write_context()) != 0) return false;

	set_slave();
	m_connected = true;
    return true;
}

bool IModbusConnection::is_connected() const
{
    return m_connected;
}

int IModbusConnection::reconnect()
{
	disconnect();
	return connect();
}

void IModbusConnection::set_slave()
{
	modbus_t* ctx = get_read_context();
	if (!ctx) {
		LOGERROR("Failed to create Modbus context");
		return;
	}

	modbus_set_slave(ctx, m_conn.slave);
	modbus_set_response_timeout(ctx, 1, 0); // 读超时1秒

	if (get_write_context() != ctx) {
		modbus_t* wctx = get_write_context();
		if (wctx) {
			modbus_set_slave(wctx, m_conn.slave);
			modbus_set_response_timeout(wctx, 0, 100000); // 写超时100ms
		}
	}
}

int IModbusConnection::read(modbus_function_code code, uint16_t address, uint16_t nb, uint8_t *buf)
{
    int rc = -1;
	modbus_t* ctx = get_read_context();
	if (!ctx) {
		LOGERROR("Failed to create Modbus context");
        m_connected = false;
		return RES_ERR_CREATE;
	}

	switch (code)
	{
	case modbus_function_code::COILS:
		rc = modbus_read_bits(ctx, address, nb, buf);
		break;
	case modbus_function_code::DISCRETE_INPUTS:
		rc = modbus_read_input_bits(ctx, address, nb, buf);
		break;
    case modbus_function_code::HOLDING_REGISTERS:
		rc = modbus_read_registers(ctx, address, nb, (uint16_t*)buf);
        break;
	case modbus_function_code::INPUT_REGISTERS:
		rc = modbus_read_input_registers(ctx, address, nb, (uint16_t*)buf);
		break;
    default:
        break;
	}

	if (rc == -1)
	{		
		LOGERROR("read buffer false, funciton code = %d, addr = %d, nb = %d, error:%s\n", code, address, nb, modbus_strerror(errno));
        if (errno == ECONNRESET || modbus_get_socket(ctx) == -1)// 检查是否为连接级错误
			m_connected = false;
        else
            m_connected = check_connection_active(ctx);
	}
	
	return (rc == -1) ? RES_ERR_READ_IO : RES_SUCCESS;
}


int IModbusConnection::write(modbus_function_code code, uint16_t address, uint16_t nb, uint8_t* buf)
{
	int rc = 0;
	modbus_t* ctx = get_write_context();
	if (!ctx) {
		LOGERROR("Failed to create Modbus context");
        m_connected = false;
		return RES_ERR_CREATE;
	}

	switch (code)
	{
	case modbus_function_code::SINGLE_COIL:
		rc = modbus_write_bit(ctx, address, *(uint16_t*)buf);
		break;
	case modbus_function_code::MULTIPLE_COIL:
		rc = modbus_write_bits(ctx, address, nb, buf);
		break;
		//所有写入操作的功能码(0x06, 0x10, 0x17) 都明确针对 保持寄存器(4xxxx)
		//3xxxx (Input Registers)：只读。用于提供设备采集的实时数据（如：传感器读数、只读状态、计算值）。
		//4xxxx(Holding Registers)：可读可写。用于存储设备参数、控制命令、可修改的状态等（如：设定值、运行模式）
	case modbus_function_code::SINGLE_REGISTER:
		//该功能码仅适用于 保持寄存器（Holding Registers，4xxxx 地址区）。
		rc = modbus_write_register(ctx, address, *(uint16_t*)buf);
		break;
	case modbus_function_code::MULTIPLE_REGISTER:
		//该功能码仅适用于 保持寄存器（Holding Registers，4xxxx 地址区），不适用于输入寄存器（3xxxx 区）。
		rc = modbus_write_registers(ctx, address, nb, (uint16_t*)buf);
		break;
	default:
		break;
	}

	if (rc == -1)
	{
		rt_string buf_hex((char*)buf, nb * 2);
		LOGERROR("write buffer false, function code = %d, addr = %d, nb = %d buffer = %s, error:%s\n", code, address, nb, buf_hex.dec_2_hex().c_str(), modbus_strerror(errno));
		if (errno == ECONNRESET || modbus_get_socket(ctx) == -1)// 检查是否为连接级错误
			m_connected = false;
        else
            m_connected = check_connection_active(ctx);
	}
	return (rc == -1) ? RES_ERR_WRITE_IO : RES_SUCCESS;
}