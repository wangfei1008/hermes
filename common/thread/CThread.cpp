
#include "CThread.h"
#include <stdio.h>
#include "../log/log.h"

CThread::CThread(const ThreadCallBack& func)
    :m_status(false)
    ,m_callback(func)
{
}

CThread::~CThread()
{
    stop();
}

void CThread::start(const char* data)
{    
    if(0 == pthread_create(&m_id, NULL, CThread::thread, this))
    {
        m_status = true;
        LOGDEBUG("create pthread id = %x, %s", &m_id, data);
    }
}

void CThread::run()
{
    m_callback();
}

void CThread::stop()
{
    if(!m_status) return;
    LOGDEBUG("stop pthread id = %x", &m_id);
    m_status = false;
    pthread_join(m_id, NULL);
    pthread_detach(m_id);
}


bool CThread::isstop()
{
    return !m_status;
}

void* CThread::thread(void* parm)
{
    CThread* pthread = (CThread*)parm;
    while(pthread->m_status)
        pthread->run();

    pthread_exit(nullptr);
    return NULL;
}
