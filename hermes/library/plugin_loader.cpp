#include "plugin_loader.h"
#include <memory>

PluginLoader& PluginLoader::instance()
{
    static PluginLoader instance;
    return instance;
}

IComponent* PluginLoader::create(const std::string& libname)
{
    // 1. 使用 try_emplace 尝试插入
    // 如果 libname 已存在，它什么都不做；如果不存在，则调用 ComponentLoader(libname) 构造新成员
    auto [itor, inserted] = m_libs.try_emplace(libname, libname);

    // 2. 无论是否是新插入的，直接返回对应的组件
    return itor->second.create();
}

void PluginLoader::release(const std::string& libname, IComponent* pcomponent)
{
    auto itor = m_libs.find(libname);
    if (itor != m_libs.end())
    {
        // 假设 ComponentLoader::release 返回 true 表示引用计数归零，可以卸载
        if (itor->second.release(pcomponent))
            m_libs.erase(itor);
    }
}
