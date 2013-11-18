#include "MutexRW.h"
#include "TLogging.h"
#include "TStats.h"
#include "DeadlockMonitor.h"

#pragma mark - MutexRW

MutexRW::MutexRW():lock(PTHREAD_RWLOCK_INITIALIZER) {
}

MutexRW::~MutexRW() {
	pthread_rwlock_destroy(&lock);	
}

void MutexRW::getReadLock() {
	pthread_rwlock_rdlock(&lock);	
}
void MutexRW::freeReadLock() {
	pthread_rwlock_unlock(&lock);
}

void MutexRW::getWriteLock() {
	pthread_rwlock_wrlock(&lock);
}
void MutexRW::freeWriteLock() {
	pthread_rwlock_unlock(&lock);
}

#pragma mark - ReadLock

ReadLock::ReadLock(MutexRW& _m, bool lockNow):m(_m) {
	if(lockNow) {
		lock();
	}
}

void ReadLock::lock() {
	
#if TRACK_DEADLOCKS
	dmIndex = DeadlockMonitor::getInstance()->onEntry();
#endif
	
	m.getReadLock();
	locked = true;
	
#if TRACK_LONG_LOCK_HOLDS
	t.reset();
#endif
}

bool ReadLock::isLocked() {
	return locked;
}

void ReadLock::unlock() {
	m.freeReadLock();
	locked = false;
	
#if TRACK_DEADLOCKS
	if(dmIndex != -1) {
		DeadlockMonitor::getInstance()->onExit(dmIndex);
		dmIndex = -1;
	}
#endif
	
#if TRACK_LONG_LOCK_HOLDS
	auto tp = t.getTimePassedMs();
	if(tp > LONG_LOCK_HOLDS_THRESHOLD_MS) {
		TSTATS_COUNT("ReadLock hold above threshold", tp);
	}
	TSTATS_VAL("ReadLock held", tp);
	t.reset();
#endif
}

ReadLock::~ReadLock() {
	if(locked) {
		unlock();
	}
}


#pragma mark - WriteLock

WriteLock::WriteLock(MutexRW& _m, bool lockNow):m(_m) {
	if(lockNow) {
		lock();
	}
}

void WriteLock::lock() {
	
#if TRACK_DEADLOCKS
	dmIndex = DeadlockMonitor::getInstance()->onEntry();
#endif
	
	m.getWriteLock();
	locked = true;
	
#if TRACK_LONG_LOCK_HOLDS
	t.reset();
#endif
}

bool WriteLock::isLocked() {
	return locked;
}

void WriteLock::unlock() {
	
	m.freeWriteLock();
	locked = false;
	
#if TRACK_DEADLOCKS
	if(dmIndex != -1) {
		DeadlockMonitor::getInstance()->onExit(dmIndex);
	}
#endif
	
#if TRACK_LONG_LOCK_HOLDS
	auto tp = t.getTimePassedMs();
	if(tp > LONG_LOCK_HOLDS_THRESHOLD_MS) {
		TSTATS_COUNT("WritedLock hold above threshold", tp);
	}
	TSTATS_VAL("WriteLock held", tp);
	t.reset();
#endif
}

WriteLock::~WriteLock() {
	if(locked) {
		unlock();
	}
}
