#ifndef _TUtils_h
#define _TUtils_h

#include "TCommon.h"
#include <string>
#include "FunctionalWrapper.h"

class TUtils {
public:
	static std::string stateFilesSubDir;
	static char* readFile(const char *name);
    
	static long usedMemory();
	static long freeMemory();
    static const char * pathForResource(const char *name);

	static bool connectedToWifi();
	static std::string toLower(const std::string& s);
	static std::string trim(const std::string& s);
	static std::string formatWithCommas(int n);

    static std::string format(const std::string& s, ...);
    static std::string format(const char* s, ...);
	
	static void setStateFilesSubDir(std::string filename);
	static std::string getStateFilesSubDir(void);
    
	static std::string getStateFilenameWithPath(std::string filename);
	static std::string appendDateTimeToFilename(std::string filename);

	static std::string getResourceFileContents(std::string resourceName, std::string type);
	static std::string getResourceFilePath(std::string resourceName, std::string resourceType);
	
	static std::string getSystemVersion();
	static std::string getModel();
	static std::string getDeviceName();

	static std::string getAppVersionAndBuild();
	static std::string getAppVersion();
	static std::string getAppBuild();

	static void executeWithinAutoreleasePool(std::function<void()> fn);

	/// returns true on success, false on failure
	static bool ensureStateFileSubdirExists(const std::string& dir, bool loggingEnabled=true);
	
	/**
	 For use with textures ....
	 */
	static int getNextPowerOf2(int w) {
		if(w<16)
			w = 16;
		
		if((w != 1) && (w & (w - 1))) {
			int i = 1;
			
			int repeats = 0;
			while(i < w) {
				i *= 2;
				repeats++;
				if(repeats>=100) {
					return 2; // guess this can happen if w>2^30
				}
			}
			w = i;
		}
		return w;
	}
	
};


#endif
