#ifndef __MUTEXLOCK_H__ //防止多次编译的问题
#define __MUTEXLOCK_H__

#include "NoCopyable.h"
#include <pthread.h>

class MutexLock:public NoCopyable{
	public:
		MutexLock();
		~MutexLock();
		//上锁
		void lock();
		//释放锁
		void unlock();
		//返回私有成员变量的地址
		pthread_mutex_t* getMutexLockPtr(){
			return &_mutex;
		}
	private:
		pthread_mutex_t _mutex;
};

class MutexLockGuard {
	public:
		MutexLockGuard(MutexLock& mutex)
		: _mutex(mutex)
		{
			_mutex.lock();
		}

		~MutexLockGuard(){
			_mutex.unlock();
		}
	private:
		MutexLock& _mutex;
};

#endif
