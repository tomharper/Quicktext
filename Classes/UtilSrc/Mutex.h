/**
 Wraps C++11 mutex+lock functionality ...
 */
#ifndef _Mutex_h
#define _Mutex_h

#include "TCommon.h"

#ifdef __cplusplus

#if TRACK_LONG_LOCK_HOLDS
#	include "TTimer.h"
#endif

//
// using c++11 implementation seems to cause issues somehow ... tho its using pthreads internally
// in any case ...
//
#define MUTEX_USE_CPP11 0

#if MUTEX_USE_CPP11
#	define MUTEX_USE_POSIX_0
#elif defined(_WIN32)
#	define MUTEX_USE_POSIX 0
#else
#	define MUTEX_USE_POSIX 1
#endif

#include <mutex>
#if MUTEX_USE_POSIX
#	include <pthread.h>
#endif


class Mutex {	
#if MUTEX_USE_CPP11
	std::recursive_mutex _mutex;
#elif MUTEX_USE_POSIX
	pthread_mutex_t _mutex;
#else
	static_assert(false, "Not yet implemented");
#endif
	
	void init();
	
public:
	Mutex();
	~Mutex();
	void lock();
	void unlock();
	bool tryLock();

	// for compatibility
#if MUTEX_USE_CPP11
	pthread_mutex_t* native_handle() { return _mutex.native_handle(); }
#else
	pthread_mutex_t* native_handle() { return &_mutex; }
#endif
	bool try_lock() { return tryLock(); }
	
	/** 
	 Make copying simply do nothing!? (as makes no sense to copy, only causes trouble ... and so the target is
	 just initialized like it normally would be with a separate mutex ...)
	 - either that, or absolutely have to disable copying of Mutex objects ...
	 */
	Mutex(const Mutex& m);
	Mutex& operator=(const Mutex& m) { return *this; }
	ENABLE_DEFAULT_MOVE_SEMANTICS(Mutex);
};

/**
 Lock 'no-tracking' (does no internal profiling/deadlock-check etc 
 i.e. within profiling/logging/etc, in order to avoid recursion) ...
 */
class LockNT {
	Mutex* m = nullptr;
	bool locked = false;
	
public:
	LockNT(Mutex& _m, bool lockNow=true);
	LockNT(Mutex* _m, bool lockNow=true); //< _m can be nullptr, in which case lock will simply do nothing when asked to lock+unlock
	
	// returns true if successfully locks
	bool tryLock();
	
	void lock();
	void unlock();
	bool isLocked();
	
	~LockNT();
};

class Lock {
	Mutex* m = nullptr;
	bool locked = false;
	
#if TRACK_DEADLOCKS
	int dmIndex { -1 };
#endif
	
#if TRACK_LONG_LOCK_HOLDS
	Timer t;
	bool needToTrack = true;
#endif
	
public:
	Lock(Mutex& _m, bool lockNow=true);
	Lock(Mutex* _m, bool lockNow=true); //< _m can be nullptr, in which case lock will simply do nothing when asked to lock+unlock
	
	bool tryLock();
	void lock();
	void unlock();
	bool isLocked();
	
	~Lock();
	
	// for compatibility
	bool owns_lock() { return isLocked(); }
	Mutex* mutex();
};


/**
 Non-recursive mutex - have to use C++11 directly in this case so can pass to Condition,
 as being able to pass to Condition is the main reason for providing this non-recursive
 variant (since that's what Condition requires - the C++11 unique_lock<std::mutex>).
 */
#if MUTEX_USE_CPP11
typedef std::mutex MutexNR;
typedef std::unique_lock<std::mutex> LockNR;
#else
typedef Mutex MutexNR;
typedef Lock LockNR;
#endif


extern "C" {
#endif //#ifdef __cplusplus

/**
 Functions for use from C files - yuck
 */
void* TMutex_Create();
void  TMutex_Delete(void* mutex);
void  TMutex_Lock(void* mutex);
void  TMutex_Unlock(void* mutex);

#ifdef __cplusplus
}
#endif


#endif // #ifndef _Mutex_h
