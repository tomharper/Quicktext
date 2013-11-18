#ifndef _TReaderI_h
#define _TReaderI_h

#ifdef ANDROID
#include <limits.h> // Android needs this for INT_MAX
#endif
#include "TCommon.h"
#include "TBuffer.h"
#include "MemoryWrapper.h"
#include <string>

class ReaderI {
public:

	//
	// functions sub-classes need to implement
	//
	virtual size_t read(void* data, int size)=0;
	virtual bool reachedEnd()=0;
	virtual ~ReaderI() {}
	
	//
	// more functions sub-classes need to implement
	// - maybe shouldn't need to rely on implementors having to implement these
	//   (shouldn't need, and limits use) ... but, seems curl needs the seekFromBegin()
	//   + seekFromEnd() at least for some reason, and generally all of these do aid
	//   usage when available.
	//
	virtual int size()=0;
	virtual int tell()=0;
	virtual void seekFromBegin(int offset=0)=0;
	virtual void seekFromEnd(int offset=0)=0;

	
	//
	// common utility functions ...
	//
	unsigned char* readRestToBuffer() {
		int const nBytesRemaining = bytesRemaining();
        // FIXME:nBytesRemaining
        //TBufU8 result(nBytesRemaining);
        unsigned char* result = new unsigned char[nBytesRemaining];
		size_t bytesRead = read(&result[0], nBytesRemaining);
		return result;
	}
	
	unsigned char* readWholeStreamToBuffer() {
		seekFromBegin();
		return readRestToBuffer();
	}
	
	int bytesRemaining() {
		return size()-tell();
	}
};
typedef SharedPtr<ReaderI> ReaderIPtr;

class BufferReader : public ReaderI {
	const uint8_t* buf = nullptr;
	int bufSize = 0;
	int pos = 0;
	bool freeBuf = false;
public:
	BufferReader() {}
	BufferReader(const uint8_t* _data, int _dataSize, bool _freeBuf=false) {
		init(_data, _dataSize, _freeBuf);
	}
	
	/**
	 @param bufferShouldYield
	  - if true, then buffer 'yields' its contents, i.e. it will be empty after this call
	  - if false, then this will just point to buffer's data, and so buffer will have to hang around!
	 */
	/*
	template<class T>
	BufferReader(TBuffer<T>& buffer, bool bufferShouldYield=true) {
		bufSize = buffer.size();
		if(bufferShouldYield) {
			buf = (uint8_t*) buffer.yield();
		}
		else {
			buf = &buffer[0];
		}
		freeBuf = bufferShouldYield;
	}
	*/
	
	void init(const uint8_t* _data, int _dataSize, bool _freeBuf=false) {
		buf = _data;
		bufSize = _dataSize;
		freeBuf = _freeBuf;
	}
	
	virtual int size()override {
		return bufSize;
	}
	virtual int tell()override {
		return pos;
	}
	
	virtual size_t read(void* data, int size)override {
		int nBytesRemaining = bufSize-pos;
		
		if(nBytesRemaining == 0) {
			return 0;
		}
		else {
			int nBytesToRead = min(size, nBytesRemaining);
			memcpy(data, &buf[pos], nBytesToRead);
			pos += nBytesToRead;
			return nBytesToRead;
		}
	}
	
	virtual bool reachedEnd()override {
		return pos == bufSize;
	}
	
	virtual void seekFromBegin(int offset=0)override {
		pos = offset;
	}
	virtual void seekFromEnd(int offset=0)override {
		pos = bufSize-offset;
	}
	
	
	~BufferReader() {
		if(freeBuf && buf) {
			delete buf;
		}
	}
};

class StringReader : public BufferReader {
	std::string str;
public:
	StringReader(const std::string& data):str(data) {
		init((uint8_t*) &str[0], (int)str.size());
	}
};

#endif
