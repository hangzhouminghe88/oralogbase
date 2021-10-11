#include "lgc_TransactionHandler.h"
#include "lgc_Transaction.h"
#include "lgc_TOriginalQueue.h"
#include "lgc_TConsumer.h"
#include "lgc_TransactionMgr.h"
#include "lgc_api.h"

//...................................
//class LGC_TransactionHeandler
//功能: 当一个事务的数据收集完后
//      处理它
//...................................

//constructor and desctructor
LGC_TransactionHandler::LGC_TransactionHandler()
{
	return;
}

LGC_TransactionHandler::~LGC_TransactionHandler()
{
	return;
}

//static member functions

/*
*处理事务: 1.从活动事务列表中删除
*          2.如果事务需要分析，则将事务添加到分析队列中
*            否则扔掉
*/
int LGC_TransactionHandler::handle(LGC_Transaction* pTransaction)
{
	if(pTransaction == NULL 
		|| pTransaction->isTrsctEnd() == false)
	{
		lgc_errMsg("new failed \n");
		return -1;
	}
	
	//从活动事务列表中删除
	LGC_TransactionHandler::delFromActiveTList(pTransaction);
	
	
	if(pTransaction->isNeedApply()){//事务需要分析
		
		//将事务添加到分析结果队列中
		if(1 !=  LGC_TransactionHandler::addTrsctToQueue(pTransaction)){
			lgc_errMsg("applyTransaction failed \n");
			return -1;
		}
		pTransaction = NULL;

	}else{//事务不需要分析

		delete pTransaction;
		pTransaction = NULL;
	}
	
	//success
	return 0;
}

/*
*将事务添加到队列中
*/
int LGC_TransactionHandler::addTrsctToQueue(LGC_Transaction* pTransaction)
{
	const unsigned short threadId	= pTransaction->getThreadId();
	LGC_TOriginalQueue* pTQueue		= LGC_TOriginalQueue::getInstance(threadId);
	
	pTQueue->push(pTransaction);
	pTransaction = NULL;
	
	//success
	return 1;
}

/*
*从活动事务列表中删除事务
*/
void LGC_TransactionHandler::delFromActiveTList(const LGC_Transaction* pTransaction)
{
	const unsigned short threadId		= pTransaction->getThreadId();
	const LGC_TransactionId  transactionId	= pTransaction->getTransactionId();

	LGC_TransactionMgr::getInstance(threadId)->delTransaction(transactionId);
	
	return;
}

/*
*将事务消费掉
*/
/*
int LGC_TransactionHandler::consumeTrsct(LGC_Transaction* pTransaction)
{
	//pTransaction->addToTListOrderByBeginSCN();
	//pTransaction->addToTListOrderByCommitSCN();
		
	LGC_TConsumer* pTConsumer = LGC_TConsumer::createTConsumer(pTransaction);
	pTransaction = NULL;
	if(pTConsumer == NULL){
		lgc_errMsg("create failed \n");
		return -1;
	}

	if(pTConsumer->consume() < 0){
		lgc_errMsg("consume failed \n");
		LGC_TConsumer::freeTConsumer(pTConsumer);
		pTConsumer = NULL;
		return -1;
	}
	
	LGC_TConsumer::freeTConsumer(pTConsumer);
	pTConsumer = NULL;

	//success
	return 1;
}
*/