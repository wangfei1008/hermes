#include "library_loader.h"
#include "log/log.h"

LibraryLoader::LibraryLoader(const std::string& file_name)
    : m_file_name(file_name) 
{
}

LibraryLoader::~LibraryLoader() 
{
    if (!(m_hints & PreventUnloadHint))
        unload();
}

bool LibraryLoader::load()
{
    if (is_loaded()) return true;
    if (m_file_name.empty())
    {
        m_error_str = "File name is empty";
		LOGERROR("[Library]LibraryLoader load failed: %s", m_error_str.c_str());
        return false;
    }

    std::string full_path = decorate_path(m_file_name);

#ifdef _WIN32
    m_handle = LoadLibraryA(full_path.c_str());
    if (!m_handle) {
        m_error_str = "Win32 Error Code: " + std::to_string(GetLastError());
		LOGERROR("[Library]LibraryLoader load failed: %s, path= %s", m_error_str.c_str(), full_path.c_str());
    }
#else
    int flags = RTLD_LAZY;
    if (m_hints & ResolveAllSymbolsHint) flags = RTLD_NOW;
    if (m_hints & ExportExternalSymbolsHint) flags |= RTLD_GLOBAL;
    if (m_hints & PreventUnloadHint) flags |= RTLD_NODELETE;

    m_handle = dlopen(fullPath.c_str(), flags);
    if (!m_handle) {
        const char* err = dlerror();
        m_error_str = err ? err : "Unknown DL error";
    }
#endif
    return m_handle != nullptr;
}

bool LibraryLoader::unload() 
{
    if (!m_handle) return false;
    bool ok;
#ifdef _WIN32
    ok = FreeLibrary(m_handle);
#else
    ok = (dlclose(m_handle) == 0);
#endif
    if (ok) m_handle = nullptr;
    return ok;
}

bool LibraryLoader::is_loaded() const 
{
    return m_handle != nullptr;
}

void* LibraryLoader::resolve(const char* symbol) 
{
    if (!m_handle && !load()) return nullptr;
#ifdef _WIN32
    return (void*)GetProcAddress(m_handle, symbol);
#else
    return dlsym(m_handle, symbol);
#endif
}

void* LibraryLoader::resolve(const std::string& file_name, const char* symbol) 
{
    LibraryLoader loader(file_name);
    return loader.resolve(symbol);
}

// 属性设置
void LibraryLoader::set_file_name(const std::string& file_name) 
{
    m_file_name = file_name;
}

std::string LibraryLoader::file_name() const 
{ 
    return m_file_name;
}

void LibraryLoader::set_load_hints(int hints) 
{ 
    m_hints = hints; 
}

std::string LibraryLoader::error_string() const 
{ 
    return m_error_str;
}

std::string LibraryLoader::decorate_path(const std::string& path) 
{
    // 如果已经包含后缀名，则不处理
    if (path.find('.') != std::string::npos) return path;

#ifdef _WIN32
    return path + ".dll";
#else
    // Linux 下处理 lib 前缀逻辑
    std::string decorated = path;
    size_t lastSlash = path.find_last_of('/');
    std::string name = (lastSlash == std::string::npos) ? path : path.substr(lastSlash + 1);

    if (name.compare(0, 3, "lib") != 0) {
        if (lastSlash == std::string::npos) {
            decorated = "lib" + path;
        }
        else {
            decorated.insert(lastSlash + 1, "lib");
        }
    }
    return decorated + ".so";
#endif
}