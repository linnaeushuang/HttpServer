// @Autthor: Lin Huang
// use RAII


#ifndef __UTILS_MUTEXLOCK_HEAD
#define __UTILS_MUTEXLOCK_HEAD

#include <pthread.h>
#include <cstdio>
#include "uncopyable.h"

class MutexLock: uncopyable{
private:
	pthread_mutex_t mutex;
	friend class Condition;
public:
	MutexLock() { pthread_mutex_init(&mutex,NULL); }
	~MutexLock() {
		pthread_mutex_lock(&mutex);
		pthread_mutex_destroy(&mutex);
	}
	void lock() { pthread_mutex_lock(&mutex); }
	void unlock() { pthread_mutex_unlock(&mutex); }
	pthread_mutex_t* get_mutex_p() { return &mutex; };

};


// this class encapsulates the entry and exit of critical sections,
// i.e. locking and unlocking.
// Its scope is just equal to the critical region
class MutexLockGuard: public uncopyable{
private:
	//ref
	MutexLock &mutex;
public:
	explicit MutexLockGuard(MutexLock &_mutex): mutex(_mutex){
		mutex.lock();
	}
	~MutexLockGuard() { mutex.unlock(); };

};
#endif
