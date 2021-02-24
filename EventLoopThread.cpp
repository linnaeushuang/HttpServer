
#include "EventLoopThread.h"
#include <functional>
#include <assert.h>

// A LoopThread has a mutex
// bind to Thread::
EventLoopThread::EventLoopThread(): loop_(NULL), exiting_(false), thread_(std::bind(&EventLoopThread::threadFunc, this),"EventLoopThread"), mutex_(), cond_(mutex_){
	//nothing to do
}

EventLoopThread::~EventLoopThread(){
	exiting_ = true;
	if(loop_!=NULL){
		loop_->quit();
		thread_.join();
	}
}

EventLoop* EventLoopThread::startLoop(){
	assert(!thread_.started());
	thread_.start();
	{
		MutexLockGuard lock(mutex_);
		// wait until threadFunc is running in Thread.
		while(loop_==NULL) cond_.wait();
	}
	return loop_;
}

void EventLoopThread::threadFunc(){
	EventLoop loop;
	{
		MutexLockGuard lock(mutex_);
		loop_=&loop;
		cond_.notify();
	}
	loop.loop();
	loop_=NULL;
}
