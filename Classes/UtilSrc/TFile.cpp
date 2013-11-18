#include "TFile.h"
#include "TLogging.h"
#include "TReaderI.h"
#include <sys/stat.h> // for mkdir

#include <vector>
#include <map>

using namespace std;

#pragma mark - Directory

int TDirectory::deleteFile(const char* filename) { return remove(filename); }
int TDirectory::deleteFile(const std::string& filename) { return deleteFile(filename.c_str());	}

int TDirectory::mkDir(const char* path) { return mkdir(path, 0755); }
int TDirectory::mkDir(const std::string& path) { return mkDir(path.c_str()); }

TDirectory::DirCreateResult TDirectory::mkDirWithChecks(const char* path, bool loggingEnabled) {
	FileType ft = checkIfFileOrDirExists(path);
	if(ft == FT_DIR) {
		return DCR_DIR_ALREADY_EXISTS;
	}
	else if(ft != FT_NONE) {
		TLogError("Can't create dir for path '%s' as already exists a file there of type: %d", path, ft);
		return DCR_SOMETHING_BLOCKING;
	}
	else {
		int result = mkDir(path);
		if(loggingEnabled) {
		}
		if(result != 0) {
			return DCR_UNEXPECTED_ERROR;
		}
		return DCR_SUCCESS;
	}
}

TDirectory::FileType TDirectory::checkIfFileOrDirExists(const char* path) {
	struct stat st;
	int res = ::stat(path, &st);
	if (res == 0) {
		// Something exists at this location, check if it is a directory
		if(S_ISDIR(st.st_mode)) {
			return FT_DIR;
		}
		else if(S_ISREG(st.st_mode)) {
			return FT_FILE;
		}
		else if(S_ISLNK(st.st_mode)) {
			return FT_LINK;
		}
		else if(S_ISSOCK(st.st_mode)) {
			return FT_SOCK;
		}
		else {
			return FT_OTHER;
		}
	}
	else {
		if(errno == ENOENT) {
			return FT_NONE;
		}
		else {
			return FT_UNKNOWN_ERROR;
		}
	}
}


#pragma mark - FileCommon

void TFileCommon::checkForError() {
#ifndef NDEBUG
	if(fp) {
		int err = ferror(fp);
		if(err) {
			char buf[256];
			perror(buf);
		}
	}
#endif
}


#pragma mark - FileReader

bool TFileReader::fileExists(const std::string& filename) {
	if (FILE* fp = fopen(filename.c_str(), "r")) {
		fclose(fp);
		return true;
	}
	return false;
}

int TFileReader::fileSize(const std::string& filename) {
	if (FILE* fp = fopen(filename.c_str(), "r")) {
		fseek(fp, 0, SEEK_END);
		long len = ftell(fp);
		
		fclose(fp);
		return len;
	}
	return -1;
}

int TFileReader::fileSize() {
	if(filesize == -1) {
		int beginPoint = tell();
		seekFromEnd();
		filesize = tell();
		seekFromBegin(beginPoint);
	}
	return filesize;
}

int TFileReader::size() { return fileSize(); }

void TFileReader::seekFromBegin(int offset) { fseek(fp, offset, SEEK_SET); }
void TFileReader::seekFromEnd(int offset) { fseek(fp, offset, SEEK_END); }

bool TFileReader::reachedEnd() {
	int fs  = fileSize();
	int pos = tell();
	
	if(pos >= fs) {
		return true;
	}
	return false;
}

TFileReader::~TFileReader() {
	if(fp) {
		fclose(fp);
	}
}

bool TFileReader::isOpen() { return fp != NULL; }

void TFileReader::open(const char* filename, const char* flags) {
	fp = fopen(filename, flags);
	checkForError();
}

size_t TFileReader::read(void* data, int size) {
	auto result = fread(data, 1, size, fp);
	checkForError();
	
	return result;
}


