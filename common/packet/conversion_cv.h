////=============================================================================
// 功能描述 : 数据类型和字符串转换
// 时间 : 2024/11/15 9:50
// 作者 : wangfei
// 联系 : ch.wangfei@gmail.com
//=============================================================================
#ifndef ORIENT_WANGFEI_CONVERSION_CHAR_VALUE_20240312
#include <stdint.h>

typedef union b1
{
	char str;
	bool num;
}b1;

typedef union ui16
{
	char str[2];
    uint16_t num;
}ui16;

typedef union ui32
{
	char str[4];
    uint32_t num;
}ui32;

typedef union i64
{
	char str[8];
    int64_t num;
}i64;

typedef union f32
{
	char str[4];
	float num;
}f32;

typedef union d64
{
	char str[8];
	double num;
}d64;

uint16_t atoui16(char* buf);
void ui16toa(char* buf, uint16_t num);

uint32_t atoui32(char* buf);
void ui32toa(char* buf, uint32_t num);

float atof32(char* buf);
void f32toa(char* buf, float num);

bool atob1(char buf);

int64_t atoi64(char* buf);
void i64toa(char* buf, int64_t num);

double atod64(char* buf);
void d64toa(char* buf, double num);

#endif//ORIENT_WANGFEI_CONVERSION_CHAR_VALUE_20240312
