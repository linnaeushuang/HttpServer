#ifndef __ASYNC_LOG_HEAD
#define __ASYNC_LOG_HEAD
#include <functional>
#include <string>
#include <vector>
#include "CountDownLatch.h"
#include "LogStream.h"
#include "MutexLock.h"
#include "Thread.h"
#include "uncopyable.h"


class AsyncLogging : uncopyable {
// AsyncLogging is the back end of log system (see Logging.cpp pthread_once).
// AsyncLogging only has one instance.
// A log system only has a asyncLogging,
// but has multi front end(Logging).
//
// Logging generate log, and push this log to AsyncLogging buffer.
public:
	AsyncLogging(const std::string basename, int flushInterval = 2);
	~AsyncLogging() {
		if (running_) stop();
	}
	void append(const char* logline, int len);
	void start();
	void stop();
private:
	void threadFunc();

	typedef FixedBuffer<kLargeBuffer> Buffer; //in LogStream
	typedef std::vector<std::shared_ptr<Buffer>> BufferVector;
	typedef std::shared_ptr<Buffer> BufferPtr;

	const int flushInterval_;
	bool running_;
	std::string basename_;
	Thread thread_;
	MutexLock mutex_;
	Condition cond_;
	BufferPtr currentBuffer_;
	BufferPtr nextBuffer_;
	BufferVector buffers_;
	CountDownLatch latch_;
};

#endif
