#include "modbus_parameter.h"

modbus_bitsize int2bitsize(int value)
{
    int count = value >> 3;
    return (modbus_bitsize)count;
}

modbus_bitsize function_code_2_bitsize(modbus_function_code code)
{
	switch (code) {
	case modbus_function_code::COILS:
	case modbus_function_code::DISCRETE_INPUTS:
	case modbus_function_code::SINGLE_COIL:
	case modbus_function_code::MULTIPLE_COIL:
		return modbus_bitsize::BIT_INT8;
	default:
		return modbus_bitsize::BIT_INT16;
	}
}

modbus_parameter_request::modbus_parameter_request() {
}

modbus_parameter_request::modbus_parameter_request(const modbus_request& other)
	: modbus_request(other)
	, modbus_parameter_data() {
}

modbus_parameter_request::modbus_parameter_request(const modbus_parameter_request& other)
	: modbus_request(other)
	, modbus_parameter_data(other) {
}

modbus_parameter_request& modbus_parameter_request::operator=(const modbus_parameter_request& other) {
	if (this != &other)
	{
		modbus_request::operator=(other);
		modbus_parameter_data::operator=(other);;
	}
	return *this;
}

uint8_t* modbus_parameter_request::malloc_data() {
	return modbus_parameter_data::malloc_data(this->get_length(), this->get_bit_size());
}

modbus_request& modbus_parameter_request::get_parent() const
{
	return *const_cast<modbus_request*>(static_cast<const modbus_request*>(this));
}

int modbus_group_request::add_parameter(const modbus_request& param)
{
	// 动态更新分组范围
	if (parameters.empty())
	{
		this->set_address(param.get_address());
		this->set_length(param.get_length());
		this->set_code(param.get_code());
	}
	else
	{
		//确保组内参数 bit_size 一致,且新参数起始地址必须大于等于当前组起始地址
		if (param.get_address() < this->get_address() || param.get_code() != this->get_code()) return -1;

		// 更新数量
		uint16_t count = param.get_address() + param.get_length() - this->get_address();
		this->set_length(count);
	}

	parameters.push_back(std::ref(const_cast<modbus_request&>(param)));

	return 0;
}

int modbus_group_request::add_parameter(const modbus_parameter_request& param)
{
	return add_parameter(param.get_parent());
}

std::vector<modbus_parameter_request> modbus_group_request::splite_to_parameters()
{
    std::vector<modbus_parameter_request> params;
    for (const auto& param_ref : parameters)
    {
		modbus_parameter_request param = param_ref;

        uint8_t* add = param.malloc_data();
        if (add != NULL)
        {
            int start = (param.get_address() - get_address()) * param.get_bit_size();
            int len = param.get_length() * param.get_bit_size();
            memcpy(add, get_data() + start, len);
        }

        params.push_back(param);

        param.free_data();
    }
    return params;
}

uint8_t *modbus_group_request::malloc_data()
{
    return modbus_parameter_data::malloc_data(this->get_length(), this->get_bit_size());
}
