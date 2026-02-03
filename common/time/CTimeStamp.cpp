#include "CTimeStamp.h"
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <cstring>

#if defined(_WIN32) || defined(_MSC_VER)
#include <windows.h>
#include <winsock.h>
#else
#include <sys/time.h>
#endif

#if defined(_WIN32) || defined(_MSC_VER)
int gettimeofday(struct timeval* tp, void* tzp)
{
    if (tzp != NULL) return -1;
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;

    GetLocalTime(&wtm);
    tm.tm_year = wtm.wYear - 1900;
    tm.tm_mon = wtm.wMonth - 1;
    tm.tm_mday = wtm.wDay;
    tm.tm_hour = wtm.wHour;
    tm.tm_min = wtm.wMinute;
    tm.tm_sec = wtm.wSecond;
    tm.tm_isdst = -1;

    clock = mktime(&tm);
    tp->tv_sec = (long)clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;
    return (0);
}
#endif

double GetTimeStamp(int type)
{
    if (type < 0 || type > 3) {
        return 0.0; // 无效类型
    }
    
    static const unsigned long timesteps[] = {1, 1000, 1000000, 1000000000};
    unsigned long sec_timestep = timesteps[type];
    unsigned long usec_timestep = timesteps[3 - type];
    
    timeval tp;
    gettimeofday(&tp, NULL);
    
    return (double)tp.tv_sec * sec_timestep + (double)tp.tv_usec / usec_timestep;
}

int GetLocalTimeZoneByC()
{
    struct tm tm_local;
    struct tm tm_gmt;
    time_t local_as_utc;

    time_t time_utc = time(nullptr);
    if (time_utc == -1) {
        return 0;
    } 

#if defined(_WIN32)
    localtime_s(&tm_local, &time_utc);
    gmtime_s(&tm_gmt, &time_utc);
#else
    localtime_r(&time_utc, &tm_local);
    gmtime_r(&time_utc, &tm_gmt);
#endif
    
    local_as_utc = mktime(&tm_local);
    time_t gmt_as_utc = mktime(&tm_gmt);
    if (local_as_utc == -1 || gmt_as_utc == -1) {
        return 0;
    }
    
    int time_zone = (int)difftime(local_as_utc, gmt_as_utc) / 3600;
    
    if (time_zone < -12) {
        time_zone += 24;
    } else if (time_zone > 12) {
        time_zone -= 24;
    }
    
    return time_zone;
}

void TimeStampToStr(char* outbuf, double ts, int type)
{
    if (outbuf == nullptr || type < 0 || type > 3) {
        if (outbuf) outbuf[0] = '\0';
        return;
    }
    
    static const unsigned long timesteps[] = {1, 1000, 1000000, 1000000000};
    unsigned long sec_timestep = timesteps[type];
    unsigned long usec_timestep = timesteps[3 - type];
    
    timeval tv;
    tv.tv_sec = (long)(ts / sec_timestep);
    double fractional = ts - (double)tv.tv_sec * sec_timestep;
    tv.tv_usec = (long)(fractional * usec_timestep);
    
    if (tv.tv_usec < 0) {
        tv.tv_usec = 0;
    } else if (tv.tv_usec > 999999) {
        tv.tv_usec = 999999;
    }
    
    time_t clock = tv.tv_sec;
    struct tm tm_local;
    
#if defined(_WIN32)
    localtime_s(&tm_local, &clock);
#else defined(__linux__) || defined(__APPLE__)
    localtime_r(&tm_local, &clock);
#endif
    
    snprintf(outbuf, 64, "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
             tm_local.tm_year + 1900, 
             tm_local.tm_mon + 1, 
             tm_local.tm_mday,
             tm_local.tm_hour, 
             tm_local.tm_min, 
             tm_local.tm_sec, 
             tv.tv_usec / 1000);
}

double StrToTimeStamp(char* buf, int type)
{
    if (buf == nullptr || type < 0 || type > 3) {
        return 0.0;
    }
    
    static const unsigned long timesteps[] = {1, 1000, 1000000, 1000000000};
    unsigned long sec_timestep = timesteps[type];
    unsigned long usec_timestep = timesteps[3 - type];
    
    struct tm tm = {0};
    int msec = 0;
    
    int parsed = sscanf(buf, "%d-%d-%d %d:%d:%d.%d",
                       &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                       &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &msec);
    
    if (parsed < 6)  return 0.0;
    
    tm.tm_year -= 1900;
    tm.tm_mon -= 1;
    tm.tm_isdst = -1; 
    
    time_t clock = mktime(&tm);
    if (clock == -1) {
        return 0.0;
    }
    
    timeval tv;
    tv.tv_sec = clock;
    tv.tv_usec = (parsed >= 7) ? (msec * 1000) : 0;
    
    if (tv.tv_usec < 0) tv.tv_usec = 0;
    if (tv.tv_usec > 999999) tv.tv_usec = 999999;
    
    return (double)tv.tv_sec * sec_timestep + (double)tv.tv_usec / usec_timestep;
}

void GetCurrentTimeStampStr(char* outbuf, int type)
{
    if (outbuf == nullptr || type < 0 || type > 3) {
        if (outbuf) outbuf[0] = '\0';
        return;
    }
    
    double ts = GetTimeStamp(type);
    TimeStampToStr(outbuf, ts, type);
}


bool IsValidTimeStamp(double ts, int type)
{
    if (type < 0 || type > 3) return false;    

    if (ts < 0) return false;
    
    return true;
}