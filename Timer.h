
#ifndef __TIMER_HEAD
#define __TIMER_HEAD

#include <unistd.h>
#include <memory>
#include <queue>
#include <deque>
#include "HttpData.h"


class HttpData;

class TimerNode{
public:
	TimerNode(std::shared_ptr<HttpData> requestData, const long &timeout);
	~TimerNode();
	TimerNode(const TimerNode &tn);
	bool isValid();
	void clearReq();
	void setDeleted() { deleted_ = true; }
	bool isDeleted() const { return deleted_; }
	size_t getExpTime() const { return expiredTime_; }
private:
	bool deleted_;
	std::shared_ptr<HttpData> SPHttpData;
	size_t expiredTime_;
	struct timeval now_tmp;
};

struct TimerCmp {
	bool operator()(const std::shared_ptr<TimerNode> &a,
                  const std::shared_ptr<TimerNode> &b) const {
		return a->getExpTime() > b->getExpTime();
	}
};

class TimerManager {
public:
	TimerManager();
	~TimerManager();
	void addTimer(std::shared_ptr<HttpData> SPHttpData, const long &timeout);
	void handleExpiredEvent();

private:
	typedef std::shared_ptr<TimerNode> SPTimerNode;
	// use time heap;
	// STL use vector to implement PQ defaultly
	std::priority_queue<SPTimerNode, std::deque<SPTimerNode>, TimerCmp> timerNodeQueue;
};
#endif
