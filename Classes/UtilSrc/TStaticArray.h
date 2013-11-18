#ifndef _TStaticArray_h
#define _TStaticArray_h

#include "TCommon.h"
#include "TAtomic.h"
#include <initializer_list>
#include <utility> // std::move

/**
 Acts like a std::vector, in that stores a count of elements, however stores the data
 like a plain C array - on the stack with a fixed size limit.
 - this means can use in limited lockfree scenarios 
 - no realloc
   - this works for the simple case of one thread adding, and others iterating/reading
   - (push_back ensures elements contents are filled before the vector is extended)
 */
template<class T, int ARR_SIZE>
class TStaticArray {
	T arr[ARR_SIZE];
	int count = 0;
	
	class ConstIterator {
		const T* p;
	public:
		ConstIterator(const T* _p ) : p(_p) { }
		const T& operator* () const                     { return *p; }
		const ConstIterator& operator ++ ()             { p++; return *this; }
		bool operator != (const ConstIterator& i) const { return p != i.p; }
	};
	
	class Iterator {
		T* p;
	public:
		Iterator(T* _p ) : p(_p) { }
		T& operator* ()                            { return *p; }
		Iterator& operator ++ ()                   { p++; return *this; }
		bool operator != (const Iterator& i) const { return p != i.p; }
	};
	
public:
	
	TStaticArray() {}
	TStaticArray(std::initializer_list<T> values) {
		for(const T& v: values) {
			push_back(v);
		}
	}
	
	void push_back(const T& t) {
		arr[count] = t;

		// so that this array could safely be read in the SRSW case of one thread adding items using
		// push_back, and another reading items
		TFullMemoryBarrier();

		count++;
	}
	void push_back(T&& t) {
		arr[count] = std::move(t);
		
		// so that this array could safely be read in the SRSW case of one thread adding items using
		// push_back, and another reading items
		TFullMemoryBarrier();
		
		count++;
	}
	T& pop_back() {
		return arr[--count];
	}
	
	T* extend() {
		count++;
		return &arr[count-1];
	}
	
	T& first() { return arr[0]; }
	T& last()  { return arr[count-1]; }

	T& front() { return first(); }
	T& back()  { return last(); }
	
	/// Warning: o(n)!
	void erase(int index) {
		for(int i=index; i<count-1; i++) {
			arr[i] = arr[i+1];
		}
		if(count > 0 && index < count) {
			count--;
		}
	}
	
	void clear() { count = 0; }
	void resize(int newSize) {
		count = newSize;
	}
	
	int size()const { return count; }
	
#ifdef _DEBUG
	const T& operator[] (int i)const {
		return arr[i];
	}
	T& operator[] (int i) 	{
		return arr[i];
	}
#else
	operator const T* ()const { return &arr[0]; }
	operator T* () { return &arr[0]; }
#endif
	
	ConstIterator begin()const { return { &arr[0] }; }
	ConstIterator end()  const { return { &arr[count-1] + 1 }; }
	
	Iterator begin() { return { &arr[0] }; }
	Iterator end()   { return { &arr[count-1] + 1 }; }
};


/**
 Multiple writer threads can add elements to this while at the same time multiple reader threads can
 iterate+read-from it ....
 */
template<class T, int ARR_SIZE>
class TStaticArrayMRMW {

	T arr[ARR_SIZE];
	
	// not sure if the 'volatile' here is neccessary ....?
	volatile bool arrWrittenTo[ARR_SIZE];
	
	int lastWritePos = -1;
	friend class ConstIterator;
	
	/**
	 index of -1 is end(), and an iterator has reached the 'end' when arrWrittenTo[index] is false
	 */
	class ConstIterator {
		int index=-1;
		const TStaticArrayMRMW* parent;

	public:
		ConstIterator(int _index, const TStaticArrayMRMW* _parent):index(_index),parent(_parent) { }
		const T& operator* () const                     { return (*parent)[index]; }
		const ConstIterator& operator ++ ()             { index++; return *this; }
		bool operator != (const ConstIterator& i) const {
			if(index == -1 && i.index == -1) {
				return false;
			}
			if(index == -1) {
				return parent->arrWrittenTo[i.index];
			}
			if(i.index == -1) {
				return i.parent->arrWrittenTo[index];
			}
			return index != i.index;
		}
	};
	
	int getNextWritePos() {
		int writePos = TAtomicIncrement(&lastWritePos);
		return writePos;
	}
	
public:
	
