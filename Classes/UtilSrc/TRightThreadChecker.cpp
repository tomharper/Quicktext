#if defined(ANDROID) || defined(WEBRTC_ANDROID) || defined(WEBRTC_LINUX)
#include <stdlib.h>
#include <sys/prctl.h>
#endif

#include "TRightThreadChecker.h"
#include "TThreadI.h"
#include "ThreadLocalValue.h"

ThreadLocalValue<TRightThreadChecker::ThreadId> rtc_threadId;


const char* TRightThreadChecker::getCurrentThreadName() {
	return rtc_threadId.getValue();
}



#ifdef WIN32
// As seen on MSDN.
// http://msdn.microsoft.com/en-us/library/xcb2z8hs(VS.71).aspx
#define MSDEV_SET_THREAD_NAME  0x406D1388
typedef struct tagTHREADNAME_INFO {
	DWORD dwType;
	LPCSTR szName;
	DWORD dwThreadID;
	DWORD dwFlags;
} THREADNAME_INFO;

void SetThreadName(DWORD dwThreadID, LPCSTR szThreadName) {
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = szThreadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;
	
	__try {
		RaiseException(MSDEV_SET_THREAD_NAME, 0, sizeof(info) / sizeof(DWORD),
							reinterpret_cast<ULONG_PTR*>(&info));
	}
	__except(EXCEPTION_CONTINUE_EXECUTION) {
	}
}
#endif  // WIN32

void TRightThreadChecker::registerThread(ThreadId name) {
	rtc_threadId.setValue(name);
	
#if defined(WIN32)
	SetThreadName(GetCurrentThreadId(), name_.c_str());
#elif defined(__APPLE__) || defined(POSIX)
	pthread_setname_np(name);
#else //if defined(WEBRTC_LINUX)
	prctl(PR_SET_NAME, (unsigned long)name, 0, 0, 0);
#endif
}

