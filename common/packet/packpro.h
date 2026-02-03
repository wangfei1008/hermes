
////=============================================================================
// 功能描述 : 通信网络包协议
// 时间 : 2024/11/15 9:37
// 作者 : wangfei
// 联系 : ch.wangfei@gmail.com
//=============================================================================
#if !defined(AFX_MPPACKPRO_H_20241116_INCLUDE)
#define AFX_MPPACKPRO_H_20241116_INCLUDE
#include <stdio.h>
#include <stdlib.h>
#include "conversion_cv.h"

typedef struct packageheader netpackhead;
typedef struct package netpack;

//package header start character
#define PACKAGE_HEADER_START_CHAR  '@'

//package header size
#define PACKAGE_HEADER_SIZE   7

//max recevie/send char
#define MAX_PACKET_SIZE 1024 * 1024


//network transmission of data packets header
typedef struct packageheader
{
    unsigned char start;      //start
    unsigned char type;       //command type
    unsigned char method;     //encryption and decryption methods
    uint32_t length;          //one package size
}packageheader;

//network package
typedef struct package
{
	netpackhead header;       //package header    
	char* data;               //data
    uint16_t check;           //check data
}package;

//create memory network package
netpack* np_create(unsigned char type, unsigned char method, uint32_t num, char* buffer);

//Package serialization
void np_serialize(netpack* pack, char* buffer, uint32_t& num);

//Deserialization of packages
netpack *np_deserialize(uint32_t num, char* buffer);

//release memory package
void np_release(netpack** pack);

#endif //!defined(AFX_MPPACKPRO_H_20241116_INCLUDE)
