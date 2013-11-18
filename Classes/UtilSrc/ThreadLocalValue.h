#ifndef _TThreadLocalValue_h
#define _TThreadLocalValue_h

#include "TCommon.h"
#include <pthread.h>


/**
 clang doesn't support __thread for the architectures that need - this could be implemented simply using __thread
 on such platforms.
 
 also tried using a map from thread-id to data, and to fetch a value querying Thread::getCurrentThreadId() and then
 looking it up in the map, but even lacking proper thread-safety on the map, it was still way slower than this way
 of using pthread_setspecific, .... as Thread::getCurrentThreadId() seems not the cheapest thing to be doing too
 frequently?
*/

struct ValueHolderI {
	void* value;
	pthread_key_t tlsKey;
	
	virtual void deleteValue()=0;
	virtual ~ValueHolderI() {}
};

extern void globalDestructorValue(void *value);

template<class T>
class ThreadLocalValue_AutoDelete {
public:
	
	//
	// using inheritence to get around being unable to pass/convert a member function to a plain
	// C-style function (as then could just define a destructor member function and pass that to
	// pthread_key_create ...
	//
	struct MyValueHolder : public ValueHolderI {
		void deleteValue()override {
			T* t = (T*)value;
			delete t;
		}
	};
	
	ThreadLocalValue_AutoDelete() {
		//
		// seems this would be possible if enabled RTTI ... but don't have it enabled currently ...
		//
		//typedef void (*FnT)(void*);
		//function<void(void*)> fn = bind(&ThreadLocalValue::globalDestructor, this, _1);
		//auto closure = [this] ( void* p ) { globalDestructor(p); };
		//FnT fnC = closure;
		
		pthread_key_create(&tlsKey, globalDestructorValue); //fn.target<FnT>());
	}
	T* getValue() {
		ValueHolderI* vh = (ValueHolderI*) pthread_getspecific(tlsKey);
		if(!vh) {
			return NULL;
		}
		else {
			return (T*) vh->value;
		}
	}
	void setValue(T* value) {
		ValueHolderI* vh = new MyValueHolder();
		vh->value = value;
		vh->tlsKey = tlsKey;
		pthread_setspecific(tlsKey, vh); //value);
	}
	
public:
	pthread_key_t tlsKey = 0;
};

template<class T>
class ThreadLocalValue {
public:
	ThreadLocalValue() {
		pthread_key_create(&tlsKey, NULL);
	}
	T getValue() {
		void* data = pthread_getspecific(tlsKey);
		return (T)data; // this is how we do it ...
	}
	void setValue(T value) {
		pthread_setspecific(tlsKey, (void*)value);
	}
	
protected:
	pthread_key_t tlsKey = 0;
};


#endif
