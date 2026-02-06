#ifndef MODBUS_PARAMETER_H
#define MODBUS_PARAMETER_H

#include <vector>
#include "data/variant.h"

#define GETSET(type, name) \
private: \
    type name; \
public: \
    type get_##name() const { return name; } \
    void set_##name(const type& wf_value) { name = wf_value; }

//read coil/register bit size
typedef enum modbus_bitsize
{
    BIT_INT8 = 1,
    BIT_INT16
}modbus_bitsize;

// 访问模式
typedef enum access_mode
{
    READ_ONLY = 0,
    WRITE_ONLY,
    READ_WRITE
}access_mode;

// 功能码
typedef enum modbus_function_code
{
	NO_FUNCTION = 0,           // 无功能码
    COILS = 1,                  // 线圈(read)
    DISCRETE_INPUTS = 2,       // 离散输入(read)
    HOLDING_REGISTERS = 3,     // 保持寄存器(read)
    INPUT_REGISTERS = 4,       // 输入寄存器(read)    
    SINGLE_COIL = 5,
    SINGLE_REGISTER = 6,
    MULTIPLE_COIL = 15,
    MULTIPLE_REGISTER = 16
}modbus_function_code;

typedef enum modbus_conntype{
    IP_TCP,
    RTU
}modbus_conntype;

// 连接信息
typedef union modbus_conninfo
{
    struct iptcp{
        char ip[1024];
        int port;
    };
    struct rtu{
        char device[1024];
        int baud;
        char parity;
        int data_bit;
        int stop_bit;
    };
    struct iptcp tcp;
    struct rtu rtu;
}modbus_conninfo;

//
typedef struct modbus_conn {
    // Slave address
    int slave;

    //connect type,0:ip/tcp, 1:rtu
    int type;

    //connect info
    modbus_conninfo info;
}modbus_conn;

class modbus_parameter_data
{
public:
    modbus_parameter_data():data(NULL), count(0) {}
    ~modbus_parameter_data() { free_data(); }
    modbus_parameter_data(const modbus_parameter_data& other)
    {
		this->data = NULL;
        this->count = other.get_count();
        if(NULL != malloc_data(this->count))
            memcpy(data, other.data, count);
    }

    modbus_parameter_data& operator=(const modbus_parameter_data& other)
    {
        if (this != &other)
        {
            this->data = NULL;
            this->count = other.get_count();
            if (NULL != malloc_data(this->count))
                memcpy(data, other.data, count);
        }
        return *this;
    }

    uint8_t* malloc_data(uint16_t len, modbus_bitsize bit_size = BIT_INT8)
    {
        if (data != NULL) free_data();

        if (len == 0) return NULL;
        count = len * bit_size;
        data = (uint8_t*)malloc(sizeof(uint8_t) * count);
        return data;
    }

    virtual uint8_t* malloc_data() = 0;

    void free_data()
    {
        if (data != NULL) free(data);
        data = NULL;
    }

    uint8_t* get_data() const
    {
        return data;
    }
private:
    uint8_t* data;
    GETSET(uint16_t, count);                    // 数量
};

class modbus_request
{
    GETSET(modbus_function_code, code);         //功能码
    GETSET(uint16_t, address);                  // Modbus 地址
    GETSET(uint16_t, length);                   // 数量
public:
	modbus_request() : code(NO_FUNCTION), address(0), length(0) {
    }

	modbus_request(const modbus_function_code& _code, uint16_t _address, uint16_t _length)
		: code(_code), address(_address), length(_length) {
	}

	modbus_request(const modbus_request& other)	{
		this->code = other.get_code();
		this->address = other.get_address();
		this->length = other.get_length();
	}

    modbus_request& operator=(const modbus_request& other){
		if (this != &other)
		{
			this->code = other.get_code();
			this->address = other.get_address();
			this->length = other.get_length();
		}
		return *this;
    }

    bool operator<(const modbus_request& other) const
    {
        if (code != other.code)
            return code < other.code;
        return address < other.address;
    }

    modbus_bitsize get_bit_size(){
        modbus_bitsize bit_size;
        //switch (code) {
        //case modbus_function_code::COILS:
        //case modbus_function_code::DISCRETE_INPUTS:
        //case modbus_function_code::SINGLE_COIL:
        //case modbus_function_code::MULTIPLE_COIL:
        //    bit_size = modbus_bitsize::BIT_INT8;
        //    break;
        //default:
            bit_size = modbus_bitsize::BIT_INT16;
        //    break;
        //}
        return bit_size;
    }
};

class modbus_parameter_request : public modbus_request, public modbus_parameter_data
{
public:
    modbus_parameter_request();

    modbus_parameter_request(const modbus_request& other);

    modbus_parameter_request(const modbus_parameter_request& other);

    modbus_parameter_request& operator=(const modbus_parameter_request& other);

    uint8_t* malloc_data() override;

	modbus_request& get_parent() const;
};

class modbus_group_request : public modbus_request, public modbus_parameter_data
{
public:
    // 添加参数到分组
    int add_parameter(const modbus_request& param);
    int add_parameter(const modbus_parameter_request& param);

    // 组读取内存块拆分为多个参数的内存块
    std::vector<modbus_parameter_request> splite_to_parameters();

    //分配内存
    uint8_t* malloc_data() override;

private:
    std::vector<modbus_request> parameters;
};


class modbus_parameter : public modbus_request
{
    GETSET(std::string, name);                  // 参数名称
	GETSET(wf::Variant::Kind, type);            // 数据类型
	GETSET(std::string, order);                 // 数据顺序
public:
    modbus_parameter() {}
	modbus_parameter(const std::string& _name, wf::Variant::Kind _type, const std::string& _order, const modbus_function_code& _code, uint16_t _address, uint16_t _length)
		: modbus_request(_code, _address, _length), name(_name), type(_type), order(_order) {
	}
    modbus_parameter(const modbus_parameter& other)
		: modbus_request(other)
	{
		this->name = other.get_name();
		this->type = other.get_type();
		this->order = other.get_order();
	}
    modbus_parameter& operator=(const modbus_parameter& other)
    {
        if (this != &other)
        {
            modbus_request::operator=(other);
			this->name = other.get_name();
            this->type = other.get_type();
            this->order = other.get_order();
        }
        return *this;
    }
};

modbus_bitsize int2bitsize(int value);
modbus_bitsize function_code_2_bitsize(modbus_function_code code);

#endif // MODBUS_PARAMETER_H
