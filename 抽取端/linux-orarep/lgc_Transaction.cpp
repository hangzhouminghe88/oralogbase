#include "lgc_Transaction.h"
#include "lgc_Change.h"
#include "lgc_DmlChangeList.h"
#include "lgc_DmlRowsOutput.h"

#include "lgc_TransactionMgr.h"
#include "lgc_api.h"

//constructor and desctructor 

/*
*构造函数
*/
LGC_Transaction::LGC_Transaction(const unsigned short threadId, const LGC_TransactionId& transactionId)
{
	m_threadId = threadId;
	m_transactionId = transactionId;

	memset(&m_trsctBegin,0,sizeof(m_trsctBegin));
	memset(&m_trsctEnd,0,sizeof(m_trsctEnd));

	m_pDmlChangeList = new LGC_DmlChangeList(this);
	m_pDmlRowsOutput = new LGC_DmlRowsOutput(this);

	if(m_pDmlChangeList == NULL 
		|| m_pDmlRowsOutput == NULL)
	{
		lgc_errMsg("check failed \n");
		exit(1);
	}

	m_isTrsctBegined = false;
	m_isTrsctEnd = false;
	m_isTrsctRollback = false;

	return;
}

/*
*析构函数
*/
LGC_Transaction::~LGC_Transaction()
{
	if(m_pDmlChangeList){
		delete m_pDmlChangeList;
		m_pDmlChangeList = NULL;
	}

	if(m_pDmlRowsOutput){
		delete m_pDmlRowsOutput;
		m_pDmlRowsOutput = NULL;
	}

	return;
}

//public member functions 

/*
*将change添加到事务中
*成功则返回-1 且 *ppChange == NULL
*/
int LGC_Transaction::addChange(LGC_Change** ppChange)
{
	int iFuncRet = 0;

	if(*ppChange == NULL){
		lgc_errMsg("check failed \n");
		return -1;
	}

	if((*ppChange)->isBeginTrsctChange()){//事务开始型Change
		
		//处理事务开始型change
		if( this->handleBeginTChange((LGC_BeginTChange*)*ppChange) <0){
			lgc_errMsg("handleBeginTChange failed \n");
			return -1;
		}
		LGC_Change::freeChange(*ppChange);
		*ppChange = NULL;

	}else if((*ppChange)->isEndTrsctChange()){//事务结束型change
		
		//处理事务结束型change
		if(this->handleEndTChange((LGC_EndTChange*)*ppChange) < 0){
			lgc_errMsg("hanleEndTChange failed \n");
			return -1;
		}
		LGC_Change::freeChange(*ppChange);
		*ppChange = NULL;

	}else{//普通dml型change
		
		//将change添加到dmlChangeList
		iFuncRet = this->addChangeToDmlChangeList(ppChange);
		if(iFuncRet != 1 || *ppChange != NULL){
			lgc_errMsg("addChangeToDmlChangeList failed \n");
			return -1;
		}
		
		//如果dmlChangeList结束了，则处理它
		if( this->dmlChangeListIsEnd() ){
			//处理dmlChangeList
			if ( this->handleDmlChangeList() < 0){
				lgc_errMsg("handleDmlChangeList failed \n");
				return -1;
			}
			this->clearDmlChangeList();
		}
	}

	//success
	if(*ppChange != NULL){
		lgc_errMsg("invalid \n");
		exit(1);
	}

	return 1;
}

//private member functions

/*
*处理事务开始型change
*/
int LGC_Transaction::handleBeginTChange(LGC_BeginTChange* pBeginTChange)
{
	const LGC_TransactionId transactionId= pBeginTChange->getTransactionId();

	m_trsctBegin.xidSlot = transactionId.xidSlot;
	m_trsctBegin.xidSqn = transactionId.xidSqn;
	m_trsctBegin.scn = pBeginTChange->getTrsctBeginSCN();
	m_trsctBegin.threadId = pBeginTChange->getThreadId();
	m_trsctBegin.len = sizeof(m_trsctBegin);
	m_trsctBegin.trsctLen = 0;
	
	//事务已经开始标志它
	m_isTrsctBegined = true;

	//检查事务开始相关的属性的合法性
	if(!this->checkTrsctBegin()){
		lgc_errMsg("checkTrsctBegin failed \n");
		return -1;
	}

	return 0;
}

