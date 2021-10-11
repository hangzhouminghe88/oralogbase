#ifndef LGC_DMLCHANGELIST_H
#define LGC_DMLCHANGELIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <list>
using namespace std;

#include "lgc_Defines.h"
#include "lgc_Structure.h"



class LGC_Change;
class LGC_Transaction;
class LGC_DmlRowsOutput;

class LGC_DmlChangeList
{
private:
	//private member variables
	LGC_Transaction* m_pTrsct;
	list<LGC_Change*> m_undoChange_list;
	list<LGC_Change*> m_redoChange_list;

	dml_header m_dmlHeader;
	unsigned int m_objNum;

	bool m_isDmlEnd;

public:
	//constructor and desctructor
	LGC_DmlChangeList(LGC_Transaction* pTransaction);
	~LGC_DmlChangeList();

public:
	//public member functions
	void addChange(LGC_Change* pChange);
	int generateDmlData(LGC_DmlRowsOutput* pDmlRowsOutput);

private:
	//private member functions
	void addUndoChange(LGC_Change* pChange);
	void addRedoChange(LGC_Change* pChange);

	int generateDmlHeaderData(LGC_DmlRowsOutput* pDmlRowsOutput);
	int generateDmlRowsData(LGC_DmlRowsOutput* pDmlRowsOutput);
	int generateDmlEndData(LGC_DmlRowsOutput* pDmlRowsOutput);

	int generateDeleteDmlRowData(LGC_DmlRowsOutput* pDmlRowsOutput);
	int generateInsertDmlRowData(LGC_DmlRowsOutput* pDmlRowsOutput);
	int generateUpdateDmlRowData(LGC_DmlRowsOutput* pDmlRowsOutput);
	int generateMultiInsertDmlRowsData(LGC_DmlRowsOutput* pDmlRowsOutput);

	unsigned char calculateDmlType();
	unsigned int  calculateObjNum();
	bool checkChangeList();

	int prepareForGenDmlData();

public:
	//some get or set properties functions
	bool isEnd();
	bool isNeedAnalyse();
	void clear();
	
};
#endif
