#ifdef __linux__
#include <sys/time.h>
#else
#include <windows.h>
#include <time.h>
#endif
#include "time_stamp.h"

#ifndef __linux__
uint32_t gettimeofday(struct timeval* tp, void* tzp)
{
	struct tm stTime = { 0 };
	SYSTEMTIME CurTime;
	GetLocalTime(&CurTime);
	stTime.tm_year = CurTime.wYear - 1900;
	stTime.tm_mon = CurTime.wMonth - 1;
	stTime.tm_mday = CurTime.wDay;
	stTime.tm_hour = CurTime.wHour;
	stTime.tm_min = CurTime.wMinute;
	stTime.tm_sec = CurTime.wSecond;
	stTime.tm_isdst = -1;
	time_t Clock = mktime(&stTime);
	tp->tv_sec = (int32_t)Clock;
	tp->tv_usec = (int32_t)CurTime.wMilliseconds * 1000;
	return (0);
}
#endif

TimeStamp TimeStamp::now() {
	struct timeval tv;
	gettimeofday(&tv, 0);
	int64_t seconds = tv.tv_sec;
	return TimeStamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}


std::string TimeStamp::toString() const
{
	char buf[32] = { 0 };
	int64_t seconds = m_microSecondsSinceEpoch / kMicroSecondsPerSecond;
	int64_t microseconds = m_microSecondsSinceEpoch % kMicroSecondsPerSecond;
	snprintf(buf, sizeof(buf) - 1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
	return buf;
}
