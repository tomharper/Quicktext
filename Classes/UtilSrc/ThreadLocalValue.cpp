#include "ThreadLocalValue.h"

void globalDestructorValue(void *value) {
	ValueHolderI* stackP = (ValueHolderI*)value;
	pthread_key_t tlsKey = stackP->tlsKey;
	stackP->deleteValue();
	delete stackP;
	pthread_setspecific(tlsKey, NULL);
}
