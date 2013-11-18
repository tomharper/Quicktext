#include "TAtomic.h"

#ifdef __APPLE__ // defined(OSX) || defined(IOS) || defined(_MAC)

#include <libkern/OSAtomic.h>

int32_t TAtomicIncrement(int32_t* i) { return OSAtomicIncrement32Barrier(i); }
int32_t TAtomicDecrement(int32_t* i) { return OSAtomicDecrement32Barrier(i); }

int32_t TAtomicAdd(int32_t* i, int32_t amountToAdd) { return OSAtomicAdd32Barrier(amountToAdd, i); }
int32_t TAtomicSub(int32_t* i, int32_t amountToSub) { return OSAtomicAdd32Barrier(-amountToSub, i); }

int TCompareAndSwap(int32_t oldValue, int32_t newValue, int32_t* theValue) {
	return OSAtomicCompareAndSwap32Barrier(oldValue, newValue, theValue);
}

int TCompareAndSwapPtr(void *oldValue, void *newValue, void* volatile* theValue) {
	return OSAtomicCompareAndSwapPtr(oldValue, newValue, theValue);
}
int TCompareAndSwapPtrBarrier(void *oldValue, void *newValue, void* volatile* theValue) {
	return OSAtomicCompareAndSwapPtrBarrier(oldValue, newValue, theValue);
}
#elif defined(ANDROID)
int32_t TAtomicIncrement(int32_t* i) { return __sync_fetch_and_add(i, 1) + 1; }
int32_t TAtomicDecrement(int32_t* i) { return __sync_fetch_and_sub(i, 1) - 1; }

int32_t TAtomicAdd(int32_t* i, int32_t amountToAdd) { return __sync_fetch_and_add(i, amountToAdd) + amountToAdd; }
int32_t TAtomicSub(int32_t* i, int32_t amountToSub) { return __sync_fetch_and_sub(i, amountToSub) - amountToSub; }

int TCompareAndSwap(int32_t oldValue, int32_t newValue, int32_t* theValue) {
	return __sync_bool_compare_and_swap(theValue, oldValue, newValue);
}

int TCompareAndSwapPtr(void *oldValue, void *newValue, void* volatile* theValue) {
	return __sync_bool_compare_and_swap(theValue, oldValue, newValue);
}
int TCompareAndSwapPtrBarrier(void *oldValue, void *newValue, void* volatile* theValue) {
	int ret;
	__sync_synchronize(ret = __sync_bool_compare_and_swap(theValue, oldValue, newValue));

	return ret;
}


#elif defined(LINUX) /*|| defined(ANDROID)*/

static_assert(false, "TODO: adapt from atomic32_posix.cc");


#elif defined(WIN32) //WIN

static_assert(false, "TODO: adapt from atomic32_win.cc");

#endif

TAtomic32::TAtomic32(int32_t initialValue) : _value(initialValue) {

}

TAtomic32::~TAtomic32() { }

int32_t TAtomic32::operator++() {
	return __sync_fetch_and_add(&_value, 1) + 1;
}

int32_t TAtomic32::operator--() {
	return __sync_fetch_and_sub(&_value, 1) - 1;
}

int32_t TAtomic32::operator+=(int32_t value) {
	int32_t returnValue = __sync_fetch_and_add(&_value, value);
	returnValue += value;
	return returnValue;
}

int32_t TAtomic32::operator-=(int32_t value) {
	int32_t returnValue = __sync_fetch_and_sub(&_value, value);
	returnValue -= value;
	return returnValue;
}

bool TAtomic32::CompareExchange(int32_t newValue, int32_t compareValue) {
	return __sync_bool_compare_and_swap(&_value, compareValue, newValue);
}

int32_t TAtomic32::Value() const {
	return _value;
}




/**
 Tried a few ways to get atomic_exchange working (for iOS) ....
 */

// doesn't compile for me, looks like some internal issue .....
#if USE_CPP11_EXCHANGE
void* TAtomic::AtomicExchange(std::atomic<void*>& mem, void* newValue) {
	return std::atomic_exchange<void*>(&mem, newValue); // serialization-point wrt producers, acquire-release
}
#else
void* TAtomicExchange(void* volatile* mem, void* newValue) {
	
	//
	// this is how chromium implements AtomicExchange - in native_client/src/include ...
	//
	// http://www.1024cores.net/home/lock-free-algorithms/your-arsenal mentions that this is what is provided by gcc
	//
#if 1
	return __sync_lock_test_and_set(mem, newValue);
	
	//
	// chromium's linux x86 code (it also has code for ARM) ...
	//
	/*
	__asm__ __volatile__("xchgl %1,%0"
								: "=r" (newValue)
								: "m" (*mem), "0" (newValue)
								: "memory");
	return new_value;  // Now it's the previous value.
	*/
	
	//
	// found this asm code for ARM on http://sources.redhat.com/ml/libc-ports/2005-10/msg00016.html
	// - haven't tested it: was going to but then found the above __sync_lock_test_and_set
	//
#elif 0

#include <stdlib.h>
#define atomic_exchange_acq(mem, newvalue) \
	({ __typeof (*mem) result; \
	if (sizeof (*mem) == 1) \
		__asm__ __volatile__ ("swpb %0, %1, [%2]" \
										: "=&r,&r" (result) \
										: "r,0" (newvalue), "r,r" (mem) : "memory"); \
	else if (sizeof (*mem) == 4) \
		__asm__ __volatile__ ("swp %0, %1, [%2]" \
										: "=&r,&r" (result) \
										: "r,0" (newvalue), "r,r" (mem) : "memory"); \
	else { \
		result = 0; \
		abort (); \
	} \
	result; })

	return atomic_exchange_acq(mem, newValue);
	
	// workaround using CAS - but probably can cause (ABA?) issues
#else
	for(;;) {
		void* result = *mem;
		if(TCompareAndSwapPtrBarrier(result, result, mem)) {
			return result;
		}
	}
#endif
	
}
#endif
