#ifndef __TCondition__
#define __TCondition__

#include "TCommon.h"
#include "Mutex.h"

#define USING_CPP11_CONDITIONAL 0

#if USING_CPP11_CONDITIONAL
#	include <condition_variable>
#else
#	include <pthread.h>
#endif

/**
 Example usage:
 
 bool needToDoThing = false;
 TCondition condition;
 MutexNR conditionMutex;
 
 ...
 
 void signal() {
    LockNR l(conditionMutex);
    needToDoThing = true;
    condition.notifyOne(l); // or notifyAll(l) if multiple threads waiting on the condition
 }
 
 ...
 
 void waitForSignal() {
    LockNR l(conditionMutex);
    condition.wait(l, [=]() { return needToDoThing; });
 }
 */

class TCondition {
#if USING_CPP11_CONDITIONAL
	std::condition_variable c;
#else
	pthread_cond_t c;
#endif
public:
	
#if !USING_CPP11_CONDITIONAL
	TCondition();
	~TCondition();
#endif
	
	void notifyOne();
	void notifyAll();
	
	/**
	 Waits until notified.
	 
	 Atomically releases lock, blocks the current executing thread, and adds it to the list 
	 of threads waiting on *this.
	 
	 Reacquires the lock when unblocked.
	 */
	void wait(LockNR& lock);
	
	/**
	 Like wait(), but with timeout.
	 
	 Returns true if was notified, false if timed-out
	 */
	bool waitWithTimeout(LockNR& lock, int timeoutMs);

	/**
	 Like wait(), but repeats until pred() returns true.
	 */
	template<class Predicate>
	void wait(LockNR& lock, Predicate pred) {
		while (!pred())
			wait(lock);
	}
	
	/**
	 Waits until either:
	  a) notified and pred returns true
	  b) timed-out.
	
	 Returns true if pred was true (i.e. will be false if timed-out)
	*/
	template<class Predicate>
	bool waitWithTimeout(LockNR& lock, int timeoutMs, Predicate pred) {
		while (!pred()) {
			if(!waitWithTimeout(lock, timeoutMs)) {
				return pred();
			}
		}
		return true;
	}
};

#endif /* defined(__TCondition__) */
