#ifndef COMPONENT_LOADER_H
#define COMPONENT_LOADER_H

#include "library_loader.h"
#include "../core/i_component.h"
#include <list>

#define  LIB_FUN_CREATE          "create_lib"
#define  LIB_FUN_RELEASE         "release_lib"

class ComponentLoader
{
    typedef bool (*CreateComponent)(IComponent**);
    typedef bool (*ReleaseComponent)(IComponent**);
public:
    ComponentLoader(const std::string& libname);

    //创建新组件
    IComponent* create();
    //释放组件，当无组件时自动卸载动态库
    bool release(IComponent* pcomponent);
    //释放所有组件，且卸载动态库
    bool release();

    ComponentLoader& operator=(const ComponentLoader& other) = default;
private:
    LibraryLoader* load(const std::string& libname);
    IComponent* init_component();
    bool release_component(IComponent* pcomponent);

private:
    LibraryLoader* m_plib;                                                       //动态库句柄
    CreateComponent m_create_func;                                               //创建组件函数
    ReleaseComponent m_release_func;                                             //释放组件函数
    std::list<IComponent*> m_list;                                               //组件指针队列
};

#endif // COMPONENT_LOADER_H
