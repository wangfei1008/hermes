#ifndef BASE_RT_STRING_H_20241021_WANGFEI
#define BASE_RT_STRING_H_20241021_WANGFEI
#include <string>
using namespace std;

class rt_string
{
public:
    rt_string(char* input, int len);
    rt_string(const std::string& input);

    //十进制转换为十六进制字符串
    string dec_2_hex();
    //十六进制转换为十进制字符串
    string hex_2_dec();
private:
    string m_input_string;
};
#endif
