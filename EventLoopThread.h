

#ifndef __EVENT_LOOP_THREAD_HEAD
#define __EVENT_LOOP_THREAD_HEAD

#include "EventLoop.h"
#include "utils/Condition.h"
#include "utils/MutexLock.h"
#include "utils/Thread.h"
#include "utils/uncopyable.h"


class EventLoopThread: uncopyable{
private:
	void threadFunc();
	EventLoop* loop_;
	bool exiting_;
	Thread thread_;
	// each thread has a mutex lock
	// this lock is only contested by main thread and this thread
	// different thread in pool do not compete the mutex with each other
	MutexLock mutex_;
	Condition cond_;

public:
	EventLoopThread();
	~EventLoopThread();
	EventLoop* startLoop();
};

#endif

