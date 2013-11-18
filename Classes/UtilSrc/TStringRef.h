#ifndef __TStringRef__
#define __TStringRef__

#include "TCommon.h"
#include "TVector.h"
#include <iosfwd>

/**
 Represents a constant reference to a string, i.e. a character array and a length, which need
 not be null terminated.
 
 This class does not own the string data, it is expected to be used in situations where the
 character data resides in some other buffer, whose lifetime extends past that of the StringRef. 
 For this reason, it is not in general safe to store a StringRef.
 */
#include <string.h>
#include <string>

class TStringRef {
public:
	const char* data = nullptr;
	int length = 0;
	
	static const int npos = -1;
	
	TStringRef() {}
    TStringRef(const char* str):data(str),length(strlen(str)) {}
	TStringRef(const char* str, int _length):data(str), length(_length) {}
    
    const char& operator[](int i)const {
        return data[i];
    }
    
    std::string str()const { return std::string(data, length); }
    
    
	template<class TStringT>
	static TVector<TStringRef> tokenize(const TStringT& str, char separator) {
		TVector<TStringRef> result;
		int substrStartPos=0;
		for(int i=0; i<(int)str.size(); i++) {
			char c = str[i];
			if(c == separator) {
				if(i > substrStartPos) {
					result.push_back({ &str[substrStartPos], i-substrStartPos });
				}
				substrStartPos = i+1;
			}
		}
		if((int)str.size() > substrStartPos) {
			result.push_back({ &str[substrStartPos], str.size()-substrStartPos });
		}
		return result;
	}
		
	/**
	 Find functions return -1 on failure. They can take either TStringRef or std::string as parameter.
	 */
	template<class StringT>
	int find(const StringT& str)const {
		ZAssert(str.size());
		int lastPos = length-str.size();
		for(int i=0; i<=lastPos; i++) {
			bool matches = true;
			for(int j=0; j<(int)str.size(); j++) {
				if(str[j] != data[i+j]) {
					matches = false;
					break;
				}
			}
			if(matches) {
				return i;
			}
		}
		return -1;
	}
	
	template<class StringT>
	int rfind(const StringT& str)const {
		int lastPos = length-str.size();
		for(int i=lastPos; i>=0; i--) {
			bool matches = true;
			for(int j=0; j<(int)str.size(); j++) {
				if(str[j] != data[i+j]) {
					matches = false;
					break;
				}
			}
			if(matches) {
				return i;
			}
		}
		return -1;
	}
	
	int size()const {
		return length;
	}
	
	TStringRef substr(int start, int end=-1) {
		if(end == -1) {
			end = length;
		}
		return TStringRef(&data[start], end-start);
	}
	
	const char* c_str()const { return data; }
};

#endif /* defined(__TStringRef__) */
