#ifndef COMPONENT_EXPORT_H
#define COMPONENT_EXPORT_H

// 处理 Windows (MSVC/MinGW)
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#  if defined(COM_EXPORT_LIBRARY)
#    define COM_EXPORT __declspec(dllexport)
#  else
#    define COM_EXPORT __declspec(dllimport)
#  endif

// 处理 Linux/macOS (GCC/Clang)
#else
#  if defined(__GNUC__) && __GNUC__ >= 4
#    define COM_EXPORT __attribute__((visibility("default")))
#  else
#    define COM_EXPORT
#  endif
#endif

#endif // COMPONENT_EXPORT_H