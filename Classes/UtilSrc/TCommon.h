#ifndef _TCommon_h
#define _TCommon_h

// this gives TARGET_OS_IPHONE, TARGET_OS_SIMULATOR etc, and should be lightweight to include ...
#ifdef __APPLE__
#	include "TargetConditionals.h"
#endif

/**
 #ifndef NDEBUG ... 
 */
#if !defined(NDEBUG) && !defined(_DEBUG)
#	define _DEBUG 1
#endif

/**
 Can use this to control the disabling of certain features for public releases (i.e. asserts, 
 logging, profiling)
 */
#ifndef RELEASE_BUILD
#	define RELEASE_BUILD 0
#endif

#ifdef ANDROID
#	define TRACK_DEADLOCKS 0
#else
#	define TRACK_DEADLOCKS 0
#endif

/**
 Can change this to enable/disable tracking lock hold durations and logging when they exceed some
 threshold ...
 */
#ifdef _DEBUG
#	define TRACK_LONG_LOCK_HOLDS 1
#else
#	define TRACK_LONG_LOCK_HOLDS 1
#endif
#define LONG_LOCK_HOLDS_THRESHOLD_MS 350


#ifndef USING_TR1
#	define USING_TR1 0
#endif

/**
 Some useful macros copied from webrtc ....
 */
#define TALK_BASE_CONSTRUCTORMAGIC_H_

#ifndef COMPILE_ASSERT
#define COMPILE_ASSERT(expression) switch(0){case 0: case expression:;}
#endif
#ifndef DISALLOW_ASSIGN
#define DISALLOW_ASSIGN(TypeName) void operator=(const TypeName&)=delete;
#endif

#define ENABLE_DEFAULT_MOVE_SEMANTICS(TypeName) \
	TypeName& operator=(TypeName&&)=default; \
	TypeName(TypeName&&)=default;

#define ENABLE_DEFAULT_COPY_SEMANTICS(TypeName) \
TypeName& operator=(const TypeName&)=default; \
TypeName(const TypeName&)=default;


// A macro to disallow the evil copy constructor and operator= functions
// This should be used in the private: declarations for a class
#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
	TypeName(const TypeName&)=delete; \
	DISALLOW_ASSIGN(TypeName)
#endif

// Alternative, less-accurate legacy name.
#ifndef DISALLOW_EVIL_CONSTRUCTORS
#define DISALLOW_EVIL_CONSTRUCTORS(TypeName) DISALLOW_COPY_AND_ASSIGN(TypeName)
#endif

// A macro to disallow all the implicit constructors, namely the
// default constructor, copy constructor and operator= functions.
//
// This should be used in the private: declarations for a class
// that wants to prevent anyone from instantiating it. This is
// especially useful for classes containing only static methods.
#ifndef DISALLOW_IMPLICIT_CONSTRUCTORS
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) TypeName(); DISALLOW_EVIL_CONSTRUCTORS(TypeName)
#endif

//TODO: could also use these to simplify assert macros? (didn't know was possible before ...)?
#define CONCATENATE_IMPL(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)

#ifdef __COUNTER__
#	define ANONYMOUS_VARIABLE(str) CONCATENATE(str, __COUNTER__)
#else
#	define ANNYMOUS_VARIABLE(str) CONCATENATE(str, __LINE__)
#endif

/**
 Some common macros ...
 */

#ifdef __cplusplus

class InternalUtils {
public:
	template<class T>
	static T abs(T v) {
		return ( (v>0) ? v : -v );
	}
	template<class T>
	static T sqr(T v) {
		return v*v;
	}
	template<class T>
	static void swap(T& x, T& y) {
		T t = x;
		x = y;
		y = t;
	}
};
#define  TMin(a,b)     (((a) < (b)) ? (a) :  (b))
#define  TMax(a,b)     (((a) > (b)) ? (a) :  (b))
#define  TInRange(a,b,c) (a >= b && a <= c)
#define  TRange(a,b,c) TMax(b, TMin(c, a))
#define  TAbs(a)    InternalUtils::abs(a)
#define  TSqr(a)    InternalUtils::sqr(a)
#define  TSwap(x,y) InternalUtils::swap(x,y)


//
// common forward-declarations
//
template<class KEY, class VALUE> class TMap;
template<class T> class TVector;

#endif // #ifdef __cplusplus

#endif
