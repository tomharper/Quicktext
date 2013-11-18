#ifndef _TAverage_h
#define _TAverage_h

#include "TCommon.h"
#include "TTimer.h"
#include "TStaticArray.h"

class Average {
	unsigned long count = 0;
	float total = 0.0;
public:
	void addN(float f, int n) {
		count += n;
		total += f*n;
	}
	void add(float f) { 
		count++; total += f; 
	}
	float getAverage()const { 
		auto countT = count;
		if(countT == 0) return -1;
		return total / countT;
	}
	float getTotal()const { return total; }
	unsigned long getCount()const { return count; }
	void reset() { 
		total = 0.0f;
		count = 0; 
	}
};

/*
class AverageI {
	unsigned long count = 0;
	unsigned long total = 0;
public:
	void addN(int i, int n) {
		count += n;
		total += i*n;
	}
	void add(int i) { 
		count++; total += i; 
	}
	float getAverage()const { 
		auto tcount = count;
		if(tcount == 0) return -1; 
		return static_cast<float>(total) / tcount; 
	}
	float getTotal()const { return total; }
	unsigned long getCount()const { return count; }
	void reset() { 
		total = 0; 
		count = 0; 
	}
};
*/

class RecentAverageTotalPerSecond {
	float previousResult = -1;
	float lastSecondTotal = 0.0f;
	float total = 0.0f;
	Average av;
	Timer t; // SharedTimer - now handled automatically globally ....
	Timer totalTime;
public:
	void add(float f) {
		lastSecondTotal += f;
		total += f;
	}
	float getLatestAveragePerSecond() {
		if(t.getTimePassedMs() > 500) {
			previousResult = (lastSecondTotal*1000.0f)/t.getTimePassedAndReset();
			lastSecondTotal = 0.0f;
		}
		av.add(previousResult);
		return previousResult;
	}
	float getAveragePerSecond() {
		int timePassed = totalTime.getTimePassedMs();
		if(timePassed == 0)
			return -1;
		
		return (total*1000.0f) / static_cast<float>(timePassed);
		//return av.getAverage();
	}
	float getTotal() {
		return total;
	}
};
	
template<class T, int LA_SIZE>
class LimitedAverage  {
	TStaticDeque<T, LA_SIZE> recentValues;
	int count = 0;
	T runningTotal = 0;
	
public:
	
	LimitedAverage() {}
	
	void reset() {
		count = 0;
		runningTotal = 0;
		recentValues.clear();
	}
	
	void add(T newValue) {
        count++;
		runningTotal += newValue;
		
		if(recentValues.size() >= LA_SIZE) {
			runningTotal -= recentValues.pop_front();
		}
		recentValues.push_back(newValue);
	}
	
	int getCount()const { return count; }
	T   getTotal()const { return runningTotal; }
	
	T getAverage()const {
		if(count == 0) {
			return -1;
		}
		
		int sizeT = TMin(count, LA_SIZE);
		
		return runningTotal / sizeT;
	}
};
	
struct WeightedAverage {
	float av = 0.0f;
	float mul = 0.0f;

	WeightedAverage(float _mul):mul(_mul){}
	
	void add(float v) {
		av = av*mul + v;
	}
};

#endif
