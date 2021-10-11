#include "lgc_ConsumeThread.h"
#include "lgc_TMergedQueue.h"
#include "lgc_TConsumer.h"
#include "lgc_api.h"

/*
*ģ����: �����߳�
*��TMergedQueue��pop������Ȼ�����ѵ�
*
*/

static LGC_Transaction* popFromMergedQueue(LGC_TMergedQueue* pTMergedQueue);
static int consumeTransaction(LGC_Transaction* pTransaction);

/*
*�����̵߳��̺߳���
*/
void* consumeThreadFunc(void* pConsumeThreadArg)
{
	const LGC_ConsumeThreadArg*		pThreadArg		= (LGC_ConsumeThreadArg*)pConsumeThreadArg;
	const unsigned short			threads			= pThreadArg->threads;
	LGC_TMergedQueue*				pTMergedQueue	= LGC_TMergedQueue::getInstance();
	
	LGC_Transaction* pTransaction = NULL;
	while(true){
		//��TMergedQueue��pop������
		pTransaction = popFromMergedQueue(pTMergedQueue);
		if(pTransaction == NULL){
			lgc_errMsg("pTransaction is NULL \n");
			exit(1);
		}
		
		//���������ѵ�
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
*��TMergedQueue��pop������
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
*���ѵ�����
*/
int consumeTransaction(LGC_Transaction* pTransaction)
{
	int iRet = 0;
	
	//��������������
	LGC_TConsumer* pTConsumer = LGC_TConsumer::createTConsumer(pTransaction);
	pTransaction			  = NULL;

	if(pTConsumer == NULL){
		lgc_errMsg("createTConsumer failed \n");
		iRet = -1;
		goto errOut;
	}
	
	//�����������߰��������ѵ�
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