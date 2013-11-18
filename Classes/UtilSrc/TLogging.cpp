#include "TLogging.h"
#include "TUtils.h"
#include "TTimer.h"
#include "TThreadI.h"
#include "TVector.h"
#include "MemoryWrapper.h"
#include "Mutex.h"
#include "TRightThreadChecker.h"
#include "TThread.h"
#include "TDateTime.h"
#include <unordered_map>
#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <sstream>
#include "TCommon.h"
#include "TQueue.h"
#include "TStringRef.h"
#include "PrintfBuffer.h"
//#include "StrUtils.h"
//#include "TQueue.h"
//#include "TStaticArray.h"


#ifdef ANDROID
#include <android/log.h>
#endif

using namespace std;

#define MAX_BUF_SIZE 200000 //32768

//
// when debugging, may often want to disable the bg-thread, so that when breaking for some reason, sure that have the most
// recent loglines visible
// - note that disabling the bg-thread can impact performance when logging a lot
//
#ifdef _DEBUG
#	define LOG_IN_BG_THREAD 0
#else
#	define LOG_IN_BG_THREAD 1
#endif

#define DO_LOGGING_TESTS 1

#if defined(_DEBUG) || !RELEASE_BUILD 
#	define FLUSH_LOG_OFTEN 0
#else
#	define FLUSH_LOG_OFTEN 0
#endif

string* getLastAssertStr() {
	static string lastAssert;
	return &lastAssert;
}

// just used to lock between passing loglines to receivers, and adding/removing of receivers
Mutex* getLoggingReceiversMutex() {
	static Mutex m;
	return &m;
}


// just used to lock between normal logging and flushing 
Mutex* getQueueReadMutex() {
	static Mutex m;
	return &m;
}

// TODO: this is presumably pretty platform-dependent ...
#ifndef ANDROID
#include <execinfo.h>
#endif
unordered_map<void*, string> backtraceSymbols;
class StackInf {
private:
	int skipFirst;
	void* returnAddresses[32];

	template<class PrintfBufT> void add(PrintfBufT& pbuf, const char* s, int& nPrinted) {
		if(pbuf.charsRemaining() < 500) {}
		else { pbuf.printf(" %-2d %s\n", nPrinted++, s); }
	}

	template<class PrintfBufT> void getAndPrintSymbols(PrintfBufT& pbuf, int start, int n, int& nPrinted) {
#ifndef ANDROID
		char **symbols = backtrace_symbols(&returnAddresses[start], n);
		
		for(int i=0; i<n; i++) {
			char* p = &symbols[i][4];
			backtraceSymbols[returnAddresses[i+start]] = p;
			add(pbuf, p, nPrinted);
		}
		free(symbols);
#endif
	}
	
public:
	int depth;

	StackInf(int _skipFirst):skipFirst(_skipFirst) {
#ifdef ANDROID
		depth = 0;
#else
		depth = backtrace(returnAddresses, sizeof returnAddresses / sizeof *returnAddresses);
#endif
	}
	template<class PrintfBufT>
	void outputSymbols(PrintfBufT& pbuf) {
		
#if 1 // caching for speed, else the symbol lookups seems to be quite slow ....
		int reducedDepth = min(20, depth-2);
		int lastSymbolThatHad = skipFirst-1;
		int nPrinted = 0;
		for(int i=skipFirst; i<reducedDepth; i++) {
			void** p = &returnAddresses[i];
			auto it = backtraceSymbols.find(*p);
			
			if(it != backtraceSymbols.end()) {
				if(i-lastSymbolThatHad > 1) {
					getAndPrintSymbols(pbuf, lastSymbolThatHad+1, i-(lastSymbolThatHad+1), nPrinted);
				}
				add(pbuf, it->second.c_str(), nPrinted);
				lastSymbolThatHad = i;
			}
		}
		if(reducedDepth-lastSymbolThatHad > 1) {
			getAndPrintSymbols(pbuf, lastSymbolThatHad+1, reducedDepth-(lastSymbolThatHad+1), nPrinted);
		}
		if(pbuf.charsRemaining() < 500) {
			pbuf.printf("....\n");
		}
#else
		char **symbols = backtrace_symbols(&returnAddresses[skipFirst], reducedDepth-skipFirst);
		for (int i = 0; i < reducedDepth-skipFirst; ++i) {
			pbuf.printf("  %s\n", symbols[i]);
			if(pbuf.charsRemaining() < 500) {
				pbuf.printf("....\n");
				break;
			}
		}
		free(symbols);
#endif
	}
};

