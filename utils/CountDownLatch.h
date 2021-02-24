
#ifndef __UTILS_COUNDT_DOWN_LATCH_HEAD
#define __UTILS_COUNDT_DOWN_LATCH_HEAD


#include "Condition.h"
#include "MutexLock.h"
#include "uncopyable.h"

class CountDownLatch: uncopyable{
private:
	mutable MutexLock mutex;
	Condition condition;
	int count;
public:
	explicit CountDownLatch(int _count);

	// child threads wait for father thread to complete task
	void wait();


	// wait for all child thread to complete task
	// use broadcast
	void countDown();
};
#endif
