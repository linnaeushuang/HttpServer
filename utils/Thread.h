
#ifndef __THREAD_HEAD
#define __THREAD_HEAD

#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <functional>
#include <memory>
#include <string>
#include "CountDownLatch.h"
#include "uncopyable.h"

class Thread: uncopyable{
public:
	typedef std::function<void ()> ThreadFunc;
	explicit Thread(const ThreadFunc&,const std::string &name=std::string());
	~Thread();
	void start();
	int join();
	bool started() const {return started_; }
	const std::string& name() const{ return name_; }

private:
	void setDefaultName();
	bool started_;
	bool joined_;
	pthread_t pthreadId_;
	pid_t tid_;
	ThreadFunc func_;
	std::string name_;
	CountDownLatch latch_;
};
#endif
