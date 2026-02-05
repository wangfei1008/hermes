#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include <functional>
#include <string>
#include "modbus_parameter.h"

class DataHandler {
public:
    using ResponseCallback = std::function<void(uint8_t, uint16_t, uint16_t, const uint8_t*)>;
    using ErrorCallback = std::function<void(const modbus_request&, const std::string&)>;

    DataHandler();

    void set_response_callback(ResponseCallback callback);
    void set_error_callback(ErrorCallback callback);

    void process_response(const std::vector<modbus_parameter_request>& request);

    void handle_error(const modbus_request& req, const std::string& error_message);

private:
    ResponseCallback m_response_callback;
    ErrorCallback m_error_callback;
};

#endif // DATA_HANDLER_H
