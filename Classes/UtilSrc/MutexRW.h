#ifndef __TMutexRW__
#define __TMutexRW__

#include "TCommon.h"
#include <pthread.h>

#if TRACK_LONG_LOCK_HOLDS
#	include "TTimer.h"
#endif


class MutexRW {
	pthread_rwlock_t lock;
	
public:
	MutexRW();
	~MutexRW();
	void getReadLock();
	void freeReadLock();
	
	void getWriteLock();
	void freeWriteLock();
};


class ReadLock {
	MutexRW& m;
	bool locked = false;
	
#if TRACK_DEADLOCKS
	int dmIndex { -1 };
#endif
	
#if TRACK_LONG_LOCK_HOLDS
	Timer t;
#endif
	
public:
	ReadLock(MutexRW& _m, bool lockNow=true);
	void lock();
	void unlock();
	bool isLocked();
	
	~ReadLock();
	
	// for compatibility
	bool owns_lock() { return isLocked(); }
	MutexRW* mutex() { return &m; }
};


class WriteLock {
	MutexRW& m;
	bool locked = false;
	
#if TRACK_DEADLOCKS
	int dmIndex { -1 };
#endif
	
#if TRACK_LONG_LOCK_HOLDS
	Timer t;
#endif
	
public:
	WriteLock(MutexRW& _m, bool lockNow=true);
	void lock();
	void unlock();
	bool isLocked();
	
	~WriteLock();
	
	// for compatibility
	bool owns_lock() { return isLocked(); }
	MutexRW* mutex() { return &m; }
};

#endif /* defined(__MutexRW__) */
