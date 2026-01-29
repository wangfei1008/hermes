#ifndef DB_MODELS_H
#define DB_MODELS_H

#include <string>
#include <vector>

//对应表:device_streams
struct StreamDTO
{
    int id;
    int device_id;
    int is_active;
    std::string stream_name;
};

//对应表:stream_components
struct ComponectDTO
{
    int id;
    int stream_id;
    int order_index;
    std::string lib_name;
    std::string comp_config;
    int output_to_next;
};
// 对应表: data_point
struct DataPointDTO
{
    int point_id;
    int device_id;
    std::string type;
    int address;
    std::string value_type;
    double scale;
    std::string expression;
    std::string description;
    int control; // 0 or 1
    std::string created_at;
    std::string updated_at;
};

// 对应表: protocol_config
struct ProtocolConfigDTO 
{
    int id;
    std::string host;
    int port;
    int station;
    int baud;
	int data_bits;
	double stop_bits;
    std::string parity;
    int timeout_ms;
    int retry;
    std::string created_at;
};

// 对应表: device
struct DeviceDTO
{
    int id;
    std::string name;
    std::string protocol;
    std::string created_at;
    std::string updated_at;
    ProtocolConfigDTO protocol_config;
};



#endif // DB_MODELS_H