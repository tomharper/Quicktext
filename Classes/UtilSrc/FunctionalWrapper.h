#ifndef _FunctionalWrapper_hpp
#define _FunctionalWrapper_hpp

#include "TCommon.h"
#include <functional>

/**
 Not using any of the C++11 functional stuff for passing method pointers, as:
 - has issues with being used without RTTI
 - not generally hugely beneficial in comparison with just declaring interfaces for callbacks
 */
#if USING_TR1
#	include <tr1/functional>
using namespace std::tr1;
using namespace std::tr1::placeholders;
#else
#	include <functional>
#endif
#endif
