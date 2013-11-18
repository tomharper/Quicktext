#ifndef _TEndian_Utils_h
#define _TEndian_Utils_h

#include "TCommon.h"

class TEndianUtils {
public:
	static unsigned short endian_swap(const unsigned short& x) { return (x>>8) | (x<<8); }
	static int16_t endian_swap(const int16_t& x) { return (x << 8 & 0xFF00) | (x >> 8 & 0x00FF); }
	static unsigned int endian_swap(const unsigned int& x) {
		return (x>>24) | ((x<<8) & 0x00FF0000) | ((x>>8) & 0x0000FF00) | (x<<24);
	}
	static int endian_swap(int val) {
		val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF );
		return (val << 16) | ((val >> 16) & 0xFFFF);
	}
	
	static int readLittleEndianInt16(FILE* fp) {
		int16_t result;
		fread(&result, 2, 1, fp);
		
#ifdef WEBRTC_BIG_ENDIAN
		result = endian_swap(result);
#endif
		return result;
	}
	static int readLittleEndianInt32(FILE* fp) {
		int result;
		fread(&result, 4, 1, fp);
		
#ifdef WEBRTC_BIG_ENDIAN
		result = endian_swap(result);
#endif
		return result;
	}
		
	template<class T> static void writeBigEndian(FILE* fp, T val) {
#ifndef WEBRTC_BIG_ENDIAN
		val = endian_swap(val);
#endif
		fwrite(&val, sizeof(T), 1, fp);
	}
	template<class T> static void writeLittleEndian(FILE* fp, T val) {
#ifdef WEBRTC_BIG_ENDIAN
		val = endian_swap(val);
#endif
		fwrite(&val, 1, sizeof(T), fp);
	}
};

template<class T>
class EndianSwapInt {
	T val = 0;
public:
	EndianSwapInt() {}
	EndianSwapInt(T& _val):val(endian_swap(_val)) {}
	
	operator T() const { return endian_swap(val); }
};
/*
#ifdef WEBRTC_BIG_ENDIAN
typedef short                   BigEndianInt16;
typedef uint16_t                BigEndianUInt16;
typedef uint32_t                BigEndianUInt32;
typedef EndianSwapInt<short>    LittleEndianInt16;
typedef EndianSwapInt<uint16_t> LittleEndianUInt16;
typedef EndianSwapInt<uint32_t> LittleEndianUInt32;
#else
typedef short                   LittleEndianInt16;
typedef uint16_t                LittleEndianUInt16;
typedef uint32_t                LittleEndianUInt32;
typedef EndianSwapInt<short>    BigEndianInt16;
typedef EndianSwapInt<uint16_t> BigEndianUInt16;
typedef EndianSwapInt<uint32_t> BigEndianUInt32;
#endif
*/
#endif
