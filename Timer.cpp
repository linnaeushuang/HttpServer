#include "Timer.h"
#include <sys/time.h>

TimerNode::TimerNode(std::shared_ptr<HttpData> requestData, const long &timeout): deleted_(false), SPHttpData(requestData) {
	gettimeofday(&now_tmp, NULL);
	// time unit is microseconds,
	// if use tv_sec, unit is secondes.
	expiredTime_ = now_tmp.tv_usec + timeout;
}

TimerNode::~TimerNode() {
	if (SPHttpData) SPHttpData->handleClose();
}

TimerNode::TimerNode(const TimerNode &tn)
	: SPHttpData(tn.SPHttpData), expiredTime_(0) {}


bool TimerNode::isValid() {
	gettimeofday(&now_tmp, NULL);
	if (now_tmp.tv_usec < expiredTime_) return true;
	else {
		this->setDeleted();
		return false;
	}
}

void TimerNode::clearReq() {
	SPHttpData.reset();
	this->setDeleted();
}

TimerManager::TimerManager() {}

TimerManager::~TimerManager() {}

void TimerManager::addTimer(std::shared_ptr<HttpData> SPHttpData, const long &timeout) {
	SPTimerNode new_node(new TimerNode(SPHttpData, timeout));
	timerNodeQueue.push(new_node);
	SPHttpData->linkTimer(new_node);
}



/* the time manager logic:
 *
 * because of:(1) priority queue do not support randomaccess
 * 			  (2) even if PQ do, randomly delete a node will destroy the structure of PQ.
 *
 * so:		      the timenode which is set delete_=true will be deleted
 * 			  (1) until it timeout 
 * 			  (2) or all node(befor it) was deleted
 *
 * advantage: (1) don't have to travese the PQ or rebuilt the PQ.
 * 			  (2) after timeout, the http-request occurs again,
 */
void TimerManager::handleExpiredEvent() {
	while (!timerNodeQueue.empty()) {
		SPTimerNode ptimer_now = timerNodeQueue.top();
		if (ptimer_now->isDeleted()) timerNodeQueue.pop();
		else if (ptimer_now->isValid() == false) timerNodeQueue.pop();
		else break;
	}
}
