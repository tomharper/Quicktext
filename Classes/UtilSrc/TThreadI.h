#ifndef _TThread_h
#define _TThread_h

#include "TCommon.h"
#include "TCondition.h"

#include "pthread.h"

#define ThreadObj void*
typedef void*(*ThreadRunFunction)(ThreadObj);
/**
 Provides std::thread-like functionality such that just construct an object, passing it i.e. a lambda,
 and it will automatically run .... and then can call join() when want to wait for it to complete.
 
 Uses TThread internally, set threadName and lets you set the thread-prio
 (unlike std::thread).
 
 Doesn't have detach() like TThread - not sure if/how that would be possible with this .... guess it
 should be but would have to check TThread doesn't use any members after run() completion.
 */


class TThreadI {
private:
	bool enableLoggingOnDestruction=true;
	std::function<void()> fn;

	
public:
    virtual void run() {
		fn();
	}
	enum Priority {
		kLowPriority = 1,
		kNormalPriority = 2,
		kHighPriority = 3,
		kHighestPriority = 4,
		kRealtimePriority = 5
	};
    
	void go(std::function<void()> _fn, bool loopTillStopped=false, Priority prio=kNormalPriority, bool logThreadCreation=true);
	
	void go(std::function<void(std::function<bool()> needToStop)> _fn, Priority prio=kNormalPriority, bool logThreadCreation=true);
	/*
     TThread(const char* nameForDebug):TThreadI(nameForDebug) {}
     TThread(const char* nameForDebug, std::function<void()> _fn, bool loopTillStopped=false, Priority prio=kNormalPriority, bool logThreadCreation=true):
     TThreadI(nameForDebug)
     {
     go(_fn, loopTillStopped, prio, logThreadCreation);
     }
     TThread(const char* nameForDebug, std::function<void(std::function<bool()> needToStop)> _fn, Priority prio=kNormalPriority, bool logThreadCreation=true):
     TThreadI(nameForDebug)
     {
     go(_fn, prio, logThreadCreation);
     }
     */
	
	void disableLoggingOnDestruction() {
		enableLoggingOnDestruction = false;
	}
	
	void join() { signalAndWaitForStop(); }
	~TThreadI();
	TThreadI(const char* nameForDebug);
    
public:
    
	int threadId = -1;
	const char* name = nullptr;
	volatile bool started = false;
	bool logThreadCreation = false;
	
public:
	
	/**
	 Used to signal from the signalAndWaitForStop() function to the thread that it needs to stop.
	 Inheriting classes should also make use of it for other signaling to their thread.
	 (public as finding useful for it to be so ...)
	 */
	TCondition condition;
	MutexNR conditionMutex;
	
	/**
	 Used to signal back from the thread when actually stopped to the signalAndWaitForStop() function.
	 (public as used from the runInternal non-member function)
	 */
	TCondition stoppedCondition;
	MutexNR stoppedConditionMutex;
	
protected:
	
	volatile bool stopPending = false;
	
	// The main loop in run() should regularly call this, and if it returns true then return
	bool queryNeedToStop();
		
public:
	
	bool isCurrentThread();
	

	bool waitOnStopSignalOrTimeout(int timeoutMs) {
		LockNR l(conditionMutex);
		return condition.waitWithTimeout(l, timeoutMs, [=](){return queryNeedToStop();});
	}
	bool waitOnSignalOrTimeout(int timeoutMs) {
		LockNR l(conditionMutex);
		condition.waitWithTimeout(l, timeoutMs);
		return queryNeedToStop();
	}
	bool waitOnSignal() {
		LockNR l(conditionMutex);
		condition.wait(l);
		return queryNeedToStop();
	}


	
	static void sleep(int ms);
	static int getCurrentThreadId();
	//int getThreadId();
	const char* getName() const { return name; }
	
	/**
	 Calls signalStop() and then waitForStop, waiting for the thread to notice+stop.
	 
	 All inheriting classes should define a destructor that calls this!
	 */
	void signalAndWaitForStop(bool enableTimeout=true, bool enableLogging=true);
	
	// can call these functions separately if required ...
	void signalStop();
	void waitForStop(bool enableTimeout=true, bool enableLogging=true);
	
	void start(Priority prio = kNormalPriority, bool logThreadCreation=true);
	

	// helper functions!
	static void getSchedParam(Priority prio, sched_param& param, int& policy);
	static int setPrio(pthread_t thread, Priority prio);
	
private:
	void startThreadInternal(ThreadRunFunction func, ThreadObj obj, Priority prio, const char* threadName);

   // disable copying
	//TThreadI(const TThreadI&);
	//TThreadI& operator=(const TThreadI&);
	
};

#endif
