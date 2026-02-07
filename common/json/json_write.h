#ifndef JSON_WRITE_H
#define JSON_WRITE_H

#define RAPIDJSON_HAS_STDSTRING 1

#include <string>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <type_traits>

using namespace rapidjson;

class json_write
{
public:
    json_write();
    ~json_write();

    //创建object
    bool front_object();
    bool front_object(const char* key, size_t len);
    bool front_object(const std::string& key);
    bool end_object();

    //创建数组
    bool front_array();
    bool front_array(const char* key, size_t len);
    bool front_array(const std::string& key);
    bool end_array();

    //设置值
    template<typename T>
    bool set_value(const char* key, size_t len, T value) {
        bool res = false;

        m_writer.Key(key, len);
        if(std::is_same<T, bool>::value)  res = m_writer.Bool(static_cast<bool>(value));
        if(std::is_same<T, int>::value)  res = m_writer.Int(value);
        if(std::is_same<T, double>::value) res = m_writer.Double(value);
        if(std::is_same<T, float>::value) res = m_writer.Double(value);
        //if(std::is_same<T, string>::value) res = m_writer.String(value);

        return res;
    }
    template<typename T>
    bool set_value(const std::string& key, T value){
        bool res = false;

        m_writer.Key(key);

        if(std::is_same<T, bool>::value) res = m_writer.Bool(static_cast<bool>(value));
        if(std::is_same<T, int>::value)  res = m_writer.Int(static_cast<int>(value));
        if (std::is_same<T, int64_t>::value)  res = m_writer.Int64(static_cast<int64_t>(value));
        if (std::is_same<T, uint64_t>::value)  res = m_writer.Uint64(static_cast<uint64_t>(value));
        if(std::is_same<T, double>::value) res = m_writer.Double(static_cast<double>(value));
        if(std::is_same<T, float>::value) res = m_writer.Double(static_cast<double>(value));
        //if(std::is_same<T, string>::value) res = m_writer.String(static_cast<string>(value));

        return res;
    }
    bool set_value_string(const std::string& key, const std::string& value){
        bool res = false;

        m_writer.Key(key);
        res = m_writer.String(static_cast<std::string>(value));

        return res;
    }
    //设置值，正常用于设置数组值
    template<typename T>
    bool set_value(T value) {
        bool res = false;

        if(std::is_same<T, bool>::value)  res = m_writer.Bool((bool)value);
        if(std::is_same<T, int>::value)  res = m_writer.Int((int)value);
        if(std::is_same<T, double>::value) res = m_writer.Double((double)value);
        if(std::is_same<T, float>::value) res = m_writer.Double((double)value);
        //if(std::is_same<T, string>::value) res = m_writer.String((string)value);

        return res;
    }
    bool set_value_string(const std::string& value){
        return m_writer.String(value);
    }

    const char* data() const {
        return m_buffer.GetString();
    }
    size_t data_length() const{
        return m_buffer.GetLength();
    }

    bool save(const std::string& path);
private:
    StringBuffer m_buffer;
    Writer<StringBuffer> m_writer;
};

#endif // JSON_WRITE_H
