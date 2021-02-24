
#include "CountDownLatch.h"

CountDownLatch::CountDownLatch(int _count)
	:mutex(),condition(mutex),count(_count) {}

void CountDownLatch::wait(){
	MutexLockGuard lock_mutex(mutex);
	while(count>0){
		// for spuriour wake up( see: http://en.wikipedia.org/wiki/Spurious_wakeup )
		// core reason: pthread_cond_signal(broadcast) may wake up multi thread, and you want to use a certain thread
		condition.wait();
	}
}

void CountDownLatch::countDown(){
	MutexLockGuard lock_mutex(mutex);
	--count;
	if(count==0) condition.notifyAll();
}
