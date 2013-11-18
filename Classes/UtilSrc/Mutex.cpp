#include "Mutex.h"
#include "TStats.h"
#include "DeadlockMonitor.h"

void* TMutex_Create() { return new Mutex(); }
void  TMutex_Delete(void* mutex) { if(mutex) { delete (Mutex*)mutex; } }
void  TMutex_Lock(void* mutex) { Mutex* m = (Mutex*) mutex; m->lock(); }
void  TMutex_Unlock(void* mutex) { Mutex* m = (Mutex*) mutex; m->unlock(); }

#if MUTEX_USE_CPP11
Mutex::Mutex() {}
Mutex::~Mutex() {}
void Mutex::lock() { _mutex.lock(); }
void Mutex::unlock() { _mutex.unlock(); }
bool Mutex::tryLock() { return _mutex.try_lock(); }

#else // MUTEX_USE_POSIX

void Mutex::init() {
	pthread_mutexattr_t attr;
	(void) pthread_mutexattr_init(&attr);
	(void) pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	(void) pthread_mutex_init(&_mutex, &attr);
}

Mutex::Mutex() {
	init();
}

Mutex::Mutex(const Mutex& m) {
	init();
}

Mutex::~Mutex() {
	(void) pthread_mutex_destroy(&_mutex);
}
void Mutex::lock() {
	(void) pthread_mutex_lock(&_mutex);
}
void Mutex::unlock() {
	(void) pthread_mutex_unlock(&_mutex);
}
bool Mutex::tryLock() {
	return (pthread_mutex_trylock(&_mutex) == 0);
	/*{
      //TRACK_OWNER(thread_ = pthread_self());
      return true;
	}
	return false;*/
}
#endif


#pragma mark - Lock

Lock::Lock(Mutex& _m, bool lockNow):m(&_m) {
	if(lockNow) {
		lock();
	}
}
Lock::Lock(Mutex* _m, bool lockNow):m(_m) {
	if(lockNow) {
		lock();
	}
}

bool Lock::tryLock() {
	if(!m || m->tryLock()) {
		locked = true;
		
#if TRACK_LONG_LOCK_HOLDS
		t.reset();
#endif
		
		return true;
	}
	return false;
}
void Lock::lock() {
	if(m) {
		
#if TRACK_DEADLOCKS
		dmIndex = DeadlockMonitor::getInstance()->onEntry();
#endif
		
		m->lock();
	}
	locked = true;
	
#if TRACK_LONG_LOCK_HOLDS
	t.reset();
#endif
}

bool Lock::isLocked() {
	return locked;
}

void Lock::unlock() {
	if(m) {
		m->unlock();
#if TRACK_DEADLOCKS
		// needToTrack will be set to false when mutex() called ...
		if(dmIndex != -1 && needToTrack) {
			DeadlockMonitor::getInstance()->onExit(dmIndex);
			dmIndex = -1;
		}
#endif
	}
	locked = false;
	
	
#if TRACK_LONG_LOCK_HOLDS
	if(needToTrack) {
		auto tp = t.getTimePassedMs();
		if(tp > LONG_LOCK_HOLDS_THRESHOLD_MS) {
			TSTATS_COUNT("Lock hold above threshold", tp);
		}
		TSTATS_VAL("Lock held", tp);
		t.reset();
	}
#endif
}

Mutex* Lock::mutex() {
	
	// if passing mutex off to third-party (i.e. conditional wait), then they'll be unlocking, so give-up
	// trying to track the time-held ourselves (in this case its anyway presumed to be minimal
	needToTrack = false;
	
#if TRACK_DEADLOCKS
	if(dmIndex != -1) {
		DeadlockMonitor::getInstance()->onExit(dmIndex);
		dmIndex = -1;
	}
#endif
	
	return m;
}

Lock::~Lock() {
	if(locked) {
		unlock();
	}
}



#pragma mark - LockNT

LockNT::LockNT(Mutex& _m, bool lockNow):m(&_m) {
	if(lockNow) {
		lock();
	}
}
LockNT::LockNT(Mutex* _m, bool lockNow):m(_m) {
	if(lockNow) {
		lock();
	}
}

bool LockNT::tryLock() {
	if(!m || m->tryLock()) {
		locked = true;
		return true;
	}
	return false;
}
void LockNT::lock() {
	if(m) {
		m->lock();
	}
	locked = true;
}

bool LockNT::isLocked() {
	return locked;
}

void LockNT::unlock() {
	if(m) {
		m->unlock();
	}
	locked = false;
}

LockNT::~LockNT() {
	if(locked) {
		unlock();
	}
}
