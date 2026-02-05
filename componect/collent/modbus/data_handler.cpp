#include "data_handler.h"

DataHandler::DataHandler()
    : m_response_callback(NULL)
    , m_error_callback(NULL)
{
}

void DataHandler::set_response_callback(ResponseCallback callback)
{
    m_response_callback = callback;
}

void DataHandler::set_error_callback(ErrorCallback callback)
{
    m_error_callback = callback;
}

void DataHandler::process_response(const std::vector<modbus_parameter_request>& request)
{
    for(auto it : request){
        if(m_response_callback)
            m_response_callback(it.get_code(), it.get_address(), it.get_length(), it.get_data());
    }
}

void DataHandler::handle_error(const modbus_request& req, const std::string& error_message)
{
    if(m_error_callback)
        m_error_callback(req, error_message);
}
