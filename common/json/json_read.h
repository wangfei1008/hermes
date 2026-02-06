#ifndef JSON_READ_H
#define JSON_READ_H

#define RAPIDJSON_HAS_STDSTRING 1
#include <string>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/pointer.h>

using namespace rapidjson;

class json_read
{
public:
    json_read();
    ~json_read();

    //1、初始化
    int init(const char* buffer, size_t len);
    int init(const std::string& buffer);
    int init(const rapidjson::Value* value);
    int init_file(const std::string& path);

    //2、判断释放存在对应key
    bool has_key(const char* key);

    //3、获取key值
    template<typename T>
    T get_value(const char* key) const {
        return get_value<T>(&m_doc, key);
    }
    template<typename T>
    T get_value(const std::string key) const {
        return get_value<T>(&m_doc, key.c_str());
    }
    template<typename T>
    T get_value(const rapidjson::Value *member, const char* key) const {
        //if (member->HasMember(key) && (*member)[key].Is<T>()) {
        //    return (*member)[key].Get<T>();
        //}

        if (member->HasMember(key)) {
            auto n = member->FindMember(key);
            if (n->value.Is<T>())
                return n->value.Get<T>();

            if (std::is_same<T, double>::value)
            {
                if (n->value.Is<int>())
                {
					double v = n->value.Get<int>();
					T* _v = (T*)(&v);
					return *_v;
				}
            }
            return T();
        }
        return T();
    }



    template<typename T>
    T get_value(const rapidjson::Value *member, std::string key) const {
        if (member->HasMember(key) && (*member)[key].Is<T>()) {
            return (*member)[key].Get<T>();
        }
        return T();
    }

    const rapidjson::Value *get_value_pointer(const rapidjson::Value *member, const char* key);
    const rapidjson::Value *get_value_pointer(const rapidjson::Value *member, const std::string& key);
    const rapidjson::Value *get_value_pointer(const char* key);
    const rapidjson::Value *get_value_pointer(const std::string& key);
    const rapidjson::Value *get_value_pointer();

    std::string to_string(const rapidjson::Value *member);
private:
    Document m_doc;
};

#endif // JSON_READ_H
