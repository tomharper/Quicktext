#ifndef __DeadlockMonitor__
#define __DeadlockMonitor__

#include "TCommon.h"
#include "TAtomic.h"

#if TRACK_DEADLOCKS

class DeadlockMonitor {

	struct ThreadInf {
		Timer t;
		StackInf si { 3 }; // see if this isn't tooo expensive to capture with every lock!?
	};
	
	/**
	 The number here is a bit ridiculous .......
	 */
	TLockFreeSetPtrMR<ThreadInf, 256> threads;

	Timer timeSinceLastCheck;
	Timer timeSinceLastDump;
	
	AtomicInt nThreads;
	
	int foundDeadlockCount = 0;
	
public:
	
	static DeadlockMonitor* getInstance();
	
	int onEntry();
	void onExit(int index);
	
	// called from the main thread (i.e. SharedTimer?) ...
	void tick();
	
	/**
	 Can explicitly call this if notice a deadlock externally, but generally deadlocks will
	 be automatically detected internally and this will thus be called internally
	 */
	void haveDeadlock();
	
};

#endif

class ScopedDeadlockCheck {
#if TRACK_DEADLOCKS
	int dmIndex { -1 };
#endif
	
public:
	ScopedDeadlockCheck(bool initNow=true) {
		if(initNow) {
			init();
		}
	}
	void init() {
#if TRACK_DEADLOCKS
		dmIndex = DeadlockMonitor::getInstance()->onEntry();
#endif
	}
	void uninit() {
#if TRACK_DEADLOCKS
		if(dmIndex != -1) {
			DeadlockMonitor::getInstance()->onExit(dmIndex);
			dmIndex = -1;
		}
#endif
	}
	
	~ScopedDeadlockCheck() {
		uninit();
	}
};


#endif /* defined(__DeadlockMonitor__) */
