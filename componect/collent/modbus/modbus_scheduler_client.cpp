#include "modbus_scheduler_client.h"
#include "error_code.h"

#include "imodbus_connection.h"
#include "modbus_connection_tcp.h"
#include "modbus_connection_rtu.h"

#include "read_scheduler.h"
#include "write_scheduler.h"

#include "parameter_manager.h"
#include "data_handler.h"

#include "modbus_parameter.h"

ModbusSchedulerClient::ModbusSchedulerClient()
	: m_connection(nullptr),
	m_param_manager(nullptr),
	m_data_handler(nullptr),
	m_read_scheduler(nullptr),
	m_write_scheduler(nullptr)
{
	m_param_manager = std::make_shared<ParameterManager>();
	m_data_handler = std::make_shared<DataHandler>();
}


void ModbusSchedulerClient::write(uint8_t function_code, uint16_t address, uint16_t length, const uint8_t* values)
{
	if (m_param_manager) {
		m_param_manager->queue_write_request(function_code, address, length, values);
	}
	else {
		throw std::runtime_error("Parameter manager is not initialized.");
	}
}

void ModbusSchedulerClient::set_data_callback(ResponseCallback callback, ErrorCallback error_callback)
{
	if (!m_data_handler)
		m_data_handler = std::make_shared<DataHandler>();
	DataHandler::DataHandler::ResponseCallback response_callback = [callback](uint8_t function_code, uint16_t start_address, uint16_t length, const uint8_t* data) {
		if (callback) {
			std::vector<uint8_t> data_vector(data, data + length * function_code_2_bitsize((modbus_function_code)function_code));
			callback(function_code, start_address, length, data_vector);
		}
		};

	DataHandler::ErrorCallback adapted_error_callback = [error_callback](const modbus_request& req, const std::string& error_message) {
		if (error_callback) {
			error_callback(req, error_message);
		}
		};

	m_data_handler->set_response_callback(response_callback);
	m_data_handler->set_error_callback(adapted_error_callback);

}

bool ModbusSchedulerClient::connect(std::shared_ptr<IModbusConnection> connect)
{
	if (connect) {
		m_connection = connect;
		return m_connection->connect();
	}

	return false;
}

bool ModbusSchedulerClient::reconnect()
{
	if (m_connection) {
		m_connection->disconnect();
		return m_connection->connect();
	}
	return false;
}

bool ModbusSchedulerClient::is_connected() const
{
	if (m_connection)
		return m_connection->is_connected();
	return false;
}

void ModbusSchedulerClient::disconnect()
{
	if (m_connection) {
		m_connection->disconnect();
		m_connection.reset();
	}
}

int ModbusSchedulerClient::add_read(uint8_t function_code, uint16_t start_address, uint16_t length)
{
	if (m_param_manager)
	{
		m_param_manager->add_read_parameter(function_code, start_address, length);
		return RES_SUCCESS;
	}
		
	return RES_ERR_IPUT_PARM;
}

int ModbusSchedulerClient::optimize_read_groups()
{
	if(m_param_manager)
		return m_param_manager->optimize_read_groups();

	return RES_ERR_IPUT_PARM;
}

void ModbusSchedulerClient::start_scheduler(unsigned int interval_ms)
{
	if (m_read_scheduler)
		m_read_scheduler->stop();

	if (m_write_scheduler)
		m_write_scheduler->stop();

	m_read_scheduler = std::make_shared<ReadScheduler>(m_connection, m_param_manager, m_data_handler);
	m_write_scheduler = std::make_shared<WriteScheduler>(m_connection, m_param_manager, m_data_handler);
	m_read_scheduler->start();
	m_read_scheduler->set_read_interval(interval_ms);
	m_write_scheduler->start();
}

void ModbusSchedulerClient::stop_scheduler()
{
	if (m_read_scheduler) {
		m_read_scheduler->stop();
		m_read_scheduler.reset();
	}

	if (m_write_scheduler) {
		m_write_scheduler->stop();
		m_write_scheduler.reset();
	}
}
