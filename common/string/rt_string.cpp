#include "rt_string.h"
#include <sstream>
#include <iomanip>

rt_string::rt_string(char *input, int len)
{
    m_input_string.clear();
    m_input_string = string(input, len);
}

rt_string::rt_string(const string &input)
{
    m_input_string = input;
}

std::string rt_string::dec_2_hex()
{
    std::ostringstream oss;
    for (char c : m_input_string) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    return oss.str();
}

std::string rt_string::hex_2_dec()
{
    std::stringstream result;

    for (size_t i = 0; i < m_input_string.length(); i += 2)
    {
        std::string byteString = m_input_string.substr(i, 2);
        char byte = (char)strtol(byteString.c_str(), nullptr, 16);
        result << byte;
    }

    return result.str();
}
