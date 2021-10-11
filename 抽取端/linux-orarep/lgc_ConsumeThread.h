#ifndef LGC_CONSUMETHREAD_H
#define LGC_CONSUMETHREAD_H

struct LGC_ConsumeThreadArg
{
	unsigned short threads;
};

void* consumeThreadFunc(void* pConsumeThreadArg);
#endif
