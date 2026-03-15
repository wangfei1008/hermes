#include "library_loader.h"
#include "log/log.h"
#include <string>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

// 返回可执行文件所在目录的 ../lib（插件目录），失败返回空串
static std::string get_plugin_lib_dir()
{
    std::string exe_path;
#ifdef __APPLE__
    char buf[4096];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) != 0) return {};
    exe_path = buf;
#elif defined(__linux__)
    char buf[4096];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return {};
    buf[n] = '\0';
    exe_path = buf;
#else
    return {};
#endif
    size_t last = exe_path.find_last_of('/');
    if (last == std::string::npos) return {};
    return exe_path.substr(0, last) + "/../lib";
}

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

    const char* path_to_try = full_path.c_str();
    std::string plugin_path;
    if (full_path.find('/') == std::string::npos) {
        std::string lib_dir = get_plugin_lib_dir();
        if (!lib_dir.empty()) {
            plugin_path = lib_dir + "/" + full_path;
            path_to_try = plugin_path.c_str();
        }
    }

    m_handle = dlopen(path_to_try, flags);
    if (!m_handle) {
        const char* err = dlerror();
        m_error_str = err ? err : "Unknown DL error";
        LOGERROR("[Library]LibraryLoader load failed: %s, path= %s", m_error_str.c_str(), path_to_try);
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
    // Linux / macOS：lib 前缀 + 平台后缀
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
#ifdef __APPLE__
    return decorated + ".dylib";
#else
    return decorated + ".so";
#endif
#endif
}