struct LogLine : public TQueue::Node {
	LogLevel logLevel;
	TStringRef data;
	StackInf* stackInf = nullptr;
	
	LogLine() {}
	
	void reset() {
		if(data.c_str()) {
			delete data.c_str();
			data = TStringRef();
		}
		if(stackInf) {
			delete stackInf;
			stackInf = nullptr;
		}
	}
	~LogLine() {
		reset();
	}
	DISALLOW_COPY_AND_ASSIGN(LogLine);
};

#if LOG_TO_FILE

// note the logfile-name, may want to access it for logfile-uploading, ....
std::string zLogfileName;
FILE* getLogfile() {
	static FILE* logfile = nullptr;
	if(!logfile) {
		
		bool createdSubdir = TUtils::ensureStateFileSubdirExists("logs", false);
		
		zLogfileName = TUtils::appendDateTimeToFilename("logfile.txt");
		
		if(createdSubdir) {
			zLogfileName = std::string("logs/") + zLogfileName;
		}
		
#		ifndef NON_IPHONE
			zLogfileName = TUtils::getStateFilenameWithPath(zLogfileName);
#		endif
		logfile = fopen(zLogfileName.c_str(), "a");
#		if ANDROID
		//			initedLogging = true;
#		endif
		
		//PrintfBuf buf; buf.printf("Logging started to file: %s\n", zLogfileName.c_str());
		//fputs(buf.c_str(), logfile);
	}
	return logfile;
}
#endif

class DateAndTimeLogger {
	const char* threadName = nullptr;
public:
	DateAndTimeLogger():threadName(TRightThreadChecker::getCurrentThreadName()) {}
	
	// returns length required, not counting trailing null
	int getBufSizeReq() {
		//TODO: not totally ideal that calculating the strlen(threadName) continually.....
		return 30
			+ (threadName?strlen(threadName):0)
			+ 4;  // for prefix
		;
	}
	
	char* logDateAndTime(char* buf, LogLevel logLevel) {
		
		const char* prefix = TLogging::getPrefix(logLevel);

		buf += sprintf(buf, "%s", prefix); //fputs(prefix, dest);
		
		//*buf++ = ' ';
		buf += TDateTime::now().outputLocalTimeToBuf(buf, TDateTime::TOF_COMPACT);
		*buf++ = ' ';
		
			//sprintf(buf, " %02d%02d%02d %02d:%02d:%02d.%03d ", st.tm_year%100, st.tm_mon+1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, ms );
		
		const char* threadName = TRightThreadChecker::getCurrentThreadName();
		if(threadName) {
			buf += sprintf(buf, "[%s:%04X] ", threadName, TThreadI::getCurrentThreadId());
		}
		else {
			buf += sprintf(buf, "[:%04X] ", TThreadI::getCurrentThreadId());
		}
		
		return buf;
	}
};


string TLogging::getBuildInfo() {
	PrintfBuf buf(1024);
	
#if !defined(TARGET_OS_MAC) && !defined(_WIN32)
	buf.printf("Device model: %s\n", TUtils::getModel().c_str());
	buf.printf("System version: %s\n", TUtils::getSystemVersion().c_str());
#endif
	buf.printf("Build date: %s %s\n", __DATE__, __TIME__);
	buf.printf("Build flags: ");
#ifdef NDEBUG
	buf.printf("NDEBUG, ");
#endif
#ifdef _DEBUG
	buf.printf("_DEBUG, ");
#endif
#if RELEASE_BUILD
	buf.printf("RELEASE_BUILD, ");
#endif
#ifdef ANDROID
	buf.printf("ANDROID, ");
#endif
#ifdef TARGET_IPHONE_SIMULATOR
	buf.printf("TARGET_IPHONE_SIMULATOR:%d, ", TARGET_IPHONE_SIMULATOR);
#endif
#ifdef TARGET_OS_MAC
	buf.printf("TARGET_OS_MAC:%d, ", TARGET_OS_MAC);
#endif
#ifdef TARGET_OS_IPHONE
	buf.printf("TARGET_OS_IPHONE:%d, ", TARGET_OS_IPHONE);
#endif
#ifdef LOG_IN_BG_THREAD
	buf.printf("LOG_IN_BG_THREAD:%d, ", LOG_IN_BG_THREAD);
#endif
#ifdef FLUSH_LOG_OFTEN
	buf.printf("FLUSH_LOG_OFTEN:%d, ", FLUSH_LOG_OFTEN);
#endif
	
	return buf.str();
}
void TLogging::logBuildInfo() {
	TLogInfo("Build Info:\n%s", getBuildInfo().c_str());
}

