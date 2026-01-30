#include "plugin_loader.h"

PluginLoader& PluginLoader::instance()
{
    static PluginLoader instance;
    return instance;
}

IComponent* PluginLoader::create(const std::string& libname)
{
    auto itor = m_libs.find(libname);
    if (itor != m_libs.end())
        return itor->second.create();

    unique_ptr<ComponentLoader> handle(new ComponentLoader(libname));
    m_libs.insert(make_pair(libname, handle));

    return handle.create();
}

void PluginLoader::release(const std::string& libname, IComponent* pcomponent)
{
    auto itor = m_libs.find(libname);
    if (itor != m_libs.end())
    {
        if (itor->second.release(pcomponent))
            m_libs.erase(itor);
    }
}
