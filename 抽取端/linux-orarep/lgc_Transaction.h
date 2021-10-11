#ifndef LGC_TRANSACTION_H
#define LGC_TRANSACTION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <algorithm>
#include <list>
using namespace std;

#include "lgc_Defines.h"
#include "lgc_Structure.h"
#include "lgc_Change.h"

class LGC_Change;
class LGC_DmlChangeList;
class LGC_DmlRowsOutput;
class LGC_DmlRow;

bool operator <(const LGC_TransactionId& left, const LGC_TransactionId& right);
bool operator ==(const LGC_TransactionId& left, const LGC_TransactionId& right);

class LGC_Transaction
{
private:
	//member variables
	unsigned short m_threadId;
	LGC_TransactionId m_transactionId;
	dml_trsct_begin m_trsctBegin;
	dml_trsct_end   m_trsctEnd;

	LGC_DmlChangeList* m_pDmlChangeList;
	LGC_DmlRowsOutput* m_pDmlRowsOutput;

	bool m_isTrsctBegined;
	bool m_isTrsctEnd;
	bool m_isTrsctRollback;

public:
	//constructor and desctructor
	LGC_Transaction(const unsigned short threadId, const LGC_TransactionId& transactionId);
	~LGC_Transaction();

public:
	//public member functions 
	int addChange(LGC_Change** ppChange);

//	void delFromTrsctHashtable();

private:
	int handleBeginTChange(LGC_BeginTChange* pBeginTChange);
	int handleEndTChange(LGC_EndTChange* pEndTChange);
	int addChangeToDmlChangeList(LGC_Change** ppChange);

	bool  dmlChangeListIsEnd();
	int  handleDmlChangeList();
	void clearDmlChangeList();
	
	bool checkTrsctBegin();
	bool checkTrsctEnd();

private:
	//some get or set properties functions
	bool trsctBegined();
	unsigned int getDmls();
	unsigned int getLenOfDmlRows();

public:
	bool isTrsctEnd();
	bool isNeedApply();

	inline const dml_trsct_begin* getTrsctBegin() const
	{
		return &m_trsctBegin;
	}
	inline const dml_trsct_end* getTrsctEnd() const
	{
		return &m_trsctEnd;
	}
	inline BYTE8 getCommitSCN() const
	{
		return m_trsctEnd.commitSCN;
	}
	inline const short getThreadId() const
	{
		return m_threadId;
	}
	inline const LGC_TransactionId getTransactionId() const
	{
		return m_transactionId;
	}


	const list<LGC_DmlRow*>* getDmlRowList() const;

	
};
#endif
