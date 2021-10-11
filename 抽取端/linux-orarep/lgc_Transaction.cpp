#include "lgc_Transaction.h"
#include "lgc_Change.h"
#include "lgc_DmlChangeList.h"
#include "lgc_DmlRowsOutput.h"

#include "lgc_TransactionMgr.h"
#include "lgc_api.h"

//constructor and desctructor 

/*
*���캯��
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
*��������
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
*��change��ӵ�������
*�ɹ��򷵻�-1 �� *ppChange == NULL
*/
int LGC_Transaction::addChange(LGC_Change** ppChange)
{
	int iFuncRet = 0;

	if(*ppChange == NULL){
		lgc_errMsg("check failed \n");
		return -1;
	}

	if((*ppChange)->isBeginTrsctChange()){//����ʼ��Change
		
		//��������ʼ��change
		if( this->handleBeginTChange((LGC_BeginTChange*)*ppChange) <0){
			lgc_errMsg("handleBeginTChange failed \n");
			return -1;
		}
		LGC_Change::freeChange(*ppChange);
		*ppChange = NULL;

	}else if((*ppChange)->isEndTrsctChange()){//���������change
		
		//�������������change
		if(this->handleEndTChange((LGC_EndTChange*)*ppChange) < 0){
			lgc_errMsg("hanleEndTChange failed \n");
			return -1;
		}
		LGC_Change::freeChange(*ppChange);
		*ppChange = NULL;

	}else{//��ͨdml��change
		
		//��change��ӵ�dmlChangeList
		iFuncRet = this->addChangeToDmlChangeList(ppChange);
		if(iFuncRet != 1 || *ppChange != NULL){
			lgc_errMsg("addChangeToDmlChangeList failed \n");
			return -1;
		}
		
		//���dmlChangeList�����ˣ�������
		if( this->dmlChangeListIsEnd() ){
			//����dmlChangeList
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
*��������ʼ��change
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
	
	//�����Ѿ���ʼ��־��
	m_isTrsctBegined = true;

	//�������ʼ��ص����ԵĺϷ���
	if(!this->checkTrsctBegin()){
		lgc_errMsg("checkTrsctBegin failed \n");
		return -1;
	}

	return 0;
}

/*
*�������������change
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
	
	//��־�����Ѿ�����
	m_isTrsctEnd = true;
	//��־�����Ƿ�ع�
	m_isTrsctRollback = pEndTChange->isTrsctRollback();
	
	//����������������ԵĺϷ���
	if(!this->checkTrsctEnd()){
		lgc_errMsg("checkTrsctEnd failed \n");
		return -1;
	}
	
	return 0;
}

/*
*���Change��DmlChangeList
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
*dmlChangeList�Ƿ����
*/
bool LGC_Transaction::dmlChangeListIsEnd()
{
	return m_pDmlChangeList->isEnd();
}

/*
*����dmlChangeList
*/
int  LGC_Transaction::handleDmlChangeList()
{
	//���˵�����Ҫ������dmlChangeList
	if(m_pDmlChangeList->isNeedAnalyse()){
		//����dmlChangeList, ����dmlData
		if( m_pDmlChangeList->generateDmlData(m_pDmlRowsOutput) < 0){
			lgc_errMsg("generateDmlData failed \n");
			return -1;
		}
	}

	return 0;
}

/*
*���DmlChangeList
*/
void LGC_Transaction::clearDmlChangeList()
{
	m_pDmlChangeList->clear();
}

/*
*�������ʼ������ԵĺϷ���
*/
bool LGC_Transaction::checkTrsctBegin()
{
	return (m_trsctBegin.xidSlot     == m_transactionId.xidSlot 
		    && m_trsctBegin.xidSqn   == m_transactionId.xidSqn
			&& m_trsctBegin.threadId == m_threadId
		    && m_isTrsctBegined      == true);
}

/*
*����������������ԵĺϷ���
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
*�����Ƿ��Ѿ���ʼ
*/
bool LGC_Transaction::trsctBegined()
{
	return m_isTrsctBegined;
}

/*
*�������ܹ��ж���������
*/
unsigned int LGC_Transaction::getDmls()
{
	return 0;
}

/*
*���������������ݵ����ݳ���
*/
unsigned int LGC_Transaction::getLenOfDmlRows()
{
	return 0;
}


/*
*�����Ƿ����
*/
bool LGC_Transaction::isTrsctEnd()
{
	return m_isTrsctEnd;
}

/*
*�����Ƿ���Ҫ����
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
*��������Id��С�ڷ���
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
*��������Id�ĵ��ڷ�
*/
bool operator ==(const LGC_TransactionId& left, const LGC_TransactionId& right)
{
	return (left.xidSlot == right.xidSlot && left.xidSqn == right.xidSqn);
}

