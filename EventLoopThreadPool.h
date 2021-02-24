

#ifndef __EVENT_LOOP_THREAD_POOL_HEAD
#define __EVENT_LOOP_THREAD_POOL_HEAD
#include <memory>
#include <vector>
#include "EventLoopThread.h"
#include "utils/uncopyable.h"


// one Loop per thread
// there are not mutex lock
class EventLoopThreadPool: uncopyable{
private:
	// main loop, listen_fd
	EventLoop* mainLoop_;
	bool started_;
	const int numThreads_;
	int next_;
	// one loop per thread
	std::vector<std::shared_ptr<EventLoopThread>> workThreads_;
	// number of workLoops == number of workThreads
	std::vector<EventLoop*> workLoops_;
public:
	EventLoopThreadPool(EventLoop* mainLoop, const int &numThreads);
	~EventLoopThreadPool();
	void start();

	EventLoop* getNextWorkLoop();
};

#endif
