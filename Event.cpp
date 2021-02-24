
#include "Event.h"
#include <unistd.h>
#include <cstdlib>
#include "Epoll.h"
#include "EventLoop.h"

Event::Event(EventLoop *loop)
	: loop_(loop), events_(0), lastEvents_(0), fd_(0) {}

Event::Event(EventLoop *loop, int fd)
    : loop_(loop), events_(0), lastEvents_(0), fd_(fd) {}

Event::~Event(){}

int Event::getFd() { return fd_; }

void Event::setFd(int fd) { fd_ = fd; }

void Event::setHttpHolder(std::shared_ptr<HttpData> holder) {holder_ = holder; }

std::shared_ptr<HttpData> Event::getHttpHolder() { 
	std::shared_ptr<HttpData> ret(holder_.lock());
	return ret;
}


void Event::setReadHandler(CallBack &&readHandler) { readHandler_ = readHandler; }

void Event::setWriteHandler(CallBack &&writeHandler) { writeHandler_ = writeHandler; }

void Event::setErrorHandler(CallBack &&errorHandler) { errorHandler_ = errorHandler; }

void Event::setConnHandler(CallBack &&connHandler) { connHandler_ = connHandler; }

void Event::handleEvents() {
	events_ = 0;
	if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
		events_ = 0;
		return;
	}
	if (revents_ & EPOLLERR) {
		if (errorHandler_) errorHandler_();
		events_ = 0;
		return;
	}
	if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
		handleRead();
	}
	if (revents_ & EPOLLOUT) {
		handleWrite();
	}
	handleConn();
}



void Event::handleRead() {
	if (readHandler_) {
		readHandler_();
	}
}

void Event::handleWrite() {
	if (writeHandler_) {
		writeHandler_();
	}
}


void Event::handleConn() {
	if (connHandler_) {
		connHandler_();
	}
}


void Event::handleError(){
	//TODO: do something
};

void Event::setRevents(__uint32_t ev) { revents_ = ev; }
void Event::setEvents(__uint32_t ev) { events_ = ev; }
__uint32_t& Event::getEvents() { return events_; }

bool Event::EqualAndUpdateLastEvents() {
	bool ret = (lastEvents_ == events_);
	lastEvents_ = events_;
	return ret;
}
__uint32_t Event::getLastEvents() { return lastEvents_; }


