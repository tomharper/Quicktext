#ifndef _TStatsInternal_h
#define _TStatsInternal_h

#include "TCommon.h"
#include "MemoryWrapper.h"
#include <string>
#include "Average.h"
#include "Mutex.h"

using namespace std;

class TStatCount {
    RecentAverageTotalPerSecond avTotalValuePS, avCountPS;
public:
    const char* msg;
    
    TStatCount(const char* _msg):msg(_msg) {}
    
    string getstr() { string temp(msg); return temp; }
    
    void addValue(float value) {
        avTotalValuePS.add(value);
        avCountPS.add(1);
    }
    float getLatestTotalValuePerSecond() { return avTotalValuePS.getLatestAveragePerSecond(); }
    float getLatestCountPerSecond() { return avCountPS.getLatestAveragePerSecond(); }
    
    
    float getAverageTotalValuePerSecond() { return avTotalValuePS.getAveragePerSecond(); }
    float getAverageCountPerSecond() { return avCountPS.getAveragePerSecond(); }
    
    float getTotal() { return avTotalValuePS.getTotal(); }
    float getCount() { return avCountPS.getTotal(); }
};

class TStat {
    Average av;
    float lastValue = -1;
public:
    const char* msg;
    
    TStat(const char* _msg):msg(_msg) {}
    
    string getstr() { string temp(msg); return temp; }
    
    void addValue(float value) {
        lastValue = value;
        av.add(value);
    }
    
    float getLastValue() { return lastValue; }
    float getAverage() { return av.getAverage(); }
    int getCount() { return (int)av.getCount(); }
};

class TStats {
public:
    TStaticArrayMRMW<TStat*,512> allStats;
    TStaticArrayMRMW<TStatCount*,512> allCountStats;
    
    static SharedPtr<TStats> getInstance() {
        static auto instance = make_shared<TStats>();
        return instance;
    }
    
    /**
     Lock the mapMutex around when getting+using the maps returned from getStatsMap() +
     getStatCountsMap().
     
     The keys are the lower-case versions of the name ('msg') of the corresponding values
     (lower-cased so sorted nicer, i.e. StrUtils::toLower()).
     */
    Mutex mapMutex;
    TMap<std::string,TStat*>& getStatsMap();
    TMap<std::string,TStatCount*>& getStatCountsMap();
    
    TStat* getStatByKey(const std::string& key);
    TStatCount* getCountStatByKey(const std::string& key);
    
    void addStat(TStat* stat) {
        allStats.push_back(stat);
    }
    void addCountStat(TStatCount* stat) {
        allCountStats.push_back(stat);
    }
    
    // experimental internal
    static void ensureStaticsLoaded();
};

#endif
