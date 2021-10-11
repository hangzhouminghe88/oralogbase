#include "lgc_ChangeHandler.h"
#include "lgc_Change.h"
#include "lgc_Transaction.h"
#include "lgc_TransactionMgr.h"
#include "lgc_TransactionHandler.h"
#include "lgc_api.h"

/*
*class LGC_ChangeHandler
*����: ����change
*��ϸ��������: 
*             1.���˵�û�õ�Change
*             2.��change�ύ����������������
*			  2.1	��ȡChange��TransactionId����
*             2.2	���������ʼ��Change�������TransactionId����������
*             2.3   ����TransactionId��TransactionMgr�л�ȡ����
*             2.4	��Change�ύ������
*/

//constructor and desctructor

/*
*���캯��
*/
LGC_ChangeHandler::LGC_ChangeHandler(LGC_Change* pChange)
{
	if(pChange == NULL){
		lgc_errMsg("pChange is NULL \n");
		exit(1);
	}

	m_pChange = pChange;
	return;
}

/*
*��������
*/
LGC_ChangeHandler::~LGC_ChangeHandler()
{
	if(m_pChange){
		LGC_Change::freeChange(m_pChange);
		m_pChange = NULL;
	}
	return;
}

//public member functions

/*
*����Change
*/
int LGC_ChangeHandler::handle()
{
	int iFuncRet = 0;
	
	//���˵�û��Ҫ������Change
	if( true == m_pChange->isNeedAddToTrsct() ){
		//��chang�ύ������
		iFuncRet = this->addChangeToTrsct();
		if(iFuncRet < 0){
			lgc_errMsg("addChangeToTrsct failed \n");
			return -1;
		}
	}

	return 0;
}

//private member functions

/*
*��change��ӵ�������
*/
int LGC_ChangeHandler::addChangeToTrsct()
{
	int iFuncRet = 0;

	const unsigned short threadId = m_pChange->getThreadId();
	const LGC_TransactionId transactionId = m_pChange->getTransactionId();
    const bool isBeginTrsctChange = LGC_ChangeHandler::isBeginTrsctChange(m_pChange);
	
	//��ȡTransactionMgr, ÿ����ȡ�̶߳����Լ���TransactionMgr
	LGC_TransactionMgr* pTransactionMgr = LGC_TransactionMgr::getInstance(threadId);
	
fprintf(stdout, "%s \n", m_pChange->toString().data());
	
	//����¿�ʼһ����������ӵ������������
	if(isBeginTrsctChange){
		iFuncRet = pTransactionMgr->addTransaction(transactionId);
		if(iFuncRet < 0){
			lgc_errMsg("addTransaction failed \n");
			//return -1;
		}
	}
	
	//������������л�ȡ����
	LGC_Transaction* pTransaction = pTransactionMgr->getTransaction(transactionId);
	if(pTransaction == NULL){//not find
		return 0; //ignore the change
	}
	
	//����Ӧ�����������change
	iFuncRet = pTransaction->addChange(&m_pChange);
	if(iFuncRet != 1 || m_pChange != NULL ){
		lgc_errMsg("add change to trsact failed \n");
		return -1;
	}
	
	//��������Ѿ�������������
	if( pTransaction->isTrsctEnd() ){
		if(LGC_TransactionHandler::handle(pTransaction) < 0){
			lgc_errMsg("handle transactions failed \n");
			return -1;
		}
		pTransaction = NULL;
	}

	//success
	return 1;
}

//static member functions

/*
*change�����Ƿ�������ʼchange
*/
bool LGC_ChangeHandler::isBeginTrsctChange(LGC_Change* pChange)
{
	return pChange->isBeginTrsctChange();
}

