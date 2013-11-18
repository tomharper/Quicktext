/*
 Mainly here for handling of 64-bit ints ...
 
 Expected to provide, cross-platform, int64_t, PRId64, PRIx64, PRIX64
 */
#ifndef _TTypes_h
#define _TTypes_h

#include <inttypes.h>

// this gives TARGET_OS_IPHONE, TARGET_OS_SIMULATOR etc, and should be lightweight to include ...
#ifdef __APPLE__
#	include "TargetConditionals.h"
#endif

#ifndef USING_TR1
#	define USING_TR1 0
#endif

#define ENABLE_DEFAULT_MOVE_SEMANTICS(TypeName) \
	TypeName& operator=(TypeName&&)=default; \
	TypeName(TypeName&&)=default;

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

// For some reason Android inttypes does not define PRId64 for C++
#ifdef ANDROID
#	define PRId64 "lld" /* int64_t */
#	define PRIx64 "llx"
#	define PRIX64 "llX"
#endif

#endif
