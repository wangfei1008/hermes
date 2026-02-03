#include "CCPUTimer.h"
#include <chrono>
#include <thread>
#if defined(QT_VERSION) && defined(QT_CORE_LIB)

#if defined(_WIN32) || defined(WIN32)        //Windows

#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(BSD)    //Linux
#include <sys/time.h>
#endif


CCPUTimer::CCPUTimer(bool usecpu)
{
    m_usecpu = usecpu;
    m_isstop = false;
    m_precision = PrecisionType::microseconds;
#if defined(_WIN32) || defined(_MSC_VER)
    //获得CPU计时器的时钟频率
    QueryPerformanceFrequency(&m_litmp);//取得高精度运行计数器的频率f,单位是每秒多少次（n/s），
    m_msstepcounter = m_litmp.QuadPart/1000;
#elif defined(__linux__) || defined(__GNUC__)

#endif
}

void CCPUTimer::SetInterval(long microsecond)
{
#if defined(_WIN32) || defined(_MSC_VER)
    m_dfquadpart = (microsecond * m_litmp.QuadPart) /1000000;
#elif defined(__linux__) || defined(__GNUC__)
    m_interval_microsecond = microsecond;
#endif
}

void CCPUTimer::SetPrecision(PrecisionType precision)
{
    m_precision = precision;
}

void CCPUTimer::Start()
{
    m_isstop = false;
    QThread::start();
}

void CCPUTimer::Stop()
{
    m_isstop = true;
    this->quit();
    this->wait();
}


void CCPUTimer::run()
{
    while(!m_isstop)
    {
        timerbycpy();
        signal_timerout();
    }
}


void CCPUTimer::timerbycpy()
{
#if defined(_WIN32) || defined(_MSC_VER)
    LARGE_INTEGER privious, current;
    LONGLONG dfQuadPart = 0;

    QueryPerformanceCounter(&privious);//取得高精度运行计数器的数值
    current = privious;
    dfQuadPart = privious.QuadPart + m_dfquadpart;

    while(!m_isstop && (current.QuadPart < dfQuadPart))
    {
        if(!m_usecpu)
        {
            if((dfQuadPart - current.QuadPart) > m_msstepcounter)
            {
                switch (m_precision) {
                case PrecisionType::nanoseconds:
                    std::this_thread::sleep_for(std::chrono::nanoseconds(1));
                    break;
                case PrecisionType::milliseconds:
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    break;
                case PrecisionType::microseconds:
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                    break;
                case PrecisionType::seconds:
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    break;
                default:
                    break;
                }
            }

        }
        QueryPerformanceCounter(&current);//取得高精度运行计数器的数值
    }
#elif defined(__linux__) || defined(__GNUC__)
    struct timeval tpstart,tpcurrent;
    double timeuse;
    gettimeofday(&tpstart,NULL);
    tpcurrent = tpstart;
    while(!m_isstop && (timeuse=1000000*(tpcurrent.tv_sec-tpstart.tv_sec) + tpcurrent.tv_usec-tpstart.tv_usec) < m_interval_microsecond)
    {
        if(!m_usecpu)
        {
            switch (m_precision) {
            case PrecisionType::nanoseconds:
                std::this_thread::sleep_for(std::chrono::nanoseconds(1));
                break;
            case PrecisionType::milliseconds:
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                break;
            case PrecisionType::microseconds:
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                break;
            case PrecisionType::seconds:
                std::this_thread::sleep_for(std::chrono::seconds(1));
                break;
            default:
                break;
            }

        }
        gettimeofday(&tpcurrent,NULL);
    }
#endif
}


#endif