const char* zLoggingPrefixes[] =  { "DBG:", "INF:", "WRN:", "ERR: ", "???: ", "???: " };
int zLoggingPrefixLengths[] =  { 3, 4, 4, 3, -1, -1 };

const char* TLogging::getPrefix(LogLevel logLevel) {
	return zLoggingPrefixes[(int)logLevel];
}

void TLogging_flushFile(FILE* dest) {
#if LOG_TO_FILE
	if(dest == getLogfile()) {
		// FIXME
		//flushToFile(dest);
	}
#endif
	fflush(dest);
}


/**
 ll.data is expected to be normal, like have size/length == strlen(str), end with a trailing zero, ...
 */
void outputLogLineTo(LogLine& ll, FILE* dest, bool compress) {

	if (dest == NULL)
		return;

	// now outputting in outputDateTime ...
	//const char* prefix = TLogging::getPrefix(ll.logLevel);
	//fputs(prefix, dest);
	
	if(compress) {
		ll.data.length++;
		char* p = const_cast<char*>(ll.data.data);
		p[ll.data.length-1] = '\n';

		// FIXME
//		addDataToFile(&ll.data[0], ll.data.size(), dest);

		p[ll.data.length-1] = 0;
		ll.data.length--;
	}
	else {
		fputs(ll.data.c_str(), dest); //vfprintf(logfile, "%s", buf);
		fputc('\n', dest);
	}
	
#	if FLUSH_LOG_OFTEN
		TLogging_flushFile(dest);
#	endif
}

void outputLogLine(LogLine& ll) {
	if(ll.stackInf) {
		Timer t;
		StackInf* si = ll.stackInf;
		PrintfBuf pbuf(4096);
		pbuf.printf("%s - dumping Stacktrace, depth:%d\n", ll.data.c_str(), si->depth);
		
		si->outputSymbols(pbuf);

		pbuf.printf("(dumptime: %dms)\n", t.getTimePassedMs());
		
		int bufSize = strlen(pbuf.c_str());
		
		// TODO: get from ringbuffer/circularbuffer
		char* buf = new char[bufSize+1];
		memcpy(buf, pbuf.c_str(), bufSize+1);
		
		ll.reset(); // delete the existing data (+ si)!
		ll.data = buf; //pbuf.str();
	}
	
	// to console (if enabled)
#if LOG_TO_CONSOLE
#	if ANDROID
		android_LogPriority priority = ANDROID_LOG_DEBUG;
		if (ll.logLevel == LogLevel::ERROR) priority = ANDROID_LOG_ERROR;
		else if (ll.logLevel == LogLevel::WARN) priority = ANDROID_LOG_WARN;
	
		__android_log_write(priority, "Proto", ll.data.c_str());
#	else
		outputLogLineTo(ll, stdout, false);
#	endif
#endif
	// to file (if enabled)
#if LOG_TO_FILE
		outputLogLineTo(ll, getLogfile(), 1);
#endif
// here we could output to log receivers, but we realistically don't have any
}


#pragma mark - LoggingThread


#if LOG_IN_BG_THREAD

class LoggingThread: public TThreadI {
public:
	int linesLogged = 0;
	
	TQueueWithCounter loggingQueue;

	LoggingThread():TThreadI("Logging") {

		// still very occassionally get the wierdest thing that have TWO logging-threads (both belonging to the same LoggingThread object??)
		// - don't see how this is possible??
		// --- ahhhh, get this if get an 
		static int runCount = 0;
		
		runCount++;
		
		TThreadI::start(TThread::kLowPriority, false);
	}
	
	~LoggingThread() {
		signalAndWaitForStop();
	}
	
protected:
	void run();
};

LoggingThread* getLoggingThreadInst() {
	static LoggingThread loggingThread;
	return &loggingThread;
}

void LoggingThread::run() {
	// don't log here or can get the exact recursion that the below lines are checking for ....
	//LogInfo("LoggingThread - start: FLUSH_LOG_OFTEN: %d, LOG_TO_FILE: %d, LOG_TO_CONSOLE: %d", FLUSH_LOG_OFTEN, LOG_TO_FILE, LOG_TO_CONSOLE);
	
	// had the wierdest thing that was having TWO logging-threads (both belonging to the same LoggingThread object??)
	{
		static int runCount = 0;
		++runCount;
	}
	
	for(;;) {

		if(waitOnStopSignalOrTimeout(250)) {
			break;
		}
		
		Lock l(*getQueueReadMutex());
		
		while(TQueue::Node* node = loggingQueue.pop()) {
			l.unlock();
			
			LogLine* ll = (LogLine*)node;
			
			
			outputLogLine(*ll);

			delete ll;
			
			linesLogged++;
			
			if(queryNeedToStop()) {
				break;
			}
			
			l.lock();
		}
	}
}

