
#include "packpro.h"
#include <stdio.h>
#include <memory.h>
#ifdef __APPLE__
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif
#include "checksum.h"


static unsigned char get_packet_header_start(char* data)
{
    return *data;
}

static void set_packet_header_start(char* data)
{
    *data = PACKAGE_HEADER_START_CHAR;
}

static unsigned char get_packet_header_type(char* data)
{
    return *(data + 1);
}

static void set_packet_header_type(char* data, unsigned char value)
{
    *(data + 1) = value;
}

static unsigned char get_packet_header_method(char* data)
{
    return *(data + 2);
}

static void set_packet_header_method(char* data, unsigned char value)
{
    *(data + 2) = value;
}

static uint32_t get_packet_header_length(char* data)
{
    return atoui32(data + 3);
}

static void set_packet_header_length(char* data, uint32_t value)
{
    ui32toa(data + 3, value);
}

static char* get_packet_data(char* data)
{
    if(0 == get_packet_header_length(data)) return NULL;

	return data + PACKAGE_HEADER_SIZE;
}

static void set_packet_data(char* data, int num, char* src)
{
	if (src == NULL || num == 0) return;
	memcpy(data + PACKAGE_HEADER_SIZE, src, num);
}

static uint16_t get_packet_check(char* data)
{
    uint32_t len = get_packet_header_length(data);

    return atoui16(data + PACKAGE_HEADER_SIZE + len);
}

static void set_packet_check(char* data, uint16_t check)
{
    uint32_t len = get_packet_header_length(data);

    ui16toa(data + PACKAGE_HEADER_SIZE + len, check);
}

netpack* np_create(unsigned char type, unsigned char method, uint32_t num, char* buffer)
{
    netpack* pack = (netpack*)malloc(sizeof(netpack));
    if (pack == NULL)
    {
        printf("create packet error01");
        return NULL;
    }

    //1、header
    pack->header.start = PACKAGE_HEADER_START_CHAR;
    pack->header.type = type;
    pack->header.method = method;
    pack->header.length = num;

    //2、 data
    pack->data = NULL;
    if (num > 0)
    {
        pack->data = (char*)malloc(pack->header.length);
        if (pack->data == NULL)
        {
            printf("create packet error02");
            free(pack);
            pack = NULL;
            return NULL;
        }
        memset(pack->data, '\0', pack->header.length);
        memcpy(pack->data, buffer, num);
    }

    //3、check
    pack->check = num > 0 ? crc_16((const unsigned char*)buffer, (size_t)num) : 0;
    return pack;
}

void np_release(netpack** pack)
{
    if((*pack) != NULL)
    {
        if((*pack)->data != NULL)
            free((*pack)->data);
        (*pack)->data = NULL;
        free(*pack);
        *pack = NULL;
    }
}

netpack *np_deserialize(uint32_t num, char* buffer)
{
    //验证包是否正确
    if(num <= (PACKAGE_HEADER_SIZE + sizeof(uint16_t))) return NULL;
    unsigned char start = get_packet_header_start(buffer);
    unsigned char type = get_packet_header_type(buffer);
    unsigned char method = get_packet_header_method(buffer);
    uint32_t len = get_packet_header_length(buffer);
    if((start != PACKAGE_HEADER_START_CHAR) || (num != len + PACKAGE_HEADER_SIZE + sizeof(uint16_t))) return NULL;
    char* data = get_packet_data(buffer);
    uint16_t c1= get_packet_check(buffer);
    uint16_t c2= crc_16((const unsigned char*)data, (size_t)num);
    if(c1 != c2)  return NULL;

    //创建包
    return np_create(type, method, len, data);
}

void np_serialize(netpack *pack, char *buffer, uint32_t &num)
{
    if(pack == NULL || buffer == NULL)
    {
        num = 0;
        return ;
    }

    set_packet_header_start(buffer);
    set_packet_header_type(buffer, pack->header.type);
    set_packet_header_method(buffer, pack->header.method);
    set_packet_header_length(buffer, pack->header.length);
    set_packet_data(buffer, pack->header.length, pack->data);
    set_packet_check(buffer, pack->check);

    num = PACKAGE_HEADER_SIZE + pack->header.length + sizeof(uint16_t);
}
