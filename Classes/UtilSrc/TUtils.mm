#import "TUtils.h"
#import "mach/mach.h" 
#import <Foundation/Foundation.h>

#include <time.h>
#include <sys/sysctl.h>
//#include "TStrUtils.h"
#include "TFile.h"
#include "TargetConditionals.h"
//#include "Mutex.h"

using namespace std;

#if defined(__has_feature) && __has_feature(objc_arc)
#	define HAVE_ARC 1
#else
#	define HAVE_ARC 0
#endif

long TUtils::usedMemory() {
	struct task_basic_info info;
	mach_msg_type_number_t size = sizeof(info);
	kern_return_t kerr = task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size);
	return (kerr == KERN_SUCCESS) ? info.resident_size : 0; // size in bytes
}

long TUtils::freeMemory() {
	mach_port_t host_port = mach_host_self();
	mach_msg_type_number_t host_size = sizeof(vm_statistics_data_t) / sizeof(integer_t);
	vm_size_t pagesize;
	vm_statistics_data_t vm_stat;
	
	host_page_size(host_port, &pagesize);
	(void) host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size);
	return vm_stat.free_count * pagesize;
}


void TUtils::executeWithinAutoreleasePool(std::function<void()> fn) {
#if HAVE_ARC
	@autoreleasepool
#endif
	{
		fn();
	}
}

std::string TUtils::getAppVersionAndBuild() {
	auto version = getAppVersion();
	auto build = getAppBuild();
	
	if(version == build) {
		return version;
	}
	else {
		return TUtils::format("%s(%s)", version.c_str(), build.c_str());
	}
}

std::string TUtils::getAppVersion() {
#if HAVE_ARC
	@autoreleasepool
#endif
	{
		NSString *version = [[NSBundle mainBundle] objectForInfoDictionaryKey: @"CFBundleShortVersionString"];
		return [version UTF8String];
	}
}
std::string TUtils::getAppBuild() {
#if HAVE_ARC
	@autoreleasepool
#endif
	{
		NSString *build   = [[NSBundle mainBundle] objectForInfoDictionaryKey: (NSString *)kCFBundleVersionKey];
		return [build UTF8String];
	}
}



//static_assert
#if TARGET_OS_IPHONE //IOS

#import <UIKit/UIKit.h>
//#import "Reachability/Reachability.h"

string TUtils::getStateFilenameWithPath(string filename) {

	//
	// new code with caching
	//
#if 1
	static std::string docsDir;
	if(!docsDir.size()) {
//		static Mutex mutex;
//		Lock l(mutex);
		if(!docsDir.size()) {
#if HAVE_ARC
			@autoreleasepool
#endif
			{
				NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
				NSString* documentsDirectory = [paths objectAtIndex:0];
				docsDir = [documentsDirectory UTF8String];
				if(docsDir.size() && docsDir[docsDir.size()-1] != '/') {
					docsDir += '/';
				}
			}
		}
	}
	
	if(stateFilesSubDir.size()) {
		return docsDir + stateFilesSubDir + "/" + filename;
	}
	else {
		return docsDir + filename;
	}

	//
	// deprecated - old code with no caching
	//
#else
	
#if HAVE_ARC
	@autoreleasepool
#endif
	{
		auto fullFilename = filename;
		if(stateFilesSubDir.size()) {
			fullFilename = stateFilesSubDir + "/" + filename;
		}
		
		
		NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
		NSString* documentsDirectory = [paths objectAtIndex:0];
		NSString* stateFile = [documentsDirectory stringByAppendingPathComponent:[NSString stringWithUTF8String:fullFilename.c_str()]];
		return string([stateFile UTF8String]);
	}
#endif
}

string TUtils::getSystemVersion() {
	NSString* result = [[UIDevice currentDevice] systemVersion];
	string resultStr = [result UTF8String];
	return resultStr;
}
string TUtils::getModel() {
	//NSString* result = [[UIDevice currentDevice] model];
	//String resultStr = [model UTF8String];
	//return resultStr;
	size_t size = -1;
	sysctlbyname("hw.machine", NULL, &size, NULL, 0);

	char* machine = new char[size+1];
	sysctlbyname("hw.machine", &machine[0], &size, NULL, 0);
	
	return machine;
}
#import <Foundation/Foundation.h>
#import <sys/stat.h>

const char * TUtils::pathForResource(const char *name)
{
    NSString *path = [[NSBundle mainBundle] pathForResource:[NSString stringWithUTF8String:name] ofType: nil];
    return [path fileSystemRepresentation];
}


string TUtils::getResourceFilePath(string resourceName, string resourceType) {
#if HAVE_ARC
	@autoreleasepool
#endif
	{
		NSString* resourceFile = [[NSBundle mainBundle] pathForResource:[NSString stringWithUTF8String:resourceName.c_str()] ofType:[NSString stringWithUTF8String:resourceType.c_str()]];
		return string([resourceFile fileSystemRepresentation]);
	}
}

string TUtils::getDeviceName() {
	NSString* uniqueId = [[UIDevice currentDevice] name];
	return [uniqueId UTF8String];
}

bool TUtils::connectedToWifi() {
//	if ([[Reachability reachabilityForLocalWiFi] currentReachabilityStatus] != ReachableViaWiFi) {
		//Code to execute if WiFi is not enabled
//		return false;
//	}
	return true;
}
#elif TARGET_OS_MAC // mac

#import <Foundation/Foundation.h>
#import <sys/types.h>
#import <sys/sysctl.h>


string TUtils::getStateFilenameWithPath(string filename) {
	auto fullFilename = filename;
	if(stateFilesSubDir.size()) {
		fullFilename = stateFilesSubDir + "/" + filename;
	}
	
	@autoreleasepool {
		NSArray* paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
		NSString* cachesDirectory = [paths[0] stringByAppendingPathComponent:[[NSBundle mainBundle] bundleIdentifier]];
		NSString* stateFile = [cachesDirectory stringByAppendingPathComponent:[NSString stringWithUTF8String:fullFilename.c_str()]];
		return string([stateFile fileSystemRepresentation]);
	}
}

string TUtils::getSystemVersion() {
	@autoreleasepool {
        return string([[[NSProcessInfo processInfo] operatingSystemVersionString] UTF8String]);
    }
}

string TUtils::getModel() {
	size_t size = -1;
	sysctlbyname("hw.model", NULL, &size, NULL, 0);
	char machine[size+1];
	sysctlbyname("hw.model", &machine[0], &size, NULL, 0);
	return string(machine);
}

string TUtils::getResourceFilePath(string resourceName, string resourceType) {
	@autoreleasepool {
		NSString* resourceFile = [[NSBundle mainBundle] pathForResource:[NSString stringWithUTF8String:resourceName.c_str()] ofType:[NSString stringWithUTF8String:resourceType.c_str()]];
		if (!resourceFile)
			return string("");
		return string([resourceFile fileSystemRepresentation]);
	}
}

string TUtils::getDeviceName() {
	return "OS X";
}

bool TUtils::connectedToWifi() {
	return true;
}

#else // huh? this can't happen!

string TUtils::getStateFilenameWithPath(string filename) {
	return filename;
}

string TUtils::getSystemVersion() {
	return "unknown";
}
string TUtils::getModel() {
	return "unknown";
}

string TUtils::getResourceFilePath(string resourceName, string resourceType) {
	return "unknown";
}

string TUtils::getDeviceName() {
	return "unknown";
}

bool TUtils::connectedToWifi() {
	return true;
}

#endif
