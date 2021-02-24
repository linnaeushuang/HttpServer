
#ifndef __SERVER_HEAR
#define __SERVER_HEAR

#include <memory>
#include "Event.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"

class Server{
private:
	int threadNum_;
	int port_;
	EventLoop *loop_;
	std::unique_ptr<EventLoopThreadPool> eventLoopThreadPool_;
	bool started_;
	std::shared_ptr<Event> acceptEvent_;
	int listenFd_;
	static const int MAXFDS = 100000;

public:
	Server(const int &threadNum, const int &port);
	~Server(){}
	EventLoop *getMainLoop() const { return loop_; }
	void start();
	void handNewConn();
	void handThisConn();
};

#endif
