#include "lgc_TransactionMgr.h"
#include "lgc_Transaction.h"
#include "lgc_api.h"

LGC_TransactionMgr* LGC_TransactionMgr::s_TrsctMgr_array[3] ={0};
Mutex LGC_TransactionMgr::s_Mutex_array[3];

LGC_TransactionMgr::LGC_TransactionMgr(const WORD threadId)
{
	m_threadId = threadId;
	m_trsctCount = 0;

	return;
}

LGC_TransactionMgr::~LGC_TransactionMgr()
{
	return;
}

LGC_TransactionMgr* LGC_TransactionMgr::getInstance(const WORD threadId)
{
	if(threadId > 2){
		exit(1);
	}

	if(s_TrsctMgr_array[threadId] == NULL){
		s_Mutex_array[threadId].Lock();
		if(s_TrsctMgr_array[threadId] == NULL){
			s_TrsctMgr_array[threadId] = new LGC_TransactionMgr(threadId);
			if(s_TrsctMgr_array[threadId] == NULL){
				exit(1);
			}
		}
		s_Mutex_array[threadId].Unlock();
	}
	
	//success
	return s_TrsctMgr_array[threadId];
}


//public member functions
LGC_Transaction* LGC_TransactionMgr::getTransaction(const LGC_TransactionId& transactionId)
{
	LGC_Transaction* pTransaction = NULL;

	map<LGC_TransactionId,LGC_Transaction*>::iterator it;

	it = m_trsctMap.find(transactionId);
	if(it == m_trsctMap.end()){//not found
		pTransaction =  NULL;
	}else{
		pTransaction = it->second;
		if(pTransaction == NULL){
			exit(1);//fatal error
		}
	}

	//success
	return pTransaction;
}

void LGC_TransactionMgr::delTransaction(const LGC_TransactionId& transactionId)
{
	int elemsRemoved = m_trsctMap.erase(transactionId);
	if(elemsRemoved != 1){//remove failed 
		exit(1);
	}

	//success
	return;
}


int LGC_TransactionMgr::addTransaction(const LGC_TransactionId& transactionId)
{
	map<LGC_TransactionId,LGC_Transaction*>::iterator it;

	it = m_trsctMap.find(transactionId);

	if(it != m_trsctMap.end()){//found but should not 
		//fatal error
		lgc_errMsg("found but should not");
		return -1;
	}

	LGC_Transaction* pTransaction  = new LGC_Transaction(m_threadId, transactionId);
	if(pTransaction == NULL){
		exit(1);
	}

	pair<map<LGC_TransactionId, LGC_Transaction*>::iterator, bool> mapInsertRet;
	mapInsertRet = m_trsctMap.insert(make_pair(transactionId, pTransaction));
	if(mapInsertRet.second == false){//insert failed
		//fatal error
		lgc_errMsg("insert failed \n");
		exit(1);
	}

	//update statistics
	++m_trsctCount;
	
	//success
	return 1;

}

