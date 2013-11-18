#include "TTimer.h"
#include "TThreadI.h"
#include "Mutex.h"

// experimental - trying calling ensure* ...
#include "TStats.h"
#include "TUtils.h"
#include "TLogging.h"
#include "DeadlockMonitor.h"

#ifdef _WIN32
#	include <windows.h>
#	include <mmsystem.h>
#else
#	include <sys/time.h>
#	include <sys/times.h>
#	include <stdlib.h>

//#	include <mach/mach.h>
//#	include <mach/clock.h>
#endif

#pragma mark - TTimer

int TTimer::getGlobalTimeMs() {
#ifdef _WIN32
	return timeGetTime();
#else
	
#if 0 // doesn't work
	struct timeval tv;
	microuptime(tv);
	return tv.tv_sec*1000 + (tv.tv_usec/1000);
	
#elif 0 // way slower on iOS, both with SYSTEM_CLOCK and CALENDAR_CLOCK! (even if didn't fetch host clock every time ....)
	clock_serv_t host_clock;
	/*kern_return_t status =*/ host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &host_clock); // SYSTEM_CLOCK
	
	mach_timespec_t now;
	clock_get_time(host_clock, &now);

	return now.tv_sec*1000 + (now.tv_nsec/1000000);

#else 
	// doesn't seem that anything beats this, on iOS at least, however it is still not the fastest thing out there - can
	// quickly become a bottleneck if i.e. profiling stuff that happens thousands of times per second.
	struct timeval tv;
	if(gettimeofday(&tv,NULL) == -1) {
		//DebugAssert(false);
	}
	
	return tv.tv_sec*1000 + (tv.tv_usec/1000);
#endif
#endif
}

int TTimer::getTimePassedMs()const {
	return getGlobalTimeMs()-startTimeMs;
}
std::string TTimer::getTimePassedStr()const {
	return TUtils::formatWithCommas(getTimePassedMs());
}


// more efficient than the two separate calls, plus ensures no gap between getting time, and resetting ...
int TTimer::getTimePassedAndReset() {
	int t = getGlobalTimeMs();
	int result = t - startTimeMs;
	startTimeMs = t;
	return result;
}

void TTimer::setTimePassedMs(int n) {
	startTimeMs = getGlobalTimeMs() - n;
}
void TTimer::addTimePassedMs(int ms) {
	startTimeMs -= ms;
}

void TTimer::reset() { startTimeMs = getGlobalTimeMs(); }
TTimer::TTimer() {
	reset();
}


#pragma mark - SharedTimerThread

//
// this is designed to load my iphone 4s to ~90% CPU usage when running in debug mode ....
// providing lots of chance for mutex clashes to show themselves without completely killing
// everything
//
#define DO_PERF_LOAD_TEST 0

class SharedTimerThread {
	TThreadI thread { "SharedTimer" };
public:
	
	SharedTimerThread() { 
		
		SharedTimer::updateGlobalTime();
		
		//
		// this is a global static var, so should be inited very early, but first ensure some
		// other things are inited first + in a good order, so they don't get inited within
		// and cause wierd stuff ...!
		//
		TLogging::ensureStaticsLoaded();
		TStats::ensureStaticsLoaded();
		
		// avoid static-destruction order dependent crashes if this is destructed after ZLogging statics
		thread.disableLoggingOnDestruction();
		
		thread.go([] {
#			if DO_PERF_LOAD_TEST
				Thread::sleep(1);
			
				//1200 to be harsh, but then often has problems getting fully 2-way conv. going ...
				for(int i=0; i<1100; i++) { SharedTimer::updateGlobalTime(); }
#			else
				TThreadI::sleep(4);
#			endif

			SharedTimer::updateGlobalTime();
			
#if TRACK_DEADLOCKS
			DeadlockMonitor::getInstance()->tick();
#endif
			
		},
					 true,
					 TThreadI::kRealtimePriority, // could be just highest, tho being highest could hurt accuracy ...
					 false); // don't lock, as other statics/whatever may not be ready yet ....
	}
};

#pragma mark - SharedTimer

#if USING_SHARED_TIMER_THREAD

int SharedTimer::globalTimeMs = -1;

void SharedTimer::updateGlobalTime() {
	globalTimeMs = TTimer::getGlobalTimeMs();
}

/**
 much simpler just to have this global variable, which would be inited on start .... just can be dangerous when 
 have too many such and need to rely on init order, so doing the complicated checkSharedTimerThreadRUnning() 
 instead, tho not sure that actually needed at this point.
 */
SharedTimerThread stt;

#if DO_PERF_LOAD_TEST
SharedTimerThread stt2;
#endif

/*
void checkSharedTimerThreadRunning() {
	static bool sharedTimerThreadRunning = false;

	//escape infinite recursion (starts thread, which involves critical section, which calls profiling, which asks for time)
	if(!sharedTimerThreadRunning) {
		SharedTimer::updateGlobalTime();
		
		static SharedTimerThread* stt;
		static Mutex mutex;
		
		mutex.lock();
		if(!sharedTimerThreadRunning) {
			sharedTimerThreadRunning = true;
			stt = new SharedTimerThread();
		}
		mutex.unlock();
	}
}
*/

int SharedTimer::getGlobalTimeMs() {
	//checkSharedTimerThreadRunning();
	//static SharedTimerThread stt;
	
	return globalTimeMs;
}

int SharedTimer::getTimePassedMs()const {
	return getGlobalTimeMs()-startTimeMs;
}

std::string SharedTimer::getTimePassedStr()const {
	return TUtils::formatWithCommas(getTimePassedMs());
}

// more efficient than the two separate calls, plus ensures no gap between getting time, and resetting ...
int SharedTimer::getTimePassedAndReset() {
	int t = getGlobalTimeMs();
	int result = t - startTimeMs;
	startTimeMs = t;
	return result;
}

void SharedTimer::setTimePassedMs(int n) {
	startTimeMs = getGlobalTimeMs() - n;
}
void SharedTimer::addTimePassedMs(int ms) {
	startTimeMs -= ms;
}

void SharedTimer::reset() { startTimeMs = getGlobalTimeMs(); }
SharedTimer::SharedTimer() {
	reset();
}

#endif // #if USING_SHARED_TIMER_THREAD
