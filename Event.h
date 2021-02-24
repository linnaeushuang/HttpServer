
#ifndef __EVENT_HEAD
#define __EVENT_HEAD

#include <functional>
#include <memory>
#include "Timer.h"

/* A class Event is an event in the Reactor structure.
 * It is always in an EventLoop and is responsible for
 * the IO event of a FD. The type of the IO envet and
 * the corresponding callback function are stored in
 * the Event class. Therefore, all objects in the
 * program with read/write time are associated with 
 * a Event instance, including eventfd, listenfd, httpdata,
 * and so on.
 *
 */

class EventLoop;
class HttpData;

class Event{
private:
	typedef std::function<void()> CallBack;

	__uint32_t events_;
	__uint32_t revents_;
	__uint32_t lastEvents_;
	int fd_;

	// for easy to find the httpdata that holds the Event.
	std::weak_ptr<HttpData> holder_;
	EventLoop *loop_;

	CallBack readHandler_;
	CallBack writeHandler_;
	CallBack errorHandler_;
	CallBack connHandler_; //for http

	int parse_URI();
	int parse_Headers();
	int analysisRequest();

public:
	Event(EventLoop *loop);
	Event(EventLoop *loop, int fd);
	~Event();
	int getFd();
	void setFd(int fd);

	void setHttpHolder(std::shared_ptr<HttpData> holder);
	std::shared_ptr<HttpData> getHttpHolder(); //weak_ptr to shared_ptr

	void setReadHandler(CallBack &&readHandler);
	void setWriteHandler(CallBack &&writeHandler);
	void setErrorHandler(CallBack &&errorHandler);
	void setConnHandler(CallBack &&connHandler);

	void handleEvents();

	void handleRead();
	void handleWrite();
	void handleError();
	void handleConn();

	void setRevents(__uint32_t ev);

	void setEvents(__uint32_t ev);

	__uint32_t &getEvents();

	bool EqualAndUpdateLastEvents();

	__uint32_t getLastEvents();
};

typedef std::shared_ptr<Event> SP_Event;
#endif
