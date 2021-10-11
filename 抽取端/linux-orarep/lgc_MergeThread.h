#ifndef LGC_MERGETHREAD_H
#define LGC_MERGETHREAD_H

#include "lgc_Defines.h"
#include "lgc_Structure.h"

struct LGC_MergeThreadArg
{
	unsigned short threads;
	const char*	   mediaFileDir;
};

void* mergeThreadFunc(void* pLGC_MergeThreadArg);

#endif
