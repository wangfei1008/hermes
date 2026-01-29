#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H
#include "component_loader.h"
#include <map>

class PluginLoader
{
public:
    static PluginLoader& instance();

    //根据库文件名，获取新的组件。可以自动加载库
    IComponent* create(const std::string& libname);

    //根据库文件名、组件，释放组件。可以自动卸载库。
    void release(const std::string& libname, IComponent* pcomponent);
private:
    PluginLoader() = default;
    ~PluginLoader() = default;
private:
    std::map<std::string, ComponentLoader> m_libs;
};
#endif