#ifndef _TFile_h
#define _TFile_h

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <string>
#include <map>
#include <vector>
#include "TEndianUtils.h"
#include "MemoryWrapper.h"

using namespace std;

class TDirectory {
public:
	
	/**
	 Returns:
	 - 0 - success
	 - else, something system specific (so hard to tell whether non-existing or failure?)
	 */
	static int deleteFile(const char* filename);
	static int deleteFile(const std::string& filename);
	
	enum DirCreateResult { DCR_SUCCESS, DCR_DIR_ALREADY_EXISTS, DCR_SOMETHING_BLOCKING, DCR_UNEXPECTED_ERROR };
	static DirCreateResult mkDirWithChecks(const char* path, bool loggingEnabled=true);
	
	enum FileType { FT_NONE, FT_FILE, FT_DIR, FT_LINK, FT_SOCK, FT_OTHER, FT_UNKNOWN_ERROR };
	static FileType checkIfFileOrDirExists(const char* path);

	/**
	 Returns 0 on success, else error-code on failure. Prefer to use mkDirWithChecks(), which checks if
	 something exists first.
	 */
	static int mkDir(const char* path);
	static int mkDir(const std::string& path);
};


/**
 These classes are mainly here to just wrap FILE* with RAII. They also provide endian conversion.
 */
class TFileCommon {
protected:
	FILE* fp = nullptr;
	
	void checkForError();
	
public:
	TFileCommon() {} //= default;
	
	void flush() { fflush(fp); }

	// move constructor
	TFileCommon(TFileCommon&& other):fp(other.fp) { other.fp = nullptr; }
	
	// disable copy constructor + operator=
	DISALLOW_COPY_AND_ASSIGN(TFileCommon); //TFileCommon(const TFileCommon& other) = delete;
	
	virtual ~TFileCommon() {}
};

class Vocab {
public:
	Vocab() : p(0), pint(0), cnt(0) { }
	Vocab(char* in, int count) : p(0), pint(0) { d=in; cnt=count; }
	~Vocab() {}
	void cloneFrom(Vocab &in)  { d = in.d; cnt = in.cnt; p= in.p; pint= in.pint; }
	std::string d;
	int cnt;
	float p;
	int64_t pint;
};
typedef SharedPtr<Vocab> VocabPtr;

class TFileReader : public TFileCommon {
	int filesize = -1;
	
public:
	
	static bool fileExists(const std::string& filename);
	static int fileSize(const std::string& filename);
	
	static unsigned char* readWholeFileToBuffer(const std::string& filename);
	
	int fileSize();
	
	virtual int size();
	virtual int tell() { return ftell(fp); }
	virtual void seekFromBegin(int offset=0);
	virtual void seekFromEnd(int offset=0);
	virtual bool reachedEnd();
	
	TFileReader() {}
	TFileReader(const char* filename, const char* flags="rb") { open(filename, flags); }
	TFileReader(std::string filename, const char* flags="rb") { open(filename.c_str(), flags); }
	
	// if have this, then should disable copy-constructor!! (or define some move semantics ...)
	~TFileReader();
	
	bool isOpen();
	
	void open(const char* filename, const char* flags="rb");
	
	size_t read(void* data, int size);
    map<string, Vocab> readVocab();
	
	
	int readLittleEndianInt16();
	int readLittleEndianInt32();
	
	// reads a string stored to binary file as size + string (with no trailing 0)
	std::string readString();
	
	/**
	 For small strings - reads 'bytes' bytes, and asserts if doesn't match the given string
	 Only used for wav-file-headers - note that this is incompatible with the readString()/
	 writeString() method of storing size+string - this is assuming that you already know the 
	 string size
	*/
	void readAndCheckMatchesString(const char* s, int bytes);
};
typedef SharedPtr<TFileReader> FileReaderPtr;


class TFileWriter : public TFileCommon {
public:
	TFileWriter() {}
	TFileWriter(const char* filename, const char* flags="wb") { open(filename, flags); }
	TFileWriter(std::string filename, const char* flags="wb") { open(filename.c_str(), flags); }
	
	// if have this, then should disable copy-constructor!! (or define some move semantics ...)
	~TFileWriter();
	
	void open(const char* filename, const char* flags="wb");
	
	size_t write(const void* data, int size);
	
    /* FIXME
	template<class T>
	size_t write(const TBuffer<T>& v) {
		auto result = fwrite(&v[0], sizeof(T), v.size(), fp);
		checkForError();
		return result;
	}
     */
	
	long tell() { return ftell(fp); }
	
	// writes a string to binary file as size + string (with no trailing 0)
	void writeString(const std::string& s);
	
	void seekFromBegin(int offset=0);
	void seekFromEnd(int offset=0);
	
	template<class T> void writeLittleEndian(T t) { TEndianUtils::writeLittleEndian(fp, t); }
	template<class T> void writeLittleEndianInt16(T t) { TEndianUtils::writeLittleEndian(fp, (short)t); }
	template<class T> void writeLittleEndianInt32(T t) { TEndianUtils::writeLittleEndian(fp, (int)t); }
};
typedef SharedPtr<TFileWriter> FileWriterPtr;

#endif
