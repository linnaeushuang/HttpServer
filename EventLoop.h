
#ifndef __EVENT_LOOP_HEAD
#define __EVENT_LOOP_HEAD

#include <functional>
#include <vector>
#include <memory>
#include "Event.h"
#include "Epoll.h"
#include "utils/SocketUtils.h"
#include "utils/CurrentThread.h"
#include "utils/Thread.h"

// reference muduo

class EventLoop{

private:
	typedef std::function<void()> Functor;
	bool looping_;
	std::shared_ptr<Epoll> epoller_;
	// wakeupFd_ is a eventfd.
	// async connect between diff thread use eventfd
	int wakeupFd_;
	bool quit_;
	bool eventHandling_;
	mutable MutexLock mutex_;
	std::vector<Functor> pendingFunctors_;
	bool callingPendingFunctors_;
	const pid_t threadId_;
	// wrapper eventfd as a Event class
	std::shared_ptr<Event> pwakeupEvent_;

	void wakeup();
	void readWakeupFd();
	void doPendingFunctors();
	void handleConn();

public:
	EventLoop();
	~EventLoop();
	void loop();
	void quit();

	void runInLoop(Functor&& callBack);
	void queueInLoop(Functor&& callBack);

	bool isInLoopThread() const{ return threadId_ == CurrentThread::tid(); }
	void assertInLoopThread();
	void shutdown(std::shared_ptr<Event> event);
	void removeFromEpoller(std::shared_ptr<Event> event);
	void updateEpoller(std::shared_ptr<Event> event, int timeout=0);
	void addToEpoller(std::shared_ptr<Event> event, int timeout=0);

};

#endif
