#include "lgc_ChangeHandler.h"
#include "lgc_Change.h"
#include "lgc_Transaction.h"
#include "lgc_TransactionMgr.h"
#include "lgc_TransactionHandler.h"
#include "lgc_api.h"

/*
*class LGC_ChangeHandler
*功能: 处理change
*详细功能描述: 
*             1.过滤掉没用的Change
*             2.将change提交到给它所属的事务
*			  2.1	获取Change的TransactionId属性
*             2.2	如果是事务开始型Change，则根据TransactionId创建空事务
*             2.3   根据TransactionId从TransactionMgr中获取事务
*             2.4	将Change提交给事务
*/

//constructor and desctructor

/*
*构造函数
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
*析构函数
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
*处理Change
*/
int LGC_ChangeHandler::handle()
{
	int iFuncRet = 0;
	
	//过滤掉没必要解析的Change
	if( true == m_pChange->isNeedAddToTrsct() ){
		//将chang提交给事务
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
*将change添加到事务中
*/
int LGC_ChangeHandler::addChangeToTrsct()
{
	int iFuncRet = 0;

	const unsigned short threadId = m_pChange->getThreadId();
	const LGC_TransactionId transactionId = m_pChange->getTransactionId();
    const bool isBeginTrsctChange = LGC_ChangeHandler::isBeginTrsctChange(m_pChange);
	
	//获取TransactionMgr, 每个抽取线程都有自己的TransactionMgr
	LGC_TransactionMgr* pTransactionMgr = LGC_TransactionMgr::getInstance(threadId);
	
fprintf(stdout, "%s \n", m_pChange->toString().data());
	
	//如果新开始一个事务，则添加到事务管理器中
	if(isBeginTrsctChange){
		iFuncRet = pTransactionMgr->addTransaction(transactionId);
		if(iFuncRet < 0){
			lgc_errMsg("addTransaction failed \n");
			//return -1;
		}
	}
	
	//从事务管理器中获取事务
	LGC_Transaction* pTransaction = pTransactionMgr->getTransaction(transactionId);
	if(pTransaction == NULL){//not find
		return 0; //ignore the change
	}
	
	//向相应的事务中添加change
	iFuncRet = pTransaction->addChange(&m_pChange);
	if(iFuncRet != 1 || m_pChange != NULL ){
		lgc_errMsg("add change to trsact failed \n");
		return -1;
	}
	
	//如果事务已经结束，则处理它
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
*change类型是否是事务开始change
*/
bool LGC_ChangeHandler::isBeginTrsctChange(LGC_Change* pChange)
{
	return pChange->isBeginTrsctChange();
}

