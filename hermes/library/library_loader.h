#ifndef LIBRARY_LOADER_HPP
#define LIBRARY_LOADER_HPP

#include <string>
#include <iostream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
typedef HMODULE LibHandle;
#else
#include <dlfcn.h>
typedef void* LibHandle;
#endif

class LibraryLoader 
{
public:
    enum LoadHint {
        None = 0x0,
        ResolveAllSymbolsHint = 0x01,     // 对应 RTLD_NOW
        ExportExternalSymbolsHint = 0x02, // 对应 RTLD_GLOBAL
        PreventUnloadHint = 0x08          // 对应 RTLD_NODELETE
    };

    explicit LibraryLoader(const std::string& file_name = "");
    ~LibraryLoader();

    // 禁用拷贝语义
    LibraryLoader(const LibraryLoader&) = delete;
    LibraryLoader& operator=(const LibraryLoader&) = delete;

    // 基础操作
    bool load();
    bool unload();
    bool is_loaded() const;

    // 符号解析 (兼容原生写法)
    void* resolve(const char* symbol);

    // 模板解析 (类型安全，推荐用法)
    template <typename T>
    T resolve_as(const char* symbol) {
        return reinterpret_cast<T>(resolve(symbol));
    }

    // 静态快捷解析
    static void* resolve(const std::string& file_name, const char* symbol);

    // 属性设置
    void set_file_name(const std::string& file_name);
    std::string file_name() const;
    void set_load_hints(int hints);
    std::string error_string() const;

private:
    std::string decorate_path(const std::string& path);

private:
    LibHandle m_handle = nullptr;
    std::string m_file_name;
    std::string m_error_str;
    int m_hints = 0;
};

#endif