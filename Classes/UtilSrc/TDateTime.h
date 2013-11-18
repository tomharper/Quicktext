#ifndef __TDateTime__
#define __TDateTime__

#include <sys/time.h>
#include <sys/types.h> // int64_t?
#include <stdint.h> // uint32_t...
#include <iosfwd>

class TDateTime {
public:
	
	static TDateTime now();
	static TDateTime fromString(const std::string& str);
	static TDateTime fromMs(int ms);
	
	TDateTime();
	TDateTime(const struct timeval& tv);
	TDateTime(long sec, long usec=0);
	
	bool isSet() {
		return tv.tv_sec > 0 || tv.tv_usec > 0;
	}
	void setToNow();
	
	void add(const struct timeval& offset);
	void add(const TDateTime& offset);
	
	/**
	 Default format: yyyy-mm-dd hh::mm::ss.xms
	 Compact format: yymmdd hh:mm:ss.xms
	 Filename fmt:   yymmdd_hh_mm_ss_xms
	 */
	
	enum TimeOutputFormat { TOF_DEFAULT, TOF_COMPACT, TOF_FILENAME };
	
	std::string asGmtStr(TimeOutputFormat tof = TOF_DEFAULT)const;
	std::string asLocalStr(TimeOutputFormat tof = TOF_DEFAULT)const;

	int64_t diffMS(TDateTime& _tv)const;
	int64_t asMs()const;
	timeval asTV()const { return tv; }
	timespec asTS()const;
	
	int outputLocalTimeToBuf(char* buf, TimeOutputFormat tof = TOF_DEFAULT)const;
	int outputGmTimeToBuf(char* buf, TimeOutputFormat tof = TOF_DEFAULT)const;
	
	void operator+=(const struct timeval& _tv);
	void operator-=(const struct timeval& _tv);

	void operator+=(const TDateTime& dt);
	void operator-=(const TDateTime& dt);
	
	TDateTime operator+(const struct timeval& _tv)const;
	TDateTime operator-(const struct timeval& _tv)const;

	TDateTime operator+(const TDateTime& dt)const;
	TDateTime operator-(const TDateTime& dt)const;

	
private:
	//struct tm st;
	std::string outputStr(struct tm* ptm, TimeOutputFormat tof)const;
	int outputToBuf(struct tm* ptm, char* buf, TimeOutputFormat tof)const;
	
	struct timeval tv;
};

#endif /* defined(__DateTime__) */
