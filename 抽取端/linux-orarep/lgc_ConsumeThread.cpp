#include "lgc_ConsumeThread.h"
#include "lgc_TMergedQueue.h"
#include "lgc_TConsumer.h"
#include "lgc_api.h"

/*
*模块名: 消费线程
*从TMergedQueue中pop出事务，然后消费掉
*
*/

static LGC_Transaction* popFromMergedQueue(LGC_TMergedQueue* pTMergedQueue);
static int consumeTransaction(LGC_Transaction* pTransaction);

/*
*消费线程的线程函数
*/
void* consumeThreadFunc(void* pConsumeThreadArg)
{
	const LGC_ConsumeThreadArg*		pThreadArg		= (LGC_ConsumeThreadArg*)pConsumeThreadArg;
	const unsigned short			threads			= pThreadArg->threads;
	LGC_TMergedQueue*				pTMergedQueue	= LGC_TMergedQueue::getInstance();
	
	LGC_Transaction* pTransaction = NULL;
	while(true){
		//从TMergedQueue中pop出事务
		pTransaction = popFromMergedQueue(pTMergedQueue);
		if(pTransaction == NULL){
			lgc_errMsg("pTransaction is NULL \n");
			exit(1);
		}
		
		//把事务消费掉
		if(consumeTransaction(pTransaction) < 0){
			lgc_errMsg("consumeTransaction failed \n");
			exit(1);
		}
		pTransaction = NULL;
	}

	//never to here
	lgc_errMsg("never to here \n");
	exit(1);
	return NULL;
}

/*
*从TMergedQueue中pop出事务
*/
LGC_Transaction* popFromMergedQueue(LGC_TMergedQueue* pTMergedQueue)
{
	LGC_Transaction* pTransaction = NULL;
	while(NULL == 
		   (pTransaction = pTMergedQueue->pop()) 
		  )
	{
		sleep(1);
	}

	return pTransaction;
}

/*
*消费掉事务
*/
int consumeTransaction(LGC_Transaction* pTransaction)
{
	int iRet = 0;
	
	//创建事务消费者
	LGC_TConsumer* pTConsumer = LGC_TConsumer::createTConsumer(pTransaction);
	pTransaction			  = NULL;

	if(pTConsumer == NULL){
		lgc_errMsg("createTConsumer failed \n");
		iRet = -1;
		goto errOut;
	}
	
	//用事务消费者把事务消费掉
	if(pTConsumer->consume() < 0){
		lgc_errMsg("consume failed \n");
		iRet = -1;
		goto errOut;
	}

errOut:
	LGC_TConsumer::freeTConsumer(pTConsumer);
	pTConsumer = NULL;
	return iRet;
}