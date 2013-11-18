#ifndef _TStats_h
#define _TStats_h

/**
 Efficient+lightweight statistics tracking macros, useable from both C & C++ code.
 
 Use the macros:
 - TSTATS_VAL("key", intOrFloatValue) to note the value of a variable.
   - the count, last value, average will be tracked
 - TSTATS_COUNT("key", intOrFloatValue) to append the total of some variable by the given value
   - the count, total, average of count+total per second, and count+total for the last second will be tracked
 
 Each key may only be used on ONE line of code, and the key should be a constant char*
 dynamically created.
 */

/**
 Defining here the specific strings used in webrtc in some of the TSTATS_* lines, so can use those string
 as keys in other part of the code to do sanity checks to check that their values fall within certain expected
 ranges ...
*/
#define TS_BOTTLENECKS_BIG "bottlenecks BIG"
#define TS_BOTTLENECKS "bottlenecks"
#define TS_RTP_RTT "RtpRtcp - RTT"
#define TS_RTP_RTT_AVG "RtpRtcp - RTT avg"
#define TS_RTP_RTT_MIN "RtpRtcp - RTT min"
#define TS_RTP_RTT_MAX "RtpRtcp - RTT max"
#define TS_QSIZE_LOGGING "Q-size: Logging"
#define TS_CPU_USAGE "CPU usage"

#define TS_AD_PLAYDELAY_TOTAL "AD:PlayDelay Total (ms)"
#define TS_AD_RECDELAY_TOTAL "AD:RecordDelay Total (ms)"

#define TS_RTP_BITRATE_SEND "RtpRtcp - Bitrate Kbit/s Send"
#define TS_RTP_BITRATE_VID  "RtpRtcp - Bitrate Kbit/s Vid"
#define TS_RTP_BITRATE_FEC  "RtpRtcp - Bitrate Kbit/s FEC"
#define TS_RTP_BITRATE_NACK "RtpRtcp - Bitrate Kbit/s NACK"

#define TS_STATE_VOICE_RECV "STATE: voice recv"
#define TS_STATE_VOICE_SEND "STATE: voice send"

#define TS_RTP_RECV_KBPS "RTPRecver recv kbps"
#define TS_RTP_SEND_KBPS "RTPSender send kbps"

#define TS_TIME_ENC_FULL       "TIME: Enc1"
#define TS_TIME_DEC_FULL       "TIME: Dec1"

#define TS_TIME_LAST_RECEIVED_RTP_PACKET "Time last rcv pkt"

#ifdef __cplusplus
#include "TStatsI.h"
extern "C" {
#endif //__cplusplus

void TS_init(void** inst, const char* msg);
void TS_addValue(void* inst, float value);

void TS_count_init(void** inst, const char* msg);
void TS_count_addValue(void* inst, float value);

#define TSTATS_VAL(msg, value) { \
	static void* zs; \
	static int zsInited = 0; \
	if(zsInited == 0) { \
		TS_init(&zs, msg); \
		zsInited = 1; \
	} \
	TS_addValue(zs, value); \
}

#define TSTATS_COUNT(msg, value) { \
	static void* zs; \
	static int zsInited = 0; \
	if(zsInited == 0) { \
		TS_count_init(&zs, msg); \
		zsInited = 1; \
	} \
	TS_count_addValue(zs, value); \
}

#define TSTATS_INC(msg) { \
	static void* zs; \
	static int zsInited = 0; \
	if(zsInited == 0) { \
		TS_count_init(&zs, msg); \
		zsInited = 1; \
	} \
	TS_count_addValue(zs, 1); \
}

#ifdef __cplusplus
}
#endif

#endif // #ifndef _TStats_h
