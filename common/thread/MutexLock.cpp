#include "MutexLock.h"
#include <pthread.h>
#include <stdio.h>

//在构造函数中进行锁资源的初始化
MutexLock::MutexLock(){
	//锁初始化，第二个参数是锁的属性，默认为null
	int ret = pthread_mutex_init(&_mutex,nullptr);
	if(ret){
		perror("pthread_mutex_init");
	}
}

//在析构函数中销毁锁资源
MutexLock::~MutexLock(){
	int ret = pthread_mutex_destroy(&_mutex);
	if(ret){
		perror("pthread_mutex_destroy");
	}
}

//上锁
void MutexLock::lock(){
	int ret = pthread_mutex_lock(&_mutex);
	if(ret){
		perror("pthread_mutex_lock");
	}
}

//释放锁
void MutexLock::unlock(){
	int ret = pthread_mutex_unlock(&_mutex);
	if(ret){
		perror("pthread_mutex_unlock");
	}
}
