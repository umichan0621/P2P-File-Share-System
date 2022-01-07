#pragma once
#include <stdio.h>
#include <string.h>
#include <string>

const int kSmallBuffer = 4096;
const int kLargeBuffer = 4096*1000;

template<int SIZE>
class LogBuffer
{
public:
	LogBuffer(): m_cur(m_data){
	}

	~LogBuffer(){
		//printf("%s", m_data);
	}

	void append(const char* /*restrict*/ buf, std::size_t len){
	// append partially
		if (/*implicit_cast<std::size_t>*/(avail()) > len)
		{
			memcpy(m_cur, buf, len);
			m_cur += len;
		}
	}

	// write in m_data directly
	char* current() {  return m_cur; };
	std::size_t avail() const { return static_cast<std::size_t> (end() - m_cur); }
	void add(std::size_t len) { m_cur += len; }
	int32_t length() const {return (int32_t)(m_cur - m_data);}
	void bzero() { ::bzero(m_data, sizeof(m_data)); }
	void reset() {m_cur = m_data;}

	const char* data() const { return m_data; }

private:
	const char* end() const { return m_data + sizeof(m_data); }

	char m_data[SIZE];
	char* m_cur;
};

class LogStream{
public:
	LogStream();
	~LogStream();

	typedef LogBuffer<kSmallBuffer> Buffer;
	typedef LogStream self;
	self& operator<<(bool v);

	self& operator<<(short);
	self& operator<<(unsigned short);
	self& operator<<(int);
	self& operator<<(unsigned int);
	self& operator<<(long);
	self& operator<<(unsigned long);
	self& operator<<(long long);
	self& operator<<(unsigned long long);

	self& operator<<(const void*);

	self& operator<<(float v);
	self& operator<<(double);

	self& operator<<(char v);
	self& operator<<(const char *);

	self& operator<<(const std::string& s);

	void append(const char* data, int len) { return m_buffer.append(data, len); }
	const Buffer& buffer() const { return m_buffer; }

private:
	LogStream(const LogStream& ls)=delete;			//no copyable
	LogStream& operator=(const LogStream& ls) = delete;

	template<typename _Type>
	void formatInteger(_Type v);

	Buffer m_buffer;
	static const std::size_t kMaxNumericSize = 32;

};

class Fmt{
public:
	template<typename _Type>
	Fmt(const char* fmt, _Type val);

	const char* data() const { return m_buf; }
	int length() const { return m_length; }

private:
	char m_buf[32];
	int m_length;
};

inline LogStream& operator<<(LogStream &s, const Fmt& fmt){
	s.append(fmt.data(), fmt.length());
	return s;
}

