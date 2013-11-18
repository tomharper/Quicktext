#include "TUtils.h"
#include "TFile.h"
#include <time.h>
#ifndef ANDROID
#include <sys/sysctl.h>
#endif
#include <string>
#import <sys/stat.h>


using namespace std;
char *TUtils::readFile(const char *name)
{
    struct stat statbuf;
    FILE *fh;
    char *source;
    
    fh = fopen(name, "r");
    if (fh == 0)
        return 0;
    
    stat(name, &statbuf);
    source = (char *) malloc(statbuf.st_size + 1);
    fread(source, statbuf.st_size, 1, fh);
    source[statbuf.st_size] = '\0';
    fclose(fh);
    
    return source;
}


string TUtils::trim(const string& s)
{
        unsigned long length = s.size();
        int firstChar = -1;
        for(unsigned long i = 0; i < length; i++) {
                if(!isspace(s[i])) {
                        firstChar = i;
                        break;
                }
        }
        if(firstChar == -1) {
                return string();
        }

        int lastChar = -1;
        for(int j=length-1; j>firstChar; j--) {
                if(!isspace(s[j])) {
                        lastChar = j;
                        break;
                }
        }
        return string(&s[firstChar], lastChar-firstChar+1);
}

std::string TUtils::toLower(const std::string& s) {
        string result(s);
        for(char&c: result) {
                if(c >= 'A' && c <= 'Z') {
                        c += 'a' - 'A';
                }
        }
        return result;
}

string TUtils::stateFilesSubDir = "";

string TUtils::format(const string& s, ...) {
        return TUtils::format(s.c_str());
}

string TUtils::format(const char* s, ...) {
        va_list ap;
        va_start(ap, s);

        char buf[4096];
        vsnprintf(buf, sizeof(buf) - 1, s, ap);

        va_end(ap);

        return string(buf);
}


bool TUtils::ensureStateFileSubdirExists(const std::string& dir, bool loggingEnabled) {
	auto result = TDirectory::mkDirWithChecks(getStateFilenameWithPath(dir).c_str(), loggingEnabled);
	if(result == TDirectory::DCR_SUCCESS || result == TDirectory::DCR_DIR_ALREADY_EXISTS) {
		return true;
	}
	else {
		return false;
	}
}

void TUtils::setStateFilesSubDir(std::string filename) {
	stateFilesSubDir = filename;
	ensureStateFileSubdirExists("");
}

std::string TUtils::getStateFilesSubDir(void)
{
    return stateFilesSubDir;
}

std::string TUtils::appendDateTimeToFilename(std::string filename) {
	time_t t;
	time(&t);
	struct tm st;
	
#ifdef _WIN32
	tm *timeptr = localtime(&t);
	memcpy(&st, timeptr, sizeof(st));
#else
	localtime_r(&t,&st);
#endif
	
	auto index = filename.find_last_of('.');
	if(index == string::npos) {
		return filename;
	}
	else {
		return filename.substr(0,index) +
		TUtils::format("_%02d%02d%02d_%02d%02d%02d", st.tm_year%100, st.tm_mon+1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec) +
		filename.substr(index);
	}
	
}

string TUtils::getResourceFileContents(string resourceName, string resourceType) {
	TFileReader fr(getResourceFilePath(resourceName, resourceType));
	if(!fr.isOpen()) {
		return "";
	}
	
	fr.seekFromEnd();
	int size = fr.tell();
	fr.seekFromBegin();
	
	string result(size, ' ');
	fr.read(&result[0], size);
	
	//int stringLengthTest = strlen(&result[0]);
	return result;
}


string TUtils::formatWithCommas(int n) {
        char buf[128];
        buf[127]=0;

        char* p = &buf[127];


        bool isNegative = false;
        if(n < 0) {
                isNegative = true;
                n *= -1;
        }

        int i=0;
        do {
                if(i%3 == 0 && i) {
                        *--p = ',';
                }
                *--p = '0' + (n % 10);

                n /= 10;
                i++;

        } while(n>0);

        if(isNegative) {
                *--p = '-';
        }
        return p;
}

