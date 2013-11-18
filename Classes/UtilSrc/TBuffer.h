#ifndef _TBuffer_h
#define _TBuffer_h


#include "TCommon.h"
#include "TAtomic.h"

/**
 For use instead of vector/plain-pointer when want to pass arrays of data around. Ensures no copying
 that tends to be associated with vector, and makes memory management safer than plain pointer (plus
 unlike plain pointer, notes the size).
 
 Asides from preventing copying  unlike std::vector:
 - doesn't bother to init memory when constructing/resizing!
 - doesn't both to keep existing contents when resizing!
 - generally not made for using like vector - push_back etc - so if try to push_back beyond 
   allocated (i.e. with reserve()) size, will simply assert+fail - no automatic re-allocing of memory.
 - allows user to take ownership of the memory (and thus responsibility for deleting), using yield().
 */
template<class T>
class TBuffer {
	T* arr = nullptr;
	int allocSize = -1;
	int count = 0;
	
public:
	
	~TBuffer() {
		if(arr) {
			delete arr;
		}
	}
	TBuffer() {}
	
	/// Note: doesn't init allocated memory
	TBuffer(int size):arr(new T[size]),allocSize(size),count(size) { }
	
	TBuffer(T* buf, int size) {
		initFromBuf(buf, size);
	}
	
	void initFromBuf(T* buf, int size) {
		if(arr) {
			delete arr;
		}

		arr = new T[size];
		allocSize = size;
		count = size;
		memcpy(arr, buf, size*sizeof(T));
	}

	// copies buf ...
	void initFromBuf(const TBuffer& buf) {
		initFromBuf(&buf[0], buf.size());
	}
	
	/// Note: destroys any existing buffer + associated data, and doesn't init memory
	void resize(int size) {
		if(arr) {
			delete arr;
		}
		arr = new T[size];
		allocSize = size;
		count = size;
	}
	
	void reserve(int size) {
		if(arr) {
			delete arr;
		}
		arr = new T[size];
		allocSize = size;
		count = 0;
	}
	
	void clear() {
		count = 0;
	}
	
	void push_back(const T& t) {
		arr[count] = t;
		TFullMemoryBarrier(); // for SRSW use
		count++;
	}
	void push_back(T&& t) {
		arr[count] = std::move(t);
		TFullMemoryBarrier(); // for SRSW use
		count++;
	}
	
	void append(const T* data, int dataCount) {
		if(count+dataCount <= allocSize) {
			memcpy(&arr[count], data, dataCount*sizeof(T));
			count += dataCount;
		}
	}

	int size()const { return count; }
	
	const T* data()const { return arr; }
	T* data() { return arr; }

	T* yield() {
		T* result = arr;
		arr = nullptr;
		allocSize = -1;
		count = 0;
		return result;
	}
	
#ifdef _DEBUG
	const T& operator[] (int i)const {
		return data()[i];
	}
	T& operator[] (int i) 	{
		return data()[i];
	}
#else
	operator const T* ()const { return arr; }
	operator T* () { return arr; }
#endif
	
	// enable moving
	TBuffer(TBuffer&& v) {
		if(arr) {
			delete arr;
		}
		arr = v.arr;
		allocSize = v.allocSize;
		count = v.count;
		
		v.arr = nullptr;
		v.allocSize = -1;
		v.count = 0;
	}
	TBuffer& operator=(TBuffer&& v) {
		if(arr) {
			delete arr;
		}
		arr = v.arr;
		allocSize = v.allocSize;
		count = v.count;
		
		v.arr = nullptr;
		v.allocSize = -1;
		v.count = 0;

		return *this;
	}

	//super-simple+dumb hash-fn ....
	int hash() {
		int result = 0;
		for(int i=0; i<count; i++) {
			result = ((result<<5)-result)+arr[i];
		}
		return result;
	}
	
private:
	DISALLOW_COPY_AND_ASSIGN(TBuffer);
};

typedef TBuffer<uint8_t> TBufU8;

#endif
