#ifndef BASE_RT_STRING_H_20241021_WANGFEI
#define BASE_RT_STRING_H_20241021_WANGFEI
#include <string>

class rt_string
{
public:
    rt_string(char* input, int len);
    rt_string(const std::string& input);

    //十进制转换为十六进制字符串
    std::string dec_2_hex();
    //十六进制转换为十进制字符串
    std::string hex_2_dec();
private:
    std::string m_input_string;
};
#endif
