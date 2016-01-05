#ifndef __MUTEX_H
#define __MUTEX_H

#include <pthread.h>

namespace cromwell {

class NullMutex {
public:
	inline bool Acquire() {
		return true;
	}

	inline bool TryLock() {
		return true;
	}

	inline bool Release() {
		return true;
	}
};

class MutexType {
public:
	MutexType() {
		pthread_mutex_init(&mutex_);
	}

	~MutexType() {
		pthread_mutex_destroy(&mutex_);
	}

	inline bool Acquire() {
		return 0 == pthread_mutex_lock(&mutex_);
	}

	inline bool TryLock() {
		return 0 == pthread_mutex_trylock(&mutex_);
	}

	inline bool Release() {
		return 0 == pthread_mutex_unlock(&mutex_);
	}

private:
	MutexType(const MutexType & );
	const MutexType& operator = (const MutexType &);

	pthread_mutex_t mutex_;
};


template<typename Mutex>
class ScopedMutex {
public:
	ScopedMutex(MutexType& lock) : lock_(lock) {
		lock_.Acquire();
	}

	~ScopedMutex() {
		lock_.Release();
	}

private:
	Mutex& lock_;
};

}//end-cromwell
