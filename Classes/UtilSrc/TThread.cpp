#include "TThread.h"
#include "TThreadI.h"

#ifdef __APPLE__ // defined(_MAC) || defined(IOS)
#	include <mach/mach.h>
#	include "pthread.h"
#endif

using namespace std;



