#include "json_write.h"

json_write::json_write():m_writer(m_buffer)
{
    
}

json_write::~json_write()
{
}

bool json_write::front_object()
{
    return m_writer.StartObject();
}

bool json_write::front_object(const char *key, size_t len)
{
    if(m_writer.Key(key, (SizeType)len))
        return m_writer.StartObject();
    return false;
}

bool json_write::front_object(const string &key)
{
    return front_object(key.c_str(), key.size());
}

bool json_write::end_object()
{
    return m_writer.EndObject();
}

bool json_write::front_array()
{
    return m_writer.StartArray();
}

bool json_write::front_array(const char *key, size_t len)
{
    if(m_writer.Key(key, (SizeType)len))
        return m_writer.StartArray();
    return false;
}

bool json_write::front_array(const string &key)
{
    return front_array(key.c_str(), key.size());
}

bool json_write::end_array()
{
    return m_writer.EndArray();
}

bool json_write::save(const string &path)
{
    FILE* pfile = fopen(path.c_str(), "wb");
    if(pfile != NULL)
    {
        fwrite(m_buffer.GetString(), 1, m_buffer.GetLength(), pfile);
        fclose(pfile);
        return true;
    }
    return false;
}