#endif

void TLogging_flush() {
#if LOG_IN_BG_THREAD
	Lock l(*getQueueReadMutex());
	
	auto* inst = getLoggingThreadInst();
	while(TQueue::Node* node = inst->loggingQueue.pop()) {
		LogLine* ll = (LogLine*)node;
		outputLogLine(*ll);
		delete ll;
		inst->linesLogged++;
	}
#endif
	TLogging_flushFile(getLogfile());
}


#pragma mark - TLogging::log

void processLogLine(LogLine* ll) {

#if LOG_IN_BG_THREAD
	getLoggingThreadInst()->loggingQueue.push(ll);
#else
	static Mutex outputMutex;
	Lock l(outputMutex);
	outputLogLine(*ll); delete ll;
#endif
}

void TLogging::logDirect(LogLevel logLevel, const string& s) {
	auto* ll = new LogLine();
	ll->logLevel = logLevel;
	
	DateAndTimeLogger dtl;
	
	// 64 as don't know the length of the threadname that logDateAndTime will output!!!!
	int bufSize = dtl.getBufSizeReq() + s.size() +16; // +1 for null terminating char

	// TODO: get from ringbuffer/circularbuffer
	char* buf = new char[bufSize];
	char* p = dtl.logDateAndTime(buf, logLevel);
	memcpy(p, &s[0], s.size() +1); // +1 for null terminating char
	
	int len = strlen(buf);
	ll->data = TStringRef(buf, len);
	
	processLogLine(ll);
}

int TLogging::vsnprintf(char* buf, size_t sz, const char* msg, va_list args) {
	
#if __APPLE__ //TARGET_OS_IPHONE
	bool found = false;
	int len = strlen(msg);
	for(int i=0; i<len-1; i++) {
		if(msg[i] == '%' && msg[i+1] == '@') {
			found = true;
			break;
		}
	}

	//
	// assuming worst case (close-to-32kb, with a bit left for a buffer for prefix to keep total size
	// under 32kb) for obj-c logging, since can't estimate ... and then if the string is actually
	// still larger than this, then truncating it in parseObjCLogMsg so don't crash
	//
	if(found) {
		if(buf == nullptr) {
			return 32400;
		}
		else {
			return TLogging::parseObjCLogMsg(buf, msg, args, 32300);
		}
	}
	else
#endif

		
	{
		/* Make sure to copy args, as they are
		 * clobbered on 64bit systems
		 */
		va_list cargs;
		va_copy(cargs, args);

		int mlen = ::vsnprintf(buf, sz, msg, cargs);
		
		va_end(cargs);

		return mlen;
	}
}

string modulePrefixes[3] = { "/zproto-ios/", "/webrtc/", "/" };


void TLogging_log(int/*LogLevel*/ logLevel, const char* file, int line, const char* fn, const char* msg, ...) {

	int fileIndex = 0;
	if(file) {
		TStringRef ff(file);

		for(auto str: modulePrefixes) {
			auto index = ff.rfind(str);
			if(index != TStringRef::npos) {
				fileIndex = index + str.size();
				break;
			}
		}
	}
		
	va_list args;
	va_start(args, msg);

	DateAndTimeLogger dtl;
	
	// date + time, i.e. ' 16:23:37.848 ' + thread, i.e. ' [:0C07] '
	int lenEst = dtl.getBufSizeReq();
		
	if(file) {
		lenEst += snprintf(nullptr, 0, "%s: %s:%d: ", &file[fileIndex], fn, line);
	}
	lenEst += TLogging::vsnprintf(nullptr, 0, msg, args);
	lenEst++; // trailing null
	
	lenEst = min(MAX_BUF_SIZE, lenEst);
	lenEst += 16;

	// TODO: get from ringbuffer/circularbuffer
	char* buf = new char[lenEst+2]; // +2: extra space at end on top of estimate used for alloc+truncation, for \n to be added, ...
	char* p = buf;

	p = dtl.logDateAndTime(p, (LogLevel)logLevel);

	if(file) {
		//for(int i=0; i<fileLen; i++) { if(file[i] == '/' || file[i] == '\\') { fileIndex = i+1; } }
		p += sprintf(p, "%s: %s:%d: ", &file[fileIndex], fn, line);
	}
		
	size_t psz = TLogging::vsnprintf(p, (buf+lenEst)-p, msg, args);
	p += psz;
	
	va_end(args);
	
	// TODO: pool objects here to avoid the allocations ...
	auto* ll = new LogLine();
	ll->logLevel = (LogLevel)logLevel;
	ll->data = buf;

	processLogLine(ll);
}


