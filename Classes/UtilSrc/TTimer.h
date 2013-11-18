#ifndef _TTimer_h
#define _TTimer_h

#include "TCommon.h"
#include <iosfwd>

/**
 Use the typedef Timer, which will be set to TTimer / SharedTimer according to the define 
 USING_SHARED_TIMER_THREAD.
 
 If this is defined, then a realtime thread is created that updates a global timer variable every 
 3ms, and then all Timer objects simply reference this value rather than querying from the system.
 
 When using Timer a lot (i.e. profiling stuff 1000's of times a second), the system calls to get 
 the time can become a bottleneck and this can be a good solution, at the cost of the extra thread 
 (which should cost a lot less than 1000's of calls) and some accuracy (which in theory should 
 average-out and in any case still catch the big bottlenecks etc).
 */
#define USING_SHARED_TIMER_THREAD 1

class TTimer {
public:
	int startTimeMs;
	
	static int getGlobalTimeMs();

	int getTimePassedMs()const;
	std::string getTimePassedStr()const;
	
	// more efficient than the two separate calls, plus ensures no gap between getting time, and resetting ...
	int getTimePassedAndReset();
	
	void setTimePassedMs(int n);
	void addTimePassedMs(int ms);
	
	void reset();
	TTimer();
	
	// clone another timer - can use to save fetching the time twice ...
	TTimer(const TTimer& t):startTimeMs(t.startTimeMs) {}
	TTimer(int _startTimeMs):startTimeMs(_startTimeMs) {}
	
	bool operator==(const TTimer& t)const { return startTimeMs == t.startTimeMs; }
	void operator=(const TTimer& t) {startTimeMs = t.startTimeMs; }
};

#if USING_SHARED_TIMER_THREAD

class SharedTimer {
	static int globalTimeMs;
public:
	
	int startTimeMs;

	static void updateGlobalTime();
	static int getGlobalTimeMs();
	
	int getTimePassedMs()const;
	std::string getTimePassedStr()const;
	
	// more efficient than the two separate calls, plus ensures no gap between getting time, and resetting ...
	int getTimePassedAndReset();
	
	void setTimePassedMs(int n);
	void addTimePassedMs(int ms);
	
	void reset();
	SharedTimer();
	
	bool operator==(const SharedTimer& t)const { return startTimeMs == t.startTimeMs; }
	void operator=(const SharedTimer& t) {startTimeMs = t.startTimeMs; }
};

typedef SharedTimer Timer;

#else

typedef TTimer Timer;

#endif // #if USING_SHARED_TIMER_THREAD


struct CountdownTimer {
	Timer timer;
	CountdownTimer(int timeInMs) {
		reset(timeInMs);
	}
	void reset(int timeInMs) {
		timer.setTimePassedMs(-timeInMs);
	}
	bool isFinished()const { return timer.getTimePassedMs() >= 0; }
	void topUp(int ms) { timer.addTimePassedMs(-ms); }
	void advance(int ms) { timer.addTimePassedMs(ms); }
	int getTimeRemaining()const { return -1 * timer.getTimePassedMs(); }
};


#endif // #ifndef _TTimer_h
