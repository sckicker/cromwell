#pragma once

#include "xtp_base.h"

BEGIN_XTP_NAMESPACE(Szseo)

class XTP_DLL_EXPORT MutexType {
public:
	MutexType() {
		os_mutex_init(&mutex_);
	}

	~MutexType() {
		os_mutex_destroy(&mutex_);
	}

	inline void Acquire() {
		os_mutex_lock(&mutex_);
	}

	inline bool TryLock() {
		return os_mutex_trylock(&mutex_);
	}

	inline void Release() {
		os_mutex_unlock(&mutex_);
	}

private:
	MutexType(const MutexType & );
	const MutexType& operator = (const MutexType &);

	os_mutex_t mutex_;
};


template<typename Mutex>
class XTP_DLL_EXPORT ScopedMutex {
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

END_XTP_NAMESPACE(Szseo)