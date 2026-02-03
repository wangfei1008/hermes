#include <string.h>
#include "conversion_cv.h"

uint16_t atoui16(char* buf)
{
    ui16 un;
	memcpy(un.str, buf, 2);
	return un.num;
}

void ui16toa(char* buf, uint16_t num)
{
    ui16 un;
	un.num = num;
	memcpy(buf, un.str, 2);
}

uint32_t atoui32(char* buf)
{
    ui32 un;
	memcpy(un.str, buf, 4);
	return un.num;
}

void ui32toa(char* buf, uint32_t num)
{
    ui32 un;
	un.num = num;
	memcpy(buf, un.str, 4);
}

float atof32(char* buf)
{
	f32 un;
	memcpy(un.str, buf, sizeof(float));
	return un.num;
}

void f32toa(char* buf, float num)
{
	f32 un;
	un.num = num;
	memcpy(buf, un.str, 4);
}

bool atob1(char buf)
{
	b1 un;
	un.str = buf;
	return un.num;
}

int64_t atoi64(char* buf)
{
	i64 un;
	memcpy(un.str, buf, 8);
	return un.num;
}

void i64toa(char* buf, int64_t num)
{
	i64 un;
	un.num = num;
	memcpy(buf, un.str, 8);
}

double atod64(char* buf)
{
	d64 un;
	memcpy(un.str, buf, 8);
	return un.num;
}

void d64toa(char* buf, double num)
{
	d64 un;
	un.num = num;
	memcpy(buf, un.str, 8);
}
