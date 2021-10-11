#include "lgc_TransactionHandler.h"
#include "lgc_Transaction.h"
#include "lgc_TOriginalQueue.h"
#include "lgc_TConsumer.h"
#include "lgc_TransactionMgr.h"
#include "lgc_api.h"

//...................................
//class LGC_TransactionHeandler
//����: ��һ������������ռ����
//      ������
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
*��������: 1.�ӻ�����б���ɾ��
*          2.���������Ҫ��������������ӵ�����������
*            �����ӵ�
*/
int LGC_TransactionHandler::handle(LGC_Transaction* pTransaction)
{
	if(pTransaction == NULL 
		|| pTransaction->isTrsctEnd() == false)
	{
		lgc_errMsg("new failed \n");
		return -1;
	}
	
	//�ӻ�����б���ɾ��
	LGC_TransactionHandler::delFromActiveTList(pTransaction);
	
	
	if(pTransaction->isNeedApply()){//������Ҫ����
		
		//��������ӵ��������������
		if(1 !=  LGC_TransactionHandler::addTrsctToQueue(pTransaction)){
			lgc_errMsg("applyTransaction failed \n");
			return -1;
		}
		pTransaction = NULL;

	}else{//������Ҫ����

		delete pTransaction;
		pTransaction = NULL;
	}
	
	//success
	return 0;
}

/*
*��������ӵ�������
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
*�ӻ�����б���ɾ������
*/
void LGC_TransactionHandler::delFromActiveTList(const LGC_Transaction* pTransaction)
{
	const unsigned short threadId		= pTransaction->getThreadId();
	const LGC_TransactionId  transactionId	= pTransaction->getTransactionId();

	LGC_TransactionMgr::getInstance(threadId)->delTransaction(transactionId);
	
	return;
}

/*
*���������ѵ�
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