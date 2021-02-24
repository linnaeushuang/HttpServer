
#ifndef __UTILS_CONDITION_HEAD
#define __UTILS_CONDITION_HEAD

#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <cstdint>
#include "MutexLock.h"
#include "uncopyable.h"

class Condition: uncopyable{
private:
	MutexLock &mutex;
	pthread_cond_t condition;
	struct timespec abstime;
public:
	explicit Condition(MutexLock &_mutex): mutex(_mutex){
		pthread_cond_init(&condition,NULL);
	}
	~Condition() {pthread_cond_destroy(&condition); }

	// pthread_cond_wait:
	// 1. push this thread into wait queue
	// 2. unlock the secend arg (mutex)
	// 3. wait condition variable
	// 4. after wake up, lock the secend arg (mutex)
	// note : atomic
	void wait() { pthread_cond_wait(&condition,mutex.get_mutex_p()); }
	void notify() {pthread_cond_signal(&condition); }
	void notifyAll() {pthread_cond_broadcast(&condition); }

	// return true if time-out
	bool waitForSeconds(int seconds){
		clock_gettime(CLOCK_REALTIME, &abstime);
		abstime.tv_sec += static_cast<time_t>(seconds);
		return ETIMEDOUT == pthread_cond_timedwait(&condition,mutex.get_mutex_p(),&abstime);
	}
};
#endif
