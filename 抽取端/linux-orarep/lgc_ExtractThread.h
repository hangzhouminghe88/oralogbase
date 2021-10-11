#ifndef LGC_EXTRACTTHREAD_H
#define LGC_EXTRACTTHREAD_H

#include <stdio.h>
#include <stdlib.h>

#include "lgc_Defines.h"
#include "lgc_Structure.h"

struct LGC_ExtractThreadArg
{
	unsigned short	threadId;
	BYTE8			startSCN;
	unsigned short	threads;
	const char*		tnsname;
	const char*     password;
	const char*     username;
};

void* extractThreadFunc(void* pExtractThreadArg);
#endif

