#ifndef LGC_TRANSACTIONMGR_H
#define LGC_TRANSACTIONMGR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <map>
using namespace std;

#include "lgc_Defines.h"
#include "lgc_Structure.h"
#include "Mutex.h"

class LGC_Transaction;

class LGC_TransactionMgr
{
private:
	//member variables
	WORD m_threadId;
	DWORD m_trsctCount;
	Mutex m_mutex;
	map<LGC_TransactionId, LGC_Transaction*> m_trsctMap;

private:
	//static member variables
	static LGC_TransactionMgr* s_TrsctMgr_array[3];
	static Mutex s_Mutex_array[3];

private:
	//constructor and destructor
	LGC_TransactionMgr(WORD threadId);
public:
	~LGC_TransactionMgr();

public:
	//static getInstance
	static LGC_TransactionMgr* getInstance(const WORD threadId);

public:
	//public member functions
	LGC_Transaction* getTransaction(const LGC_TransactionId& transactionId);
	void delTransaction(const LGC_TransactionId& transactionId);

	int addTransaction(const LGC_TransactionId& transactionId);
};
#endif