/*
*处理事务结束型change
*/
int LGC_Transaction::handleEndTChange(LGC_EndTChange* pEndTChange)
{
	const LGC_TransactionId transactionId = pEndTChange->getTransactionId();

	m_trsctEnd.xidSlot = transactionId.xidSlot;
	m_trsctEnd.xidSqn = transactionId.xidSqn;
	m_trsctEnd.scn = pEndTChange->getCommitSCN();
	m_trsctEnd.commitSCN = pEndTChange->getCommitSCN();
	m_trsctEnd.threadId = pEndTChange->getThreadId();
	m_trsctEnd.len = sizeof(m_trsctEnd);
	m_trsctEnd.trsctLen = m_trsctBegin.len + this->getLenOfDmlRows() + m_trsctEnd.len; 
	m_trsctEnd.dmls = this->getDmls();
	
	m_trsctBegin.trsctLen = m_trsctEnd.trsctLen;
	m_trsctBegin.dmls = m_trsctEnd.dmls;
	
	//标志事务已经结束
	m_isTrsctEnd = true;
	//标志事务是否回滚
	m_isTrsctRollback = pEndTChange->isTrsctRollback();
	
	//检查事务结束相关属性的合法性
	if(!this->checkTrsctEnd()){
		lgc_errMsg("checkTrsctEnd failed \n");
		return -1;
	}
	
	return 0;
}

/*
*添加Change到DmlChangeList
*/
int LGC_Transaction::addChangeToDmlChangeList(LGC_Change** ppChange)
{
	if(!lgc_check(this->trsctBegined() == true
		          && this->isTrsctEnd() == false))
	{
		lgc_errMsg("trsct status invalid \n");
		return -1;
	}
	
	m_pDmlChangeList->addChange(*ppChange);
	*ppChange = NULL;

	return 1;
}


/*
*dmlChangeList是否结束
*/
bool LGC_Transaction::dmlChangeListIsEnd()
{
	return m_pDmlChangeList->isEnd();
}

/*
*处理dmlChangeList
*/
int  LGC_Transaction::handleDmlChangeList()
{
	//过滤掉不需要解析的dmlChangeList
	if(m_pDmlChangeList->isNeedAnalyse()){
		//解析dmlChangeList, 生成dmlData
		if( m_pDmlChangeList->generateDmlData(m_pDmlRowsOutput) < 0){
			lgc_errMsg("generateDmlData failed \n");
			return -1;
		}
	}

	return 0;
}

/*
*清空DmlChangeList
*/
void LGC_Transaction::clearDmlChangeList()
{
	m_pDmlChangeList->clear();
}

/*
*检查事务开始相关属性的合法性
*/
bool LGC_Transaction::checkTrsctBegin()
{
	return (m_trsctBegin.xidSlot     == m_transactionId.xidSlot 
		    && m_trsctBegin.xidSqn   == m_transactionId.xidSqn
			&& m_trsctBegin.threadId == m_threadId
		    && m_isTrsctBegined      == true);
}

/*
*检查事务结束相关属性的合法性
*/
bool LGC_Transaction::checkTrsctEnd()
{
	return (m_trsctEnd.xidSlot      == m_trsctBegin.xidSlot 
		     && m_trsctEnd.xidSqn   == m_trsctBegin.xidSqn 
		     && m_trsctEnd.threadId == m_trsctBegin.threadId
			 && m_isTrsctBegined    == true
		     && m_isTrsctEnd        == true);
}


//some get or set properties functions

/*
*事务是否已经开始
*/
bool LGC_Transaction::trsctBegined()
{
	return m_isTrsctBegined;
}

/*
*事务中总共有多少行数据
*/
unsigned int LGC_Transaction::getDmls()
{
	return 0;
}

/*
*事务中所有行数据的数据长度
*/
unsigned int LGC_Transaction::getLenOfDmlRows()
{
	return 0;
}


/*
*事务是否结束
*/
bool LGC_Transaction::isTrsctEnd()
{
	return m_isTrsctEnd;
}

/*
*事务是否需要解析
*/
bool LGC_Transaction::isNeedApply()
{
	return !m_isTrsctRollback && m_pDmlRowsOutput->getRowCount() > 0;
}

const list<LGC_DmlRow*>* 
LGC_Transaction::getDmlRowList() const
{
	return m_pDmlRowsOutput->getDmlRowList();
}
/*
*重载事务Id的小于符号
*/
bool operator <(const LGC_TransactionId& left, const LGC_TransactionId& right)
{
	if(left.xidSlot < right.xidSlot){
		return true;
	}else if(left.xidSlot == right.xidSlot){
		return (left.xidSqn < right.xidSqn);
	}else{
		return false;
	}
}

/*
*重载事务Id的等于符
*/
bool operator ==(const LGC_TransactionId& left, const LGC_TransactionId& right)
{
	return (left.xidSlot == right.xidSlot && left.xidSqn == right.xidSqn);
}

