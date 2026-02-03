#ifndef BASE_LOCK_H_WANGFEI_20241021_
#define BASE_LOCK_H_WANGFEI_20241021_

#include <shared_mutex>
using namespace  std;

typedef std::shared_lock<std::shared_mutex>    ReadLock;
typedef std::lock_guard<std::shared_mutex>     WriteLock;
typedef std::lock_guard<std::mutex>            NormalLock;

#endif
