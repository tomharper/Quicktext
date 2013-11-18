#ifndef _TAtomicWrapper_h
#define _TAtomicWrapper_h

#include "TCommon.h"
#include <stdint.h>
#include <stddef.h>

#define USE_CPP11_EXCHANGE 0

/**
 This is designed to be useable from C files (as needed for NetEQ for example).
 C++ files may prefer to use the AtomicWrapper.h classes for example).
 */
#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
	
	/**
	 The following four functions all return the new value
	 */
	int32_t TAtomicIncrement(int32_t* i);
	int32_t TAtomicDecrement(int32_t* i);
	int32_t TAtomicAdd(int32_t* i, int32_t amountToAdd);
	int32_t TAtomicSub(int32_t* i, int32_t amountToAdd);
	
	/**
	 The following functions compare the value in oldValue to the value in the memory location referenced
	 by theValue. If the values match, this function stores the value from newValue into that memory location
	 atomically.
	 
	 TCompareAndSwap has barrier - could be split into barrierless + with-barrier
	 
	 Returns bool - true on match, false otherwise (but called int as C sucks)
	 */
	int TCompareAndSwap(int32_t oldValue, int32_t newValue, int32_t* theValue);
	int TCompareAndSwapPtr(void *oldValue, void *newValue, void* volatile* theValue);
	int TCompareAndSwapPtrBarrier(void *oldValue, void *newValue, void* volatile* theValue);
	
	void* TAtomicExchange(void* volatile* mem, void* newValue);
	
#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

#if USE_CPP11_EXCHANGE
#include <atomic>
#endif

// use own implementation (copied from webrtc, but thus removing the dependency on webrtc)
#if 1

// 32 bit atomic variable.  Note that this class relies on the compiler to
// align the 32 bit value correctly (on a 32 bit boundary), so as long as you're
// not doing things like reinterpret_cast over some custom allocated memory
// without being careful with alignment, you should be fine.

class TAtomic32
{
public:
	TAtomic32(int32_t initialValue = 0);
	~TAtomic32();
	
	// Prefix operator!
	int32_t operator++();
	int32_t operator--();
	
	int32_t operator+=(int32_t value);
	int32_t operator-=(int32_t value);
	
	// Sets the value atomically to newValue if the value equals compare value.
	// The function returns true if the exchange happened.
	bool CompareExchange(int32_t newValue, int32_t compareValue);
	int32_t Value() const;
	
private:
	// Disable the + and - operator since it's unclear what these operations
	// should do.
	TAtomic32 operator+(const TAtomic32& other);
	TAtomic32 operator-(const TAtomic32& other);
	
	// Checks if |_value| is 32bit aligned.
	inline bool Is32bitAligned() const {
      return (reinterpret_cast<ptrdiff_t>(&_value) & 3) == 0;
	}
	
	//DISALLOW_COPY_AND_ASSIGN(TAtomic32);
	
	int32_t _value;
};

typedef TAtomic32 AtomicInt;

// C++11 atomic stuff - this part not yet working in latest xcode+clang
#elif 0

#include <atomic>
typedef std::atomic_int AtomicInt;
//std::atomic<Node*> x; std::atomic_exchange<Node*>(&this->head, n);

// google/webrtc - adds dependency on webrtc
#else

#include "Atomic32.h"
typedef webrtc::Atomic32 AtomicInt;

#endif

/*
class TAtomic {
public:
#if USE_CPP11_EXCHANGE
	static void* AtomicExchange(std::atomic<void*>* mem, void* newValue);
#else
	static void* AtomicExchange(void* volatile* mem, void* newValue);
#endif
};
*/

#endif // #ifdef __cplusplus

//
// memory barriers - taken from PortAudio - pa_memorybarrier.h
//
#if defined(__APPLE__)
#   include <libkern/OSAtomic.h>
/* Here are the memory barrier functions. Mac OS X only provides
 full memory barriers, so the three types of barriers are the same,
 however, these barriers are superior to compiler-based ones. */
#   define TFullMemoryBarrier()  OSMemoryBarrier()
#   define TReadMemoryBarrier()  OSMemoryBarrier()
#   define TWriteMemoryBarrier() OSMemoryBarrier()
#elif defined(__GNUC__)
/* GCC >= 4.1 has built-in intrinsics. We'll use those */
#   if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)
#      define TFullMemoryBarrier()  __sync_synchronize()
#      define TReadMemoryBarrier()  __sync_synchronize()
#      define TWriteMemoryBarrier() __sync_synchronize()
/* as a fallback, GCC understands volatile asm and "memory" to mean it
 * should not reorder memory read/writes */
/* Note that it is not clear that any compiler actually defines __PPC__,
 * it can probably removed safely. */
#   elif defined( __ppc__ ) || defined( __powerpc__) || defined( __PPC__ )
#      define TFullMemoryBarrier()  asm volatile("sync":::"memory")
#      define TReadMemoryBarrier()  asm volatile("sync":::"memory")
#      define TWriteMemoryBarrier() asm volatile("sync":::"memory")
#   elif defined( __i386__ ) || defined( __i486__ ) || defined( __i586__ ) || \
defined( __i686__ ) || defined( __x86_64__ )
#      define TFullMemoryBarrier()  asm volatile("mfence":::"memory")
#      define TReadMemoryBarrier()  asm volatile("lfence":::"memory")
#      define TWriteMemoryBarrier() asm volatile("sfence":::"memory")
#   else
#      ifdef ALLOW_SMP_DANGERS
#         warning Memory barriers not defined on this system or system unknown
#         warning For SMP safety, you should fix this.
#         define TFullMemoryBarrier()
#         define TReadMemoryBarrier()
#         define TWriteMemoryBarrier()
#      else
#         error Memory barriers are not defined on this system. You can still compile by defining ALLOW_SMP_DANGERS, but SMP safety will not be guaranteed.
#      endif
#   endif
#elif (_MSC_VER >= 1400) && !defined(_WIN32_WCE)
#   include <intrin.h>
#   pragma intrinsic(_ReadWriteBarrier)
#   pragma intrinsic(_ReadBarrier)
#   pragma intrinsic(_WriteBarrier)
#   define TFullMemoryBarrier()  _ReadWriteBarrier()
#   define TReadMemoryBarrier()  _ReadBarrier()
#   define TWriteMemoryBarrier() _WriteBarrier()
#elif defined(_WIN32_WCE)
#   define TFullMemoryBarrier()
#   define TReadMemoryBarrier()
#   define TWriteMemoryBarrier()
#elif defined(_MSC_VER) || defined(__BORLANDC__)
#   define TFullMemoryBarrier()  _asm { lock add    [esp], 0 }
#   define TReadMemoryBarrier()  _asm { lock add    [esp], 0 }
#   define TWriteMemoryBarrier() _asm { lock add    [esp], 0 }
#else
#   ifdef ALLOW_SMP_DANGERS
#      warning Memory barriers not defined on this system or system unknown
#      warning For SMP safety, you should fix this.
#      define TFullMemoryBarrier()
#      define TReadMemoryBarrier()
#      define TWriteMemoryBarrier()
#   else
#      error Memory barriers are not defined on this system. You can still compile by defining ALLOW_SMP_DANGERS, but SMP safety will not be guaranteed.
#   endif
#endif




#endif // #ifndef _AtomicWrapper_h
