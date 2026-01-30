#ifndef MESSAGE_BUS_TYPES_H
#define MESSAGE_BUS_TYPES_H

//消息类型
enum MessageType
{
    MESSAGE_NONE = 0,
    //1、启动
    MESSAGE_SETUP = 1,
    //2、通用（对内），包括服务器信息,开始转发
    MESSAGE_PRIVATE_SERVER = 10,
    MESSAGE_PRIVATE_SEND = 11,
    //3、通用（对内、外开发），包括启动、停止、配置、写控制
    MESSAGE_PUBLICE_START = 20,
    MESSAGE_PUBLICE_STOP = 21,
    MESSAGE_PUBLICE_CONFIG = 22,
    MESSAGE_PUBLICE_WRITE = 23,
    MESSAGE_EXCEPTION = 99
};

//消息等级
enum MessagePriority
{
    MP_LOW = 0,
    MP_NORMAL,
    EP_HIGH
};

#endif