	TStaticArrayMRMW() {
		for(int i=0; i<ARR_SIZE; i++) { arrWrittenTo[i] = false; }
	}
	TStaticArrayMRMW(std::initializer_list<T> values) {
		for(int i=0; i<ARR_SIZE; i++) { arrWrittenTo[i] = false; }

		for(const T& v: values) {
			push_back(v);
		}
	}
	
	void push_back(const T& t) {
		int writePos = getNextWritePos();
		
		arr[writePos] = t;
		
		TFullMemoryBarrier();
		arrWrittenTo[writePos] = true;
	}
	void push_back(T&& t) {
		int writePos = getNextWritePos();

		arr[writePos] = std::move(t);
		
		TFullMemoryBarrier();
		arrWrittenTo[writePos] = true;
	}
	
	
	T& first() { return arr[0]; }
	T& front() { return first(); }
	
	bool haveValueAtPos(int i) {
		if(i >= ARR_SIZE) {
			return false;
		}
		return arrWrittenTo[i];
	}
	
#ifdef _DEBUG
	const T& operator[] (int i)const {
		return arr[i];
	}
	T& operator[] (int i) 	{
		return arr[i];
	}
#else
	operator const T* ()const { return &arr[0]; }
	operator T* () { return &arr[0]; }
#endif
	
	ConstIterator begin()const { return { 0, this }; }
	ConstIterator end()  const { return { -1,this }; }
};



/**
 Acts like a std::deque, however using stack rather than heap (and thus with a fixed size limit, which
 must be a power of 2!).
 
 Actually is also basically the same as TStaticQueue, except that doesn't provide SRSW threadsafety, 
 and that does provide both front+back access, as well as array access
 */
template<class T, int DQ_SIZE>
class TStaticDeque {
#	define DQ_MASK ((DQ_SIZE)-1)

	T arr[DQ_SIZE];
	int beginPos = 0;
	int endPos = 0;
	
	class Iterator {
		TStaticDeque* parent;
		int index;
	public:
		Iterator(TStaticDeque* _parent, int _index) : parent(_parent),index(_index) { }
		T& operator* ()                            { return (*parent)[index]; }
		bool operator != (const Iterator& i) const { return index != i.index; }
		Iterator& operator ++ ()                   { index++; return *this; }
	};
	
	class ConstIterator {
		const TStaticDeque* parent;
		int index;
	public:
		ConstIterator(const TStaticDeque* _parent, int _index) : parent(_parent),index(_index) { }
		const T& operator* () const                     { return (*parent)[index]; }
		bool operator != (const ConstIterator& i) const { return index != i.index; }
		ConstIterator& operator ++ ()                   { index++; return *this; }
	};
	
public:
	TStaticDeque() {}
	TStaticDeque(std::initializer_list<T> values) {
		for(const T& v: values) {
			push_back(v);
		}
	}
	
	
	void push_back(const T& t)  { arr[endPos++ & DQ_MASK] = t; }
	void push_front(const T& t) { arr[--beginPos & DQ_MASK] = t; }

	void push_back(T&& t)  { arr[endPos++ & DQ_MASK] = std::move(t); }
	void push_front(T&& t) { arr[--beginPos & DQ_MASK] = std::move(t); }
	
	T pop_back()  { return arr[--endPos & DQ_MASK]; }
	T pop_front() { return arr[beginPos++ & DQ_MASK]; }
	
	T& front() { return arr[beginPos & DQ_MASK]; }
	T& back()  { return arr[(endPos-1) & DQ_MASK]; }

	T* extend() {
		return &arr[endPos++ & DQ_MASK];
	}
	
	void erase(int index) {
		if(index==0) {
			pop_front();
		}
		else if(index == size()-1) {
			pop_back();
		}
		else {
			for(int i=index; i<size()-1; i++) {
				(*this)[i] = (*this)[i+1];
			}
			endPos--;
		}
	}
	
	void clear() { endPos = 0; beginPos = 0; }
	
	int size()const {
		int s = endPos - beginPos;
		if (s < 0)
			s += (DQ_SIZE);
		
		return s;
	}
	
	const T& operator[] (int i)const {
		return arr[(i+beginPos) & DQ_MASK];
	}
	T& operator[] (int i) 	{
		return arr[(i+beginPos) & DQ_MASK];
	}
	
	Iterator begin() { return { this, 0 }; }
	Iterator end()   { return { this, size() }; }
	ConstIterator begin() const { return { this, 0 }; }
	ConstIterator end()   const { return { this, size() }; }
};


#endif
