#include "TThreadI.h"
#include "TLogging.h"
#include "TRightThreadChecker.h"

#ifdef __APPLE__ // defined(_MAC) || defined(IOS)
#	include <mach/mach.h>
#	include "pthread.h"
#endif

using namespace std;

void TThreadI::go(std::function<void()> _fn, bool loopTillStopped, TThreadI::Priority prio, bool logThreadCreation) {
	if(loopTillStopped) {
		fn = [=]() {
			for(;;) {
				_fn();
				if(queryNeedToStop()) break;
			}
		};
	}
	else {
		fn = _fn;
	}
	TThreadI::start(prio, logThreadCreation);
}

void TThreadI::go(std::function<void(std::function<bool()> needToStop)> _fn, Priority prio, bool logThreadCreation) {
	fn = [=] {
		_fn( [this] {return queryNeedToStop();} );
	};
	TThreadI::start(prio, logThreadCreation);
}



void TThreadI::sleep(int ms) {

#ifdef _WIN32
	Sleep(msecs);
#else
	struct timespec short_wait;
	struct timespec remainder;
	short_wait.tv_sec = ms / 1000;
	short_wait.tv_nsec = (ms % 1000) * 1000 * 1000;
	nanosleep(&short_wait, &remainder);
#endif
}

int TThreadI::getCurrentThreadId() {
#ifdef _WIN32
	return GetCurrentThreadId();
#elif defined(__APPLE__)
	return static_cast<uint32_t>(mach_thread_self());
#elif !defined(WEBRTC_ANDROID) && defined(WEBRTC_LINUX)
	return static_cast<uint32_t>(syscall(__NR_gettid));
#else
	return static_cast<uint32_t>(pthread_self());
#endif
	
}

bool TThreadI::queryNeedToStop() {
	return stopPending;
}

bool TThreadI::isCurrentThread() {
	return threadId == getCurrentThreadId();
}


void TThreadI::signalAndWaitForStop(bool enableTimeout, bool enableLogging) {
	signalStop();
	waitForStop();
}

void TThreadI::signalStop() {
	LockNR l(conditionMutex);
	stopPending = true;
	condition.notifyOne();
}
void TThreadI::waitForStop(bool enableTimeout, bool enableLogging) {
	{
		LockNR l(stoppedConditionMutex);
		if(enableTimeout) {
			if(!stoppedCondition.waitWithTimeout(l, 10000, [&](){return !started; })) {
				TLogError("timed out!? %p", this);
			}
			else {
			}
		}
		else {
			stoppedCondition.wait(l, [&](){return !started; });
		}
	}
	if(!started) {
		stopPending = false;
	}
}

void* runInternal(void* to) {
	TThreadI* t = (TThreadI*) to;
	t->threadId = TThreadI::getCurrentThreadId();
	
	// hmmm - can't do this if this is the logging thread!
	if(t->logThreadCreation) {
		TLogError("Thread %s started with ID: %X", t->getName(), t->threadId); //getThreadId());
	}
	
	TRightThreadChecker::registerThread(t->name);
	
	t->run();
	
	{
		LockNR l(t->stoppedConditionMutex);
		t->started = false;
		t->stoppedCondition.notifyOne();
	}
	
	return 0;
}


void TThreadI::startThreadInternal(ThreadRunFunction func, ThreadObj obj, Priority prio, const char* threadName)
{
	pthread_attr_t  attr;
	pthread_t thread;

	// don't need thread cancellation in TTools' threads (differing here from system_wrapper!) ...
#ifndef ANDROID
	if(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL)) {
	}
#endif
	if(pthread_attr_init(&attr)) {
	}

	int result = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	result    |= pthread_attr_setstacksize(&attr, 1024*1024); // Set the stack stack size to 1M !
	
	result |= pthread_create(&thread, &attr, &runInternal, this);
	if (result != 0) {
		return;
	}
	if(setPrio(thread, prio)) {
	}
}

	
void TThreadI::start(Priority prio, bool _logThreadCreation) {
	
	logThreadCreation = _logThreadCreation;
	
	if(logThreadCreation) {
		TLogError("Starting thread %s", name);
	}
	
	started = true;

	startThreadInternal(&runInternal, this, prio,	name);
}

TThreadI::TThreadI(const char* nameForDebug):name(nameForDebug) {}

TThreadI::~TThreadI() {
	signalAndWaitForStop();
}

#include <errno.h>

void TThreadI::getSchedParam(Priority prio, sched_param& param, int& policy) {
	
//#ifdef WEBRTC_THREAD_RR
	policy = SCHED_RR;
//#else
//	policy = SCHED_FIFO;
//#endif
	
	const int minPrio = sched_get_priority_min(policy);
	const int maxPrio = sched_get_priority_max(policy);
	if ((minPrio == EINVAL) || (maxPrio == EINVAL)) {
		return;
	}
	
	switch (prio) {
		case kLowPriority:
			param.sched_priority = minPrio + 1;
			break;
		case kNormalPriority:
			param.sched_priority = (minPrio + maxPrio) / 2;
			break;
		case kHighPriority:
			param.sched_priority = maxPrio - 3;
			break;
		case kHighestPriority:
			param.sched_priority = maxPrio - 2;
			break;
		case kRealtimePriority:
			param.sched_priority = maxPrio - 1;
			break;
	}	
}


int TThreadI::setPrio(pthread_t thread, Priority prio) {
#ifdef ANDROID
	/* Not allowed in Android */
	return 0;
#else
	sched_param param;
	int policy;
	getSchedParam(prio, param, policy);
	return pthread_setschedparam(thread, policy, &param);
#endif
}
