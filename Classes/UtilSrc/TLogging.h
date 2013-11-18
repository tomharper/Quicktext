#ifndef _TLogging_h
#define _TLogging_h

#include "TCommon.h"
#include "TTypes.h" // need for int64_t usage .....

//
// Logging to File
//
// Enable this to log to file (should be able to then copy to computer using organizer, or if thats not 
// working/possible, can also 'upload' the log by e-mail in fling with cc.upload.log)
//
#if RELEASE_BUILD
#	define LOG_TO_FILE 0
#elif defined(_DEBUG)
#	define LOG_TO_FILE 1
#else
#	define LOG_TO_FILE 1
#endif

//
// Logging to Console
//
// Enable this to log to console (default when debugging in simulator, but when do this on device, i can never 
// see this logging anywhere??)
//
#if defined(_DEBUG) || PROFILING_ENABLED || defined(_WIN32) || !RELEASE_BUILD
#	if __APPLE__
#		define LOG_TO_CONSOLE 1 // disable, if using testflight
#	else
#		define LOG_TO_CONSOLE 1
#	endif
#else
#	define LOG_TO_CONSOLE 0
#endif

#if _WIN32
#	undef ERROR
#endif

#define TLogLevel_DBG 0
#define TLogLevel_INFO 1
#define TLogLevel_WARN 2
#define TLogLevel_ERROR 3


#ifdef __cplusplus

// for forward-dec of std::string (string.h includes this plus tons more, so surely better)
#include <iosfwd>
#include <vector>

/**
 was trying to forward-dec this, but breaks initializer_list use generally!?, so using '...'
 instead of initializer_list in the two functions ... since really want to avoid many includes,
 in this file, especially from STL, as included from so many places ... although initializer_list
 should be relatively lightweight ...
 
*/

enum class LogLevel { DBG=0, INFO, WARN, ERROR };

/**
 Functionality that requires C++ - mainly relating to controlling logging.
 */

class TLogging {
public:

	/**
	 bypasses all the printf-syntax parsing, just the raw string will be logged,
	 and used to bypass the normal size limit of that string due to the default 
	 PrintfBuf size
	 */
	static void logDirect(LogLevel logLevel, const std::string& logline);
	void parseCommandLineForLogConfigArgs(int argc, char* argv[]);
	
	static void ensureStaticsLoaded();
	
	static const char* getPrefix(LogLevel logLevel);
	
	static std::string getBuildInfo();
	static void logBuildInfo();
	
	static int vsnprintf(char* buf, size_t sz, const char* msg, va_list args);
	
#if defined(__APPLE__)
	// internal helper fn ...
	static int parseObjCLogMsg(char* destBuf, const char* msg, va_list args, int lenLimit=-1);
#endif
	
};

extern "C" {
#endif //__cplusplus

//static string getLastAssert();

// Needs to have default visibility such that
void TLogging_log(int/*LogLevel*/ logLevel, const char* file, int line, const char* fn, const char* msg, ...) __attribute__((visibility("default")));

// extends vsprintf to work with %@ as well if included in msg ....

void TLogging_flush();

void TLogging_printStackTrace(int skipFirstN, const char* msg, ...);
	
#ifdef __cplusplus
}
#endif

#	define LOGGING_ENABLED 1

#if LOGGING_ENABLED
//#	define LogInfoEvery(ms, ...) { static ClockOffsetIgnoringPauses _l_coip; if(_l_coip.isNull()) _l_coip.reset(); if(_l_coip.getTimePassed() > ms) { _l_coip.reset(); Logging::log("\n", __VA_ARGS__); } }
#	define TLog(logLevel, ...)  { TLogging_log((int)logLevel, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); }
#	define TLogDebug(...)  { TLogging_log(TLogLevel_DBG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); }
#	define TLogInfo(...)  { TLogging_log(TLogLevel_INFO, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); }
#	define TLogWarn(...)  { TLogging_log(TLogLevel_WARN, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); }
#	define TLogError(...) { TLogging_log(TLogLevel_ERROR, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); }

#	define TLogDebug_(...)  { TLogging_log(TLogLevel_DBG, NULL, -1, NULL, __VA_ARGS__); }
#	define TLogInfo_(...)  { TLogging_log(TLogLevel_INFO, NULL, -1, NULL, __VA_ARGS__); }
#	define TLogWarn_(...)  { TLogging_log(TLogLevel_WARN, NULL, -1, NULL, __VA_ARGS__); }
#	define TLogError_(...)  { TLogging_log(TLogLevel_ERROR, NULL, -1, NULL, __VA_ARGS__); }
#else
#	define TLog(...)
#	define TLogInfoEvery(...)
#	define TLogDebug(...)
#	define TLogInfo(...)
#	define TLogWarn(...)
#	define TLogError(...)
#	define TLogDebugIf(...)
#	define TLogInfoIf(...)
#	define TLogWarnIf(...)
#	define TLogErrorIf(...)

#	define TLogDebug_(...)
#	define TLogInfo_(...)
#	define TLogWarn_(...)
#	define TLogError_(...)

class TLoggingReceiverI {
public:
    virtual void log(LogLevel logLevel, const char* msg)=0;
    virtual ~LoggingReceiverI() {}
};
#endif // #if LOGGING_ENABLED

#endif // #ifndef _TLogging_h
