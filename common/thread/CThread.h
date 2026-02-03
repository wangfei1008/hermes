
#ifndef WANGFEI_BASE_THREAD_H_
#define WANGFEI_BASE_THREAD_H_

#include "pthread.h"
#include <functional>
#include <string>
using namespace std;

class CThread
{
public:
    typedef std::function<void ()> ThreadCallBack;

    CThread(const ThreadCallBack&);
    ~CThread();
    void start(const char* data);
    void stop();
    bool isstop();
private:
    void run();

public:
    static void* thread(void* parm);
private:
    bool m_status;                   //线程启动标志
    pthread_t m_id;                  //线程的id
    ThreadCallBack m_callback;       //回调函数
};


#endif // WANGFEI_BASE_THREAD_H_
