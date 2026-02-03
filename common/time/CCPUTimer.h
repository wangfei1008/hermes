#ifndef CCPUTIMER_H_20231123
#define CCPUTIMER_H_20231123

// 只有使用 Qt 时才编译
#if defined(QT_VERSION) && defined(QT_CORE_LIB)

#if defined(_WIN32) || defined(_MSC_VER)
#include<windows.h>
#elif defined(__linux__) || defined(__GNUC__)

#endif

#include<QThread>

class CCPUTimer:public QThread
{
    Q_OBJECT
public:
    typedef enum PrecisionType{nanoseconds,microseconds,milliseconds,seconds}PrecisionType;
    CCPUTimer(bool usecpu = false);
    void SetInterval(long microsecond);
    void SetPrecision(PrecisionType precision);
    void Start();
    void Stop();
    void run();

private:
    void timerbycpy();                  //设置us延时

signals:
    void signal_timerout();            //定时器超时信号

private:
#if defined(_WIN32) || defined(_MSC_VER)
    LARGE_INTEGER m_litmp;
    LONGLONG m_msstepcounter;
    LONGLONG m_dfquadpart;
#elif defined(__linux__) || defined(__GNUC__)
    long m_interval_microsecond;         //超时的的微秒时间差
#endif
    bool m_usecpu;                      //使用高精度计时，导致CPU使用很大
    bool m_isstop;                      //是否停止计时器
    PrecisionType m_precision;          //cpu精度
};

#endif // QT_VERSION && QT_CORE_LIB
#endif // CCPUTIMER_H_20231123
