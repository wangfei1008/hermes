#include "Condition.h"
#include "MutexLock.h"
#include <pthread.h>

//因为每个条件变量会对应一把锁
//所以需要在初始化一个条件变量时就将其对应的锁传进来
Condition::Condition(MutexLock& mutex):_mutex(mutex){
	pthread_cond_init(&_cond,nullptr);
}

Condition::~Condition(){
	pthread_cond_destroy(&_cond);
}

//阻塞
void Condition::wait(){
	//pthread_cond_wait的第二个参数是锁的地址
	//但我们的锁是一个对象形式的MutexLock，其内部的成员变量_mutex
	//才是这里应该传进去的，但其是private私有的
	//因此我们应该到MutexLock类内写一个get函数，
	//其返回私有成员变量_mutex的地址
	pthread_cond_wait(&_cond,_mutex.getMutexLockPtr());
}

//唤醒一个
void Condition::notify(){
	pthread_cond_signal(&_cond);
}

//广播唤醒
void Condition::notifyAll(){
	pthread_cond_broadcast(&_cond);
}
