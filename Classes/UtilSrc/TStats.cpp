#include "TStats.h"
#include "TUtils.h"
#include "TMap.h"

void TS_init(void** inst, const char* msg) {
	auto ts = new TStat(msg);
	*inst = (void*) ts;
	
	TStats::getInstance()->addStat(ts);
}
void TS_addValue(void* inst, float value) {
	TStat* ts = (TStat*) inst;
	ts->addValue(value);
}


void TS_count_init(void** inst, const char* msg) {
	auto ts = new TStatCount(msg);
	*inst = (void*) ts;
	
	TStats::getInstance()->addCountStat(ts);
}
void TS_count_addValue(void* inst, float value) {
	TStatCount* ts = (TStatCount*) inst;
	ts->addValue(value);
}

// experimental internal
void TStats::ensureStaticsLoaded() {
	TStats::getInstance();
}

TMap<std::string,TStat*>& TStats::getStatsMap() {
	static TMap<std::string,TStat*> myMap;
	for(;;) {
		int n = myMap.size();
		if(allStats.haveValueAtPos(n)) {
			TStat* ts = allStats[n];
			
			auto s = TUtils::toLower(ts->msg);
			while(myMap.contains(s)) {
				s += " ";
			}
			myMap[s] = ts;
		}
		else {
			break;
		}
	}
		
	return myMap;
}

TMap<std::string,TStatCount*>& TStats::getStatCountsMap() {
	static TMap<std::string,TStatCount*> myMap;
	for(;;) {
		int n = myMap.size();
		if(allCountStats.haveValueAtPos(n)) {
			TStatCount* ts = allCountStats[n];
			if(ts) {
				auto s = TUtils::toLower(ts->msg);
				while(myMap.contains(s)) {
					s += " ";
				}
				myMap[s] = ts;
			}
		}
		else {
			break;
		}
	}
	
	return myMap;
}

TStat* TStats::getStatByKey(const std::string& key) {
	Lock l(mapMutex);
	auto& allStats = getStatsMap();
	auto it = allStats.find(TUtils::toLower(key));
	if(it != allStats.end()) {
		return it->second;
	}
	return nullptr;
}

TStatCount* TStats::getCountStatByKey(const std::string& key) {
	Lock l(mapMutex);
	auto& allStats = getStatCountsMap();
	auto it = allStats.find(TUtils::toLower(key));
	if(it != allStats.end()) {
		return it->second;
	}
	return nullptr;
}


