#include "Logging.h"
#include "CurrentThread.h"
#include "Thread.h"
#include "AsyncLogging.h"
#include <assert.h>
#include <iostream>
#include <time.h>  
#include <sys/time.h> 

class AsyncLoggingSingletonWrapper: public AsyncLogging {
private:
	AsyncLoggingSingletonWrapper():AsyncLogging("./logDir/"){};
	AsyncLoggingSingletonWrapper(const AsyncLoggingSingletonWrapper& other) = delete;
	AsyncLoggingSingletonWrapper& operator=(const AsyncLoggingSingletonWrapper&)=delete;

public:
	static AsyncLoggingSingletonWrapper& getInstance(){
		static AsyncLoggingSingletonWrapper value;
		return value;
	}
	void append(const char* logline, int len){
		AsyncLogging::append(logline,len);
	}
};

Logger::Impl::Impl(const char *fileName, int line)
  : stream_(),
    line_(line),
    basename_(fileName)
{
    formatTime();
}

void Logger::Impl::formatTime()
{
    struct timeval tv;
    time_t time;
    char str_t[26] = {0};
    gettimeofday (&tv, NULL);
    time = tv.tv_sec;
    struct tm* p_time = localtime(&time);   
    strftime(str_t, 26, "%Y-%m-%d %H:%M:%S\n", p_time);
    stream_ << str_t;
}

Logger::Logger(const char *fileName, int line)
  : impl_(fileName, line)
{ }

Logger::~Logger()
{
    impl_.stream_ << " -- " << impl_.basename_ << ':' << impl_.line_ << '\n';
    const LogStream::Buffer& buf(stream().buffer());
	AsyncLoggingSingletonWrapper::getInstance().append(buf.data(),buf.length());
}
