#ifndef LGC_DMLROWSOUTPUT_H
#define LGC_DMLROWSOUTPUT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <list>
using namespace std;

#include "lgc_Defines.h"
#include "lgc_Structure.h"

class LGC_Transaction;
class LGC_DmlRow;

class LGC_DmlRowsOutput
{
private:
	enum StatusType{
		DML_NOTSTART=1,
		DML_START,
		DML_REDOCOLUMNS,
		DML_UNDOCOLUMNS,
		DML_END
	};
private:
	//member variables
	StatusType m_status;
	LGC_Transaction* m_pTrsct;
	list<LGC_DmlRow*> m_dmlRow_list;
	unsigned int m_rowCount;
	LGC_DmlRow* m_pCurDmlRow;

public:
	LGC_DmlRowsOutput(LGC_Transaction* pTrsct);
	~LGC_DmlRowsOutput();

public:
	//public member functions
	int writeDmlHeader(const void* dmlHeader, const unsigned int dmlHeaderLen);
	int writeDmlEndInfo(const void* dmlEndInfo, const unsigned int dmlEndInfoLen);
	int writeOneColumn(const void* colData, const unsigned int colLen, const unsigned short colNo, const bool isFinish);
	void switchToRedo();
	void switchToUndo();

private:
	//private member functions	
	int  writeOneRedoColumn(const void* colData, const unsigned int colLen, const unsigned short colNo, const bool isFinish);
	int  writeOneUndoColumn(const void* colData, const unsigned int colLen, const unsigned short colNo, const bool isFinish);
	void updateStatus(StatusType status);
	void addDmlRow(LGC_DmlRow* pDmlRow);

public:
	//some get or set properties
	dml_header getCurDmlHeader();
	inline const list<LGC_DmlRow*>* getDmlRowList() const
	{
		return &m_dmlRow_list;
	}
	inline unsigned int getRowCount() const
	{
		return m_rowCount;
	}


};
#endif
