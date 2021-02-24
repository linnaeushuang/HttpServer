
#include "Epoll.h"
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <deque>
#include <queue>
#include "utils/SocketUtils.h"
#include "utils/Logging.h"
#include <arpa/inet.h>
#include <iostream>


//typedef shared_ptr<Event> SP_Event; define in Event.h


// epoll_create1(flag)
// EPOLL_CLOECEC: close epollfd on exec
// this epollfd will close after fork a thread and exec this thread.
Epoll::Epoll() : epollFd_(epoll_create1(EPOLL_CLOEXEC)), events_(EVENTSNUM) {
	assert(epollFd_ > 0);
}

Epoll::~Epoll() {}

void Epoll::epoll_add(SP_Event request, int timeout) {
	int fd = request->getFd();
	if (timeout > 0) {
		add_timer(request, timeout);
		fd2http_[fd] = request->getHttpHolder();
	}
	struct epoll_event event;
	event.data.fd = fd;
	event.events = request->getEvents();
	
	request->EqualAndUpdateLastEvents();
	
	fd2event_[fd] = request;
	if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0) {
		perror("epoll_add error");
		fd2event_[fd].reset();
	}
}

void Epoll::epoll_mod(SP_Event request, int timeout) {
	if (timeout > 0) add_timer(request, timeout);
	int fd = request->getFd();
	if (!request->EqualAndUpdateLastEvents()) {
		struct epoll_event event;
		event.data.fd = fd;
		event.events = request->getEvents();
		if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0){
			perror("epoll_mod error");
			fd2event_[fd].reset();
		}
	}
}

void Epoll::epoll_del(SP_Event request) {
	int fd = request->getFd();
	struct epoll_event event;
	event.data.fd = fd;
	event.events = request->getLastEvents();
	if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event) < 0) {
		perror("epoll_del error");
	}
	fd2event_[fd].reset();
	fd2http_[fd].reset();
}

std::vector<SP_Event> Epoll::exec_epoll_wait() {
	while (true) {
		// std::vector's iterator is pointer
		int event_count = epoll_wait(epollFd_, &*events_.begin(), events_.size(), EPOLLWAIT_TIME);
		if (event_count < 0) perror("epoll wait error");
		std::vector<SP_Event> req_data = getEventsRequest(event_count);
		if (req_data.size() > 0) return req_data;
	}
}

void Epoll::handleExpired() { timerManager_.handleExpiredEvent(); }

std::vector<SP_Event> Epoll::getEventsRequest(int events_num) {
	std::vector<SP_Event> req_data;
	for (int i = 0; i < events_num; ++i) {
		// get fd which has a event create
		int fd = events_[i].data.fd;
		SP_Event cur_req = fd2event_[fd];
		if (cur_req) {
			cur_req->setRevents(events_[i].events);
			cur_req->setEvents(0);
			req_data.push_back(cur_req);
		} else {
			LOG << "SP cur_req is invalid";
		}
	}
	return req_data;
}

void Epoll::add_timer(SP_Event request_data, int timeout) {
	std::shared_ptr<HttpData> t = request_data->getHttpHolder();
	if (t)
		timerManager_.addTimer(t, timeout);
	else
		LOG << "timer add fail";
}
