
#include "EventLoopThreadPool.h"
#include "utils/Logging.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *mainLoop, const int &numThreads): mainLoop_(mainLoop), started_(false), numThreads_(numThreads), next_(0), workThreads_(numThreads_), workLoops_(numThreads_){
	if(numThreads_ <= 0){
		LOG<<"number of thread <= 0,error exit";
		abort();
	}
}

void EventLoopThreadPool::start(){
	mainLoop_->assertInLoopThread();
	started_ = true;
	for(int i = 0; i< numThreads_; ++i){
		std::shared_ptr<EventLoopThread> t(new EventLoopThread());
		workThreads_[i] = t;
		// start LoopThread( return current loop )
		workLoops_[i] = t->startLoop();
	}
}

EventLoop* EventLoopThreadPool::getNextWorkLoop(){
	mainLoop_->assertInLoopThread();
	assert(started_);
	EventLoop *loop = mainLoop_;
	if(!workLoops_.empty()){
		loop = workLoops_[next_];
		next_ = (next_ + 1) % numThreads_;
	}
	return loop;
}
EventLoopThreadPool::~EventLoopThreadPool(){
	LOG<<"EventLoopThreadPool exit!";
}
