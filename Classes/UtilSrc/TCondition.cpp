#include "TCondition.h"
#include "TDateTime.h"
#include <string>

#if USING_CPP11_CONDITIONAL

void TCondition::notifyOne() { c.notify_one(); }
void TCondition::notifyAll() { c.notify_all(); }

void TCondition::wait(LockNR& lock) {
	c.wait(lock);
}

bool TCondition::waitWithTimeout(LockNR& lock, int timeoutMs) {
	return c.wait_for(lock, std::chrono::milliseconds(timeoutMs)) != std::cv_status::timeout;
}


void TConditionNL::notifyOne() { c.notify_one(); }
void TConditionNL::notifyAll() { c.notify_all(); }

void TConditionNL::wait() {
	std::unique_lock<std::mutex> l(mutex);
	c.wait(l);
}
bool TConditionNL::waitWithTimeout(int timeoutMs) {
	std::unique_lock<std::mutex> l(mutex);
	return c.wait_for(l, std::chrono::milliseconds(timeoutMs)) != std::cv_status::timeout;
}

#else

TCondition::~TCondition() {
	pthread_cond_destroy(&c);
}
TCondition::TCondition() {
	// seems this is not necessary ...?
	int ec = pthread_cond_init(&c, NULL);
}
void TCondition::notifyOne() {
	pthread_cond_signal(&c);
}

void TCondition::notifyAll() {
	pthread_cond_broadcast(&c);
}

void TCondition::wait(LockNR& lock)
{
	lock.owns_lock();
	int ec = pthread_cond_wait(&c, lock.mutex()->native_handle());
}

bool TCondition::waitWithTimeout(LockNR& lock, int timeoutMs) {
	lock.owns_lock();

	TDateTime dt { TDateTime::now() + TDateTime::fromMs(timeoutMs) };
	
	timespec ts = dt.asTS();
	int ec = pthread_cond_timedwait(&c, lock.mutex()->native_handle(), &ts);

	if (ec != 0 && ec != ETIMEDOUT) {
        //
	}
	
	// supposed to return false if times-out
	return ec != ETIMEDOUT;
}

/*
void notify_all_at_thread_exit(condition_variable& cond, unique_lock<mutex> lk) {
	__thread_local_data()->notify_all_at_thread_exit(&cond, lk.release());
}
*/

#endif
