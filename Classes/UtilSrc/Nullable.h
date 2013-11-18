#ifndef __Nullable__
#define __Nullable__

#include "TCommon.h"
#include "TAtomic.h"

#ifdef __cplusplus

#include <utility> // std::move

template<class T>
class Nullable {
	bool isSet_ = false;
	T t;
public:
	Nullable& operator=(const T& _t) { t = _t; isSet_ = true; return *this; }
	Nullable& operator=(T&& _t) { t = std::move(_t); isSet_ = true; return *this; }
	operator bool()const { return isSet(); }
   const T& operator->()const { return t; }
	T* operator->() { return &t; }
	
	void reset(const T& _t) { t = _t; isSet_ = true; }
	void reset(T&& _t) { t = std::move(_t); isSet_ = true; }
	void reset() { unset(); }
	void unset() { isSet_ = false; }
	bool isSet()const { return isSet_; }
	
	const T& get()const { return getValue(); }
	T& get() { return getValue(); }
	
	const T& getValue()const { return t; }
	T& getValue() { return t; }
	
	
	void setValue(const T& _t) { t = _t; isSet_ = true; }

	/**
	 Sets value by ensuring that only sets isSet once value is written, in case being used in some
	 kind of SRSW lockfree collection that relies on this behaviour (i.e. ZLockFreeSet)
	 */
	void setValueTS(const T& _t) {
		t = _t;
		TFullMemoryBarrier();
		isSet_ = true;
	}

};

class NullableBool {
	int b = -1;
public:
	NullableBool& operator=(bool _b) { b = (_b ? 1 : 0); return *this; }
	
	void reset() { unset(); }
	void unset() { b = -1; }
	bool isSet()const { return (b != -1); }
	bool getValue()const { return b==1; }
	void setValue(bool _b) { b = (_b ? 1 : 0); }
	
	//
	// atomic functions ...
	//
	
	// returns whether set, and if set, sets the result in result
	bool isSetAndGet(bool& result)const {
		int bT = b;
		
		if(bT != -1) {
			result = (bT==1);
		}
		return bT != -1;
	}
	
	// returns true on success
	bool atomicUnsetIfEq(bool oldValue) {
		return TCompareAndSwap(oldValue?1:0, -1, &b);
	}
	bool atomicSetIfUnset(bool newValue) {
		return TCompareAndSwap(-1, newValue?1:0, &b);
	}
};
#endif

#endif /* defined(__Nullable__) */
