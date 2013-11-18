#ifndef _TRightThreadChecker_h
#define _TRightThreadChecker_h

#include "TCommon.h"

/**
 Allows us to check assumptions about what thread methods are being called from - i.e. so can be sure that
 no potential for blocking of realtime threads when mutexes clash.
 
 In non-debug builds it compiles to nothing and thus has no overhead
 */

class TRightThreadChecker {
public:
	typedef const char* ThreadId;
	// sets the thread's name, and sets a thread-local variable with the thread-name too (in theory should
	// provide quicker access?)
	static void registerThread(ThreadId name);
	static const char* getCurrentThreadName();
};

#else

#endif // #ifndef _RightThreadChecker_h

//enum ThreadId { SEND_THREAD=1, RCV_THREAD, INC_PACKETS_THREAD };
#define RTC_CAPTURE_THREAD "CaptureThread"
#define RTC_PLAYOUT_THREAD "PlayoutThread"
#define RTC_PROCESS_THREAD "ProcessThread"
#define RTC_TRACE_THREAD   "Trace"