void TLogging_printStackTrace(int skipFirst, const char* msg, ...) {
#if 1
	auto ll = new LogLine();
	ll->logLevel = LogLevel::WARN;

	char* buf = new char[4096];
	char* p = &buf[0];
	
	DateAndTimeLogger dtl;
	p = dtl.logDateAndTime(p, ll->logLevel);
	
	va_list args;
	va_start(args, msg);
	vsprintf(p, msg, args);
	va_end(args);
	
	ll->data = buf;
	ll->stackInf = new StackInf(skipFirst);
	
	processLogLine(ll);
	
#else
	
	Timer t;
	void *returnAddresses[256];
#ifdef ANDROID
	int depth = 0;
#else
	int depth = backtrace(returnAddresses, sizeof returnAddresses / sizeof *returnAddresses);
#endif
	int t0 = t.getTimePassedMs();
	
	//char buf[4096];
	PrintfBuffer<4096> pbuf;
	pbuf.printf("Dumping Stacktrace: ");
	va_list args;
	va_start(args, msg);
	pbuf.vprintf(msg, args);
	va_end(args);
	
	//TLogInfo_("Dumping Stacktrace: %s - stack depth = %d", buf, depth);
	pbuf.printf(" - stack depth = %d\n", depth);
	
	int reducedDepth = min(15, depth-2);
#ifdef ANDROID
	char **symbols = NULL;
#else
	char **symbols = backtrace_symbols(&returnAddresses[skipFirst], reducedDepth-skipFirst);
#endif
	int t1 = t.getTimePassedMs();
	for (int i = 0; i < reducedDepth-skipFirst; ++i) {
		pbuf.printf("  %s\n", symbols[i]);
		if(pbuf.charsRemaining() < 500) {
			pbuf.printf("....\n");
			break;
		}
	}
	TLogInfo_("%s (stackdump timeTaken: %dms,%dms,%dms)\n", pbuf.str().c_str(), t0, t1, t.getTimePassedMs());
	free(symbols);
	
#endif
}

void TLogging::ensureStaticsLoaded() {
	getLoggingReceiversMutex();
	getQueueReadMutex();
	
#if LOG_IN_BG_THREAD
	getLoggingThreadInst();
#endif
}

// have to do this when parseCommandLineForLogConfigArgs as can affect whether the diagnostics themselves get printed!
#define LogAndPrint(msg, ...) { printf(msg, __VA_ARGS__); printf("\n"); }

void TLogging::parseCommandLineForLogConfigArgs(int argc, char* argv[]) {
	if(argc > 1) {
		for(int i=1; i<argc; i++) {
			LogAndPrint("Processing command-line argument: %s", argv[i]);
			
			std::string s = argv[i];
			auto index = s.find("=");
			if(index == string::npos) {
				LogAndPrint("Ignoring command-line argument '%s' as no '=' - not of interest to me at least.", s.c_str());
			}
			else {
				std::string key = TUtils::trim(s.substr(0, index));
				std::string value = TUtils::trim(s.substr(index+1));
				std::string valueLC = TUtils::toLower(value);
				auto index2 = key.find(".");
				if(index2 == string::npos) {
					LogAndPrint("Ignoring command-line argument '%s' as key '%s' has no '.' - not of interest to me at least.", s.c_str(), key.c_str());
				}
				else {
					std::string targets = TUtils::trim(TUtils::toLower(key.substr(0, index2)));
					std::string fnName = TUtils::trim(TUtils::toLower(key.substr(index2+1)));
					
					if(fnName == "minloglevel") {
						LogLevel ll;
						
						if(valueLC == "debug" || valueLC == "dbg") {
							ll = LogLevel::DBG;
						}
						else if(valueLC == "warn" || valueLC == "wrn") {
							ll = LogLevel::WARN;
						}
						else if(valueLC == "error" || valueLC == "err") {
							ll = LogLevel::ERROR;
						}
						else if(valueLC == "info" || valueLC == "inf") {
							ll = LogLevel::INFO;
						}
						else {
							LogAndPrint("Invalid log-level '%s' for argument '%s', so ignoring", value.c_str(), s.c_str());
							continue;
						}
						LogAndPrint("Setting logLevel for target '%s' to '%s'", targets.c_str(), value.c_str());
					}
				}
			}
		}
	} 
}
