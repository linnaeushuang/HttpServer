
#ifndef __EPOLL_HEAD
#define __EPOLL_HEAD

#include <sys/epoll.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include "Event.h"
#include "HttpData.h"
#include "Timer.h"


// muduo's epoll use default model (LT).
// this epoll use ET,
// so if we get a event,
// we must handle the event completely.

class Epoll {
public:
	Epoll();
	~Epoll();
	// add epoll fd
	void epoll_add(SP_Event request, int timeout);
	// fix epoll fd state
	void epoll_mod(SP_Event request, int timeout);
	// del epoll fd from epoll
	void epoll_del(SP_Event request);

	// loop to exec epoll_wait
	// if active event number == 0, countiue loop
	// return active event
	std::vector<std::shared_ptr<Event>> exec_epoll_wait();
	std::vector<std::shared_ptr<Event>> getEventsRequest(int events_num);
	void add_timer(std::shared_ptr<Event> request_data, int timeout);
	int getEpollFd() { return epollFd_; }
	void handleExpired();

private:
	static const int MAXFDS = 100000;
	int epollFd_;
	std::vector<epoll_event> events_;
	std::shared_ptr<Event> fd2event_[MAXFDS];
	std::shared_ptr<HttpData> fd2http_[MAXFDS];
	TimerManager timerManager_;
};

const int EVENTSNUM = 4096;
const int EPOLLWAIT_TIME = 10000000; // 10seceonds

#endif
