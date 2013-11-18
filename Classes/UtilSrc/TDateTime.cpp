#include "TDateTime.h"
//#include "StrUtils.h"
#include "TLogging.h"
#include <string>
#ifndef ANDROID
#include <xlocale.h>
#endif


#ifdef ANDROID
static int is_leap(unsigned y) {
	y += 1900;
	return (y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0);
}

time_t timegm (struct tm *tm) {
	static const unsigned ndays[2][12] ={
		{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
		{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
	};
	time_t res = 0;
	int i;

	for (i = 70; i < tm->tm_year; ++i) {
		res += is_leap(i) ? 366 : 365;
	}

	for (i = 0; i < tm->tm_mon; ++i) {
		res += ndays[is_leap(tm->tm_year)][i];
	}
	res += tm->tm_mday - 1;
	res *= 24;
	res += tm->tm_hour;
	res *= 60;
	res += tm->tm_min;
	res *= 60;
	res += tm->tm_sec;

	return res;
}

#endif


TDateTime TDateTime::now() {
	TDateTime result;
	result.setToNow();
	return result;
}
TDateTime TDateTime::fromMs(int ms) {
	TDateTime result;
	result.tv.tv_sec = ms/1000;
	result.tv.tv_usec = (ms%1000)*1000;
	return result;
}

timespec TDateTime::asTS()const {
	timespec result;
	result.tv_sec = tv.tv_sec;
	result.tv_nsec = tv.tv_usec*1000;
	return result;
}


/*
2013-04-19T01:05:20.250
*/
TDateTime TDateTime::fromString(const std::string& _str) {
	std::string str = _str;
	if(str[10] == 'T')
		str[10] = ' ';
	
	TDateTime result;
	struct tm t;
#ifdef ANDROID
	char* res = strptime(&str[0], "%Y-%m-%d %H:%M:%S", &t);
#else
	char* res = strptime_l(&str[0], "%Y-%m-%d %H:%M:%S", &t, NULL);
#endif
	
	if(!res) {
		TLogError("Failure parsing time string: '%s'", str.c_str());
		return result;
	}
	if(res != &str[19]) {
		TLogError("Expecting string parsing to reach 19th char, but reached char: %d", int(res - &str[0]));
		return result;
	}
	
	// note - not how standard this is, but need to get UTC tm into timeval!
	time_t tt = timegm(&t);
	result.tv.tv_sec = tt;
	result.tv.tv_usec = 0;
	if(str.size() > 22) {
		char* msS = &str[20];
		if(msS[0] < '0' || msS[0] > '9' || msS[1] < '0' || msS[1] > '9' || msS[2] < '0' || msS[2] > '9') {
			TLogError("ms invalid!?: %s", msS);
		}
		else {
			int ms = ((msS[0]-'0')*100) + ((msS[1]-'0')*10) + (msS[2]-'0');
			result.tv.tv_usec = ms*1000;
		}
	}
	else {
		TLogError("Expected ms, but didn't find?: %s", str.c_str());
	}
	return result;
}
void TDateTime::setToNow() {
	/* from logging .....
	 time_t t;
	 time(&t);
	 
	 #ifdef _WIN32
	 tm *timeptr = localtime(&t);
	 memcpy(&result.st, timeptr, sizeof(st));
	 #else
	 localtime_r(&t,&result.st);
	 #endif
	 */

	gettimeofday(&tv, NULL);
};

void TDateTime::add(const struct timeval& offset) {
	tv.tv_sec += offset.tv_sec;
	tv.tv_usec += offset.tv_usec;
}
void TDateTime::add(const TDateTime& offset) {
	add(offset.tv);
}

std::string TDateTime::asGmtStr(TimeOutputFormat tof)const {
	return outputStr(gmtime(&tv.tv_sec), tof);
}
std::string TDateTime::asLocalStr(TimeOutputFormat tof)const {
	return outputStr(localtime(&tv.tv_sec), tof);
}

int TDateTime::outputLocalTimeToBuf(char* buf, TimeOutputFormat tof)const {
	return outputToBuf(localtime(&tv.tv_sec), buf, tof);
}
int TDateTime::outputGmTimeToBuf(char* buf, TimeOutputFormat tof)const {
	return outputToBuf(gmtime(&tv.tv_sec), buf, tof);
}

int TDateTime::outputToBuf(struct tm *ptm, char *_buf, TimeOutputFormat tof)const {
	char buf[64];
	if(tof == TOF_COMPACT) {
		strftime(buf, sizeof (buf), "%y%m%d %H%M:%S", ptm);
	}
	else if(tof == TOF_DEFAULT) {
		strftime(buf, sizeof (buf), "%Y-%m-%d %H:%M:%S", ptm);
	}
	else {
		strftime(buf, sizeof (buf), "%Y%m%d_%H%M_%S", ptm);
	}
	long ms = tv.tv_usec / 1000;
	
	if(tof == TOF_FILENAME) {
		return sprintf(_buf, "%s_%03ld", buf, ms);
	}
	else {
		return sprintf(_buf, "%s.%03ld", buf, ms);
	}
}

std::string TDateTime::outputStr(struct tm* ptm, TimeOutputFormat tof)const {
	char buf[64];
	outputToBuf(ptm, buf, tof);
	return buf;
}

TDateTime::TDateTime() { tv.tv_sec = 0; tv.tv_usec = 0; }
TDateTime::TDateTime(long sec, long usec) { tv.tv_sec = sec; tv.tv_usec = usec; }
TDateTime::TDateTime(const struct timeval& _tv) { tv.tv_sec = _tv.tv_sec; tv.tv_usec = _tv.tv_usec; }

/*
TDateTime::TDateTime(NtpTime& ntp) {
	tv.tv_sec = ntp.second - 0x83AA7E80; // secs from Jan 1, 1900 to Jan 1, 1970
	tv.tv_usec = (uint32_t)((double)ntp.fraction * 1.0e6 / (double)(1LL<<32));
}
 */


int64_t TDateTime::asMs()const {
	return (static_cast<int64_t>(tv.tv_sec)*1000) + (static_cast<int64_t>(tv.tv_usec)/1000);
}


int64_t TDateTime::diffMS(TDateTime& _tv)const {
	return (static_cast<int64_t>(tv.tv_sec)*1000) + (static_cast<int64_t>(tv.tv_usec)/1000) - _tv.asMs();
}

void TDateTime::operator+=(const struct timeval& _tv) { *this = *this + _tv; }
void TDateTime::operator-=(const struct timeval& _tv) { *this = *this - _tv; }
void TDateTime::operator+=(const TDateTime& dt)       { *this = *this + dt; }
void TDateTime::operator-=(const TDateTime& dt)       { *this = *this - dt; }

TDateTime TDateTime::operator+(const struct timeval& _tv)const {
	TDateTime result { tv.tv_sec + _tv.tv_sec, tv.tv_usec + _tv.tv_usec };
	if(result.tv.tv_usec > 1000000) {
		result.tv.tv_usec -= 1000000;
		result.tv.tv_sec += 1;
	}
	return result;
}
TDateTime TDateTime::operator-(const struct timeval& _tv)const {
	TDateTime result { tv.tv_sec - _tv.tv_sec, tv.tv_usec - _tv.tv_usec };
	if(result.tv.tv_usec < 0) {
		result.tv.tv_usec += 1000000;
		result.tv.tv_sec -= 1;
	}
	return result;
}
TDateTime TDateTime::operator+(const TDateTime& dt)const {
	return *this + dt.tv;
}
TDateTime TDateTime::operator-(const TDateTime& dt)const {
	return *this - dt.tv;
}

