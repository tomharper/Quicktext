#include "TLogging.h"
#import <Foundation/Foundation.h>
#include <string.h>

int TLogging::parseObjCLogMsg(char* destBuf, const char* msg, va_list args, int lenLimit) {
	auto* str = [[NSString alloc] initWithFormat:[NSString stringWithUTF8String:msg] arguments:args];
	
	const char* p = [str UTF8String];
	
	int len = strlen(p);
	int myLen = TMin(len, lenLimit);

	memcpy(destBuf, p, myLen);
	destBuf[myLen] = 0;
	return myLen;
}
