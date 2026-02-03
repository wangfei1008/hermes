#ifndef __CONDITION_H__
#define __CONDITION_H__
#include <pthread.h>
#include "NoCopyable.h"

class MutexLock;//前向声明

class Condition : public NoCopyable{
	public:
		//因为每个条件变量会对应一把锁
		//所以需要在初始化一个条件变量时就将其对应的锁传进来
		Condition(MutexLock& mutex);
		~Condition();
		//阻塞
		void wait();
		//唤醒一个
		void notify();
		//广播唤醒
		void notifyAll();
	private:
		pthread_cond_t _cond;
		MutexLock& _mutex;
};


#endif
