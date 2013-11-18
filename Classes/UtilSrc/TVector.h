#ifndef _TVector_h
#define _TVector_h

#ifdef __cplusplus
#include <vector>
#include "MemoryWrapper.h"

/**
 Override std::vector, so can:
 - make it non-copyable (remains moveable), so ensuring never accidentally copying entire vectors
 - ...
 */
template<class T>
class TVector : public std::vector<T> {
public:
	TVector<T>():std::vector<T>() {}
	TVector(std::initializer_list<T> l):std::vector<T>(l) {}
	
	/**
	 O(n): returns true if found+erased one element (stops on first match)
	 */
	bool eraseMatchingElement(const SharedPtr<T>& v) {
		for(unsigned long i=0; i<this->size(); i++) {
			auto& cur = (*this)[i];
			if(cur == v) {
				this->erase(this->begin() + i);
				return true;
			}
		}
		return false;
	}
	
	/**
	 Allow user to force explicit cloning when actually needed:
	 a.clone(b)
	 is like a = b, but with b being copied.
	 */
	void cloneFrom(const TVector<T>& v) {
		*this = v;
	}
	TVector<T> clone()const {
		TVector<T> copyOfMe(*this);
		return copyOfMe;
	}

	// enable moving
	TVector(TVector&& v) = default; //:std::vector<T>(v) {}
	TVector& operator=(TVector&& rhs) = default;
	
//private:
	
   // disable copying (or not!?)
	TVector(const TVector&) = default;
	TVector& operator=(const TVector&) = default;
};

#endif

#endif
