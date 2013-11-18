#ifndef _MemoryWrapper_h
#define _MemoryWrapper_h

#include "TCommon.h"

/**
 Include this to get SharedPtr, ScopedPtr
 Note: the TR1 implementation only builds if have RTTI enabled ... and tr1 doesn't have make_shared (so haven't been using)
 */
#if USING_TR1
#	include <tr1/memory>
#	define SharedPtr std::tr1::shared_ptr
#	define UniquePtr std::tr1::unique_ptr
#else
#	include <memory>
#	define SharedPtr std::shared_ptr
#	define UniquePtr std::unique_ptr
#endif

/**
 Works like UniquePtr, but allows defining custom delete function with a typedef without
 then requiring an object of the custom delete function class to be passed to every
 constructor as UniquePtr does, rather just requires the delete function class to define
 static void free(T* ptr), i.e.:

	struct FileDel {
		static void free()(FILE* p) { fclose(fp); }
	};
	typedef TCustomDelPtr<FILE, FileDel> FilePtr;
 
	void fn() {
		FilePtr fp(fopen(...);
 
		// use fp knowing it will be closed properly ...
	}
 */
template<class T, class DelFn>
class TCustomDelPtr {
private:
	T* ptr = nullptr;
public:
	TCustomDelPtr() {}
	TCustomDelPtr(T* _ptr) { ptr = _ptr; }
	~TCustomDelPtr() { reset(); }
	
	void reset(T* _ptr = nullptr) {
		if(ptr) {
			DelFn::free(ptr);
		}
		ptr = _ptr;
	}
	
	T* operator->() { return ptr; }
	operator bool() const { return ptr != nullptr; }
	bool operator!()const { return ptr == nullptr; }
	
	T* get() { return ptr; }
	
	void swap(TCustomDelPtr& rhs) {
		T* t = rhs.ptr;
		rhs.ptr = ptr;
		ptr = t;
	}
	
	//
	// disable copy semantics (as with unique_ptr)
	//
	DISALLOW_COPY_AND_ASSIGN(TCustomDelPtr);
	
	//
	// enable move semantics (as with unique_ptr)
	//
	TCustomDelPtr(TCustomDelPtr&& rhs):ptr(rhs.ptr) {
		rhs.ptr = nullptr;
	}
	void operator=(TCustomDelPtr&& rhs) {
		if(ptr) {
			reset();
		}
		ptr = rhs.ptr;
		rhs.ptr = nullptr;
	}
};

#endif
