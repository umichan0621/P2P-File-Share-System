#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "time_stamp.h"
#include "logger.h"

char t_time[64];
time_t t_lastSecond;
char t_errnobuf[512];

static const char black[]  = {0x1b, '[', '1', ';', '3', '0', 'm', 0};
static const char red[]    = {0x1b, '[', '1', ';', '3', '1', 'm', 0};
static const char green[]  = {0x1b, '[', '1', ';', '3', '2', 'm', 0};
static const char yellow[] = {0x1b, '[', '1', ';', '3', '3', 'm', 0};
static const char blue[]   = {0x1b, '[', '1', ';', '3', '4', 'm', 0};
static const char purple[] = {0x1b, '[', '1', ';', '3', '5', 'm', 0};
static const char normal[] = {0x1b, '[', '0', ';', '3', '9', 'm', 0};

const char* strerror_tl(int savedErrno)
{
#ifdef __linux__
	return strerror_r(savedErrno, t_errnobuf, sizeof(t_errnobuf));
#else
	return (char *)strerror_s(t_errnobuf, savedErrno);
#endif
}

Logger::LogLevel g_logLevel = Logger::TRACE;

void Logger::setLogLevel(LogLevel level){
	g_logLevel = level;
}

Logger::LogLevel Logger::logLevel(){
	return g_logLevel;
}

const char* LogLevelName[Logger::NUM_LOG_LEVELS] =
{
	"[TRACE]",
	"[DEBUG]",
	"[INFO ]",
	"[WARN ]",
	"[ERROR]",
	"[FATAL]",
};

// helper class for known string length at compile time
class _Type
{
 public:
  _Type(const char* str, unsigned len)
    :m_str(str),
     m_len(len)
  {
    assert(strlen(str) == m_len);
  }

  const char* m_str;
  const unsigned m_len;
};

void defaultOutput(const char *msg, int32_t len){
	size_t n = fwrite(msg, 1, len, stdout);
	(void)n;
}

void defaultFlush(){
	fflush(stdout);
}

Logger::outputFunc g_output = defaultOutput;
Logger::flushFunc g_flush = defaultFlush;

void Logger::setOutput(outputFunc out){
	g_output = out;
}

void Logger::setFlush(flushFunc flush){
	g_flush = flush;
}

Logger::Logger(SourceFile file, int line)
	: m_impl(LogLevel::INFO, 0, file, line){
}

Logger::Logger(SourceFile file, int line, LogLevel level)
	: m_impl(level, 0, file, line){
}

Logger::Logger(SourceFile file, int line, bool toAbort)
	: m_impl(toAbort? LogLevel::FATAL: LogLevel::ERROR_, errno, file, line){
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
	: m_impl(level, 0, file, line){
	m_impl.m_stream << '[' << func << "] ";
}

Logger::~Logger(){
	m_impl.finish();
	const LogStream::Buffer& buf(stream().buffer());
	g_output(buf.data(), buf.length());
	if (m_impl.m_level == LogLevel::FATAL)
	{
		g_flush();
		abort();
	}
}

Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line)
	: m_time(TimeStamp::now()),
	  m_stream(),
	  m_level(level),
	  m_line(line),
	  m_fileBaseName(file)
{
	formatTime();

	switch(level)
	{
		case TRACE:
			m_stream << green << LogLevelName[level]  << ' ';
		break;
		case DEBUG:
			m_stream << blue << LogLevelName[level]  << ' ';
		break;
		case INFO:
			m_stream << black << LogLevelName[level]  << ' ';
		break;
		case WARN:
			m_stream << yellow << LogLevelName[level]  << ' ';
		break;
		case ERROR_:
			m_stream << purple << LogLevelName[level]  << ' ';
		break;
		case FATAL:
			m_stream << red << LogLevelName[level]  << ' ';
		break;
		default:
			m_stream << LogLevelName[level] << ' ';
		break;
	}

	//m_stream << LogLevelName[level] << ' ';
	m_stream << '[' << m_fileBaseName.m_data << ':' << m_line << "] ";
	if (savedErrno != 0)
	{
		m_stream << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
	}
}

void Logger::Impl::finish()
{
	m_stream << normal << '\n';
}

void Logger::Impl::formatTime()
{
	int64_t microSecondsSinceEpoch = m_time.microSecondsSinceEpoch();
	time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / TimeStamp::kMicroSecondsPerSecond);
	int microseconds = static_cast<int>(microSecondsSinceEpoch % TimeStamp::kMicroSecondsPerSecond);
	if (seconds != t_lastSecond){
		t_lastSecond = seconds;
		struct tm tm_time;
#ifdef __linux__
		::gmtime_r(&seconds, &tm_time); // FIXME TimeZone::fromUtcTime
#else
		gmtime_s(&tm_time, &seconds);
#endif
		int len = snprintf(t_time, sizeof(t_time), "%4d-%02d-%02d %02d:%02d:%02d",
		tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
		tm_time.tm_hour + 8, tm_time.tm_min, tm_time.tm_sec);
		assert(len == 19); (void)len;
	}

	Fmt us(".%06d ", microseconds);
	assert(us.length() == 8);
	m_stream << t_time << us.data();
}

