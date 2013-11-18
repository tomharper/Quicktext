#ifndef _TMap_h
#define _TMap_h

#include "TCommon.h"
#include <map>

/**
 Override std::map, so can:
 - make it not copyable
 - add bool contains()
 - add bool getValueForKey(key, out value)
 */

template<class KEY, class VALUE>
class TMap : public std::map<KEY, VALUE> {
public:
	TMap<KEY, VALUE>():std::map<KEY, VALUE>() {}
	TMap(std::initializer_list<std::pair<const KEY,VALUE>> l):std::map<KEY, VALUE>(l) {}

	// returns true if found
	bool contains(const KEY& key)const {
		return (std::map<KEY, VALUE>::find(key) != std::map<KEY, VALUE>::end());
	}
	
	// returns true if found
	bool getValueForKey(const KEY& key, VALUE** value) {
		auto it = std::map<KEY, VALUE>::find(key);
		if(it == std::map<KEY, VALUE>::end()) {
			return false;
		}
		value = &it->second;
		return true;
	}

	// returns true if found
	bool getValueForKey(const KEY& key, VALUE& value) {
		auto it = std::map<KEY, VALUE>::find(key);
		if(it == std::map<KEY, VALUE>::end()) {
			return false;
		}
		value = it->second;
		return true;
	}
	
	// enable moving
	TMap(TMap
         && v) = default; //:std::map<KEY, VALUE>(v) {} // move constructor
	TMap& operator=(TMap&& rhs) = default;

	void cloneFrom(const TMap<KEY, VALUE>& m) {
		*this = m;
	}
	
private:
	
   // disable copying
	TMap(const TMap&) = default;
	TMap& operator=(const TMap&) = default;
};

#endif