#define MAX_LINE       100
map<string, Vocab> TFileReader::readVocab()
{
	char line[MAX_LINE];
	const char* whiteSpace = " \t\n\v\f\r";
	std::vector<Vocab> word_data;
    std::map<std::string, Vocab> mymap;
	
	if (!fp)
		return mymap;
	
	int64_t nCountTot = 0;
	while (fgets(line, MAX_LINE, fp) != NULL)
	{
		char* ptr = line;
		char* thisword;
		char* thisnumber;
		int i = 0;
				
		//skip white space at the start of a line
		while (isspace(*ptr))
			++ptr;
		if (*ptr == '\0') // new line
			continue;
		if ((thisword = strtok(ptr, whiteSpace)) == NULL)
		{
			// should never happen
		}
		if ((thisnumber = strtok(NULL, whiteSpace)) == NULL || (i = atoi(thisnumber)) < 1)
		{
			// should never happen
		}
		auto myptr = Vocab(thisword,i);
		word_data.push_back(myptr);
		nCountTot += i;
	}
	for (std::vector<Vocab>::iterator it = word_data.begin(); it != word_data.end(); ++it) {
		it->pint = (((int64_t)it->cnt)*10000000)/nCountTot; // this is enough precision for the small lists
		it->p = (float) (double)it->pint/10000000.0;

		mymap[it->d].cloneFrom( *it );
	}

	return mymap;
}

int TFileReader::readLittleEndianInt16() {
	return TEndianUtils::readLittleEndianInt16(fp);
}
int TFileReader::readLittleEndianInt32() {
	return TEndianUtils::readLittleEndianInt32(fp);
}

// reads a string stored to binary file as size + string (with no trailing 0)
std::string TFileReader::readString() {
	
	int stringSize = readLittleEndianInt32();
	
	if(stringSize <= 0)
		return "";
		
	std::string result(stringSize, ' ');
	read(&result[0], stringSize);
	return result;
}

/**
 For small strings - reads 'bytes' bytes, and asserts if doesn't match the given string
 Only used for wav-file-headers - note that this is incompatible with the readString()/
 writeString() method of storing size+string - this is assuming that you already know the
 string size
 */
void TFileReader::readAndCheckMatchesString(const char* s, int bytes) {
	char buf[16];
	read(&buf[0], bytes);
	buf[bytes] = 0;
}


unsigned char* TFileReader::readWholeFileToBuffer(const std::string& filename) {
	TFileReader fr(filename);
	return fr.readWholeFileToBuffer(filename);
}


#pragma mark - FileWriter

TFileWriter::~TFileWriter() {
	if(fp) {
		fclose(fp);
	}
}

void TFileWriter::open(const char* filename, const char* flags) {
	fp = fopen(filename, flags);
	if (fp == NULL) {
		TLogError("fopen failed: %d(%s)", errno, strerror(errno));
	}
	checkForError();
}

size_t TFileWriter::write(const void* data, int size) {
	auto result = fwrite(data, 1, size, fp);
	checkForError();
	
	return result;
}
/*
void TFileWriter::writeReader(TReaderI* reader) {
	reader->seekFromBegin();
	int size = reader->size();
	char buf[65536];
	int pos = 0;
	while(pos < size) {
		int chunkSize = TMin(size-pos, 65536);
		int bytesRead = reader->read(&buf[0], chunkSize);
        if (bytesRead)
            write(&buf[0], chunkSize);
		pos += chunkSize;
	}
	reader->seekFromBegin();
}
 */

// writes a string to binary file as size + string (with no trailing 0)
void TFileWriter::writeString(const std::string& s) {
	writeLittleEndianInt32(s.size());
	if(s.size()) {
		write(&s[0], s.size());
	}
}

void TFileWriter::seekFromBegin(int offset) { fseek(fp, offset, SEEK_SET); }
void TFileWriter::seekFromEnd(int offset) { fseek(fp, offset, SEEK_END); }
