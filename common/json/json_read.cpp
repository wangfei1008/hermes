#include "json_read.h"
#include <rapidjson/filereadstream.h>
#include <rapidjson/pointer.h>
#include <rapidjson/writer.h>
#include <iostream>
#include <fstream>

json_read::json_read()
{

}

json_read::~json_read()
{

}

int json_read::init(const char *buffer, size_t len)
{
    //if(!m_doc.Empty()) m_doc.Clear();
    m_doc.Parse(buffer, len);
    return m_doc.HasParseError() ? -1 : 0;
}

int json_read::init(const std::string &buffer)
{
    //if(!m_doc.Empty()) m_doc.Clear();
    m_doc.Parse(buffer);

    return m_doc.HasParseError() ? -1 : 0;
}

int json_read::init(const rapidjson::Value* value)
{
    if (!value) return -1;
    m_doc.CopyFrom(*value, m_doc.GetAllocator());
    return m_doc.HasParseError() ? -1 : 0;
}

int json_read::init_file(const std::string& path)
{
    std::ifstream fs(path);
    std::string str((std::istreambuf_iterator<char>(fs)), (std::istreambuf_iterator<char>()));
    if(fs.is_open()) fs.close();

    return init(str);
}

bool json_read::has_key(const char *key)
{
    return m_doc.HasMember(key);
}

const Value *json_read::get_value_pointer(const Value *member, const char *key)
{
    std::string _key("/");
    _key += key;
    rapidjson::Pointer pointer(_key);
    if (!pointer.IsValid()) return NULL;

    return rapidjson::GetValueByPointer(*member, pointer);
}

const Value *json_read::get_value_pointer(const Value *member, const std::string &key)
{
    std::string _key("/");
    _key += key;
    rapidjson::Pointer pointer(_key);
    if (!pointer.IsValid()) return NULL;

    return rapidjson::GetValueByPointer(*member, pointer);
}

const Value *json_read::get_value_pointer(const char *key)
{    
    return get_value_pointer(&m_doc, key);
}

const Value *json_read::get_value_pointer(const std::string &key)
{
    return get_value_pointer(&m_doc, key.c_str());
}

const Value *json_read::get_value_pointer()
{
    return &m_doc;
}

std::string json_read::to_string(const Value *member)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    member->Accept(writer);

    return buffer.GetString();
}
