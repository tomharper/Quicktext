#include "DeadlockMonitor.h"

#if TRACK_DEADLOCKS

#include "Mutex.h"
#include "TThread.h"

DeadlockMonitor* DeadlockMonitor::getInstance() {
	static DeadlockMonitor dm;
	return &dm;
}

int DeadlockMonitor::onEntry() {
	auto* ti = new ThreadInf;
	int result = threads.insertMW(ti);
	
	++nThreads;
	
	// temp debug - find out what is piling up here!???
	int nt = nThreads.Value();
	if(nt > 100 && nt % 200 == 0) {
		haveDeadlock();
	}
	
	return result;
}
void DeadlockMonitor::onExit(int index) {
	
	threads.eraseIndexMR(index);
	--nThreads;
}

void DeadlockMonitor::tick() {
	if(timeSinceLastCheck.getTimePassedMs() > 2000 && timeSinceLastDump.getTimePassedMs() > 10000) {
		timeSinceLastCheck.reset();
		
		// do our stuff ..
		int n=0;
		bool foundDeadlock = false;
		
		threads.forEachMR([&](ThreadInf* t) {
			if(t && t->t.getTimePassedMs() > 4000) {
				foundDeadlock = true;
			}
			n++;
		});
		
		//
		// make sure detect deadlock at least twice before report (as timer values
		// could jump high just coz like device was paused or something)
		//
		if(foundDeadlock) {
			if(foundDeadlockCount++ > 1) {
				TThread::sleep(100);
				haveDeadlock();
			}
		}
	}
}

void DeadlockMonitor::haveDeadlock() {
	PrintfBuf pbuf;

	pbuf.printf("### Deadlock detected!? ###\n");
	
	int n=0;
	for(auto* t: threads) {
		pbuf.printf("Stack #%d", n++);
		t->si.outputSymbols(pbuf);
		timeSinceLastDump.reset();
	}
	
	// make sure that still have 2+ threads, i.e. perhaps it wasn't really
	// a deadlock and now over ...?
	if(n > 1) {
		TLogging::logDirect(LogLevel::ERROR, pbuf.str());
	}
}

#endif
