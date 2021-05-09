
#include "EventLoop.h"
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include "utils/SocketUtils.h"
#include "utils/Logging.h"


__thread EventLoop *t_loopInThisThread=0;

int createEventFd(){
	int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if(evtfd < 0){
		LOG<<"Failed in eventfd create. Threads can connect!";
		abort();
	}
	return evtfd;
}


EventLoop::EventLoop(): looping_(false), epoller_(new Epoll()), wakeupFd_(createEventFd()), quit_(false), eventHandling_(false), callingPendingFunctors_(false), threadId_(CurrentThread::tid()), pwakeupEvent_(new Event(this, wakeupFd_)){
	if(!t_loopInThisThread){
		t_loopInThisThread=this;
	}
	// set wakeup eventfd
	// set event of interest on wakeup fd.
	pwakeupEvent_->setEvents(EPOLLIN | EPOLLET);
	pwakeupEvent_->setReadHandler(std::bind(&EventLoop::readWakeupFd,this));
	pwakeupEvent_->setConnHandler(std::bind(&EventLoop::handleConn,this));
	// register wakeupEvent to epoll Kernel.
	epoller_->epoll_add(pwakeupEvent_,0);
}

EventLoop::~EventLoop(){
	close(wakeupFd_);
	t_loopInThisThread = NULL;
}

void EventLoop::wakeup(){
	// write something to eventfd and wakeup other thread epoll
	uint64_t one = 1;
	ssize_t n = writen(wakeupFd_, (char*)(&one), sizeof(one));
	if(n != sizeof(one)){
		LOG<<"EventLoop::wakeup() writes "<<n<<" bytes instead of 8";
	}
}

void EventLoop::readWakeupFd(){
	uint64_t one = 1;
	ssize_t n = readn(wakeupFd_, &one, sizeof(one));
	if(n != sizeof(one)){
		LOG<<"EventLoop::readWakeupFd() writes "<<n<<" bytes instead of 8";
	}
	pwakeupEvent_->setEvents(EPOLLIN | EPOLLET);
}

void EventLoop::doPendingFunctors(){
	callingPendingFunctors_ = true;

	std::function<void()> nowFun;
	while(pendingFunctors_.size()>0){
		pendingFunctors_.pop(nowFun);
		nowFun();
	}

	callingPendingFunctors_ = false;
}

void EventLoop::handleConn(){
	epoller_->epoll_mod(pwakeupEvent_,0);
}

void EventLoop::loop(){
	assert(!looping_);
	assert(isInLoopThread());
	looping_ = true;
	quit_ = false;
	std::vector<SP_Event> ret;
	while(!quit_){
		ret.clear();
		// get event from epoll in this thread
		ret = epoller_->exec_epoll_wait();
		eventHandling_ = true;
		// do event in this thread
		for(auto& it:ret) it->handleEvents();
		eventHandling_ = false;
		// do event which registered from other thread
		doPendingFunctors();
		epoller_->handleExpired();
	}
	looping_=false;
}


void EventLoop::quit(){
	quit_=true;
	if(!isInLoopThread()){
		wakeup();
	}
}


// runInLoop and queueInLoop is copy from muduo
void EventLoop::runInLoop(Functor&& callBack){
	if(isInLoopThread()){
		callBack();
	}else{
		queueInLoop(std::move(callBack));
	}
}

void EventLoop::queueInLoop(Functor&& callBack){
	// this function will be call by main thread.
	// need to use mutex to ensure synchronization.
	
	pendingFunctors_.push(std::move(callBack));


	// if call by other(main) thread 
	// or 
	// doing pending function(1).
	//
	// (1) when calling pending function,
	// the function may be queueInLoop again.
	// wakeup in advance,
	// and after complete callingPendingFunctors,
	// this thread will continue to do.
	if(!isInLoopThread() || callingPendingFunctors_) wakeup();
}


void EventLoop::assertInLoopThread(){
	assert(isInLoopThread());
}

void EventLoop::shutdown(std::shared_ptr<Event> event){
	// half close for all thread which hold this socket
	shutDownWR(event->getFd());
}


void EventLoop::removeFromEpoller(std::shared_ptr<Event> event){
	epoller_->epoll_del(event);
}

void EventLoop::updateEpoller(std::shared_ptr<Event> event, int timeout){
	epoller_->epoll_mod(event,timeout);
}

void EventLoop::addToEpoller(std::shared_ptr<Event> event, int timeout){
	epoller_->epoll_add(event,timeout);
}

