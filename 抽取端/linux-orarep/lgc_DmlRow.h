#ifndef LGC_DMLROW_H
#define LGC_DMLROW_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <list>
using namespace std;

#include "lgc_Defines.h"
#include "lgc_Structure.h"

class LGC_DmlColumn;

class LGC_DmlRow
{
private:
	//member variables
	dml_header m_dmlHeader;
	list<LGC_DmlColumn*> m_redoColList;
	list<LGC_DmlColumn*> m_undoColList;

	LGC_DmlColumn* m_pCurDmlCol;

private:
	//静态成员变量
	static BYTE8 s_createTimes;
	static BYTE8 s_freeTimes;
public:
	//constructor and desctructor
	LGC_DmlRow(const dml_header* pDmlHeader);
	~LGC_DmlRow();

public:
	//public member functions
	int writeRedoColumn(const void* colData, const unsigned int colLen, const unsigned short colNo, const bool isFinish);
	int writeUndoColumn(const void* colData, const unsigned int colLen, const unsigned short colNo, const bool isFinish);
	
	int handleDmlRowEnd();

private:
	//private member functions
	LGC_DmlColumn* getCurDmlCol();
	void detachCurDmlCol();
	void addToRedoColList(LGC_DmlColumn* pDmlCol);
	void addToUndoColList(LGC_DmlColumn* pDmlCol);
	bool checkColNoOrder();

public:
	//some get or set functions
	inline const dml_header& getDmlHeader() const{
		return m_dmlHeader;
	}
	inline const list<LGC_DmlColumn*>& getRedoColList() const
	{
		return m_redoColList;
	}
	inline const list<LGC_DmlColumn*>& getUndoColList() const
	{
		return m_undoColList;
	}

};

class LGC_DmlColumn
{
private:
	//member variables
	dml_oneColumnData_header m_colHeader;
	char* m_colData;
public:
	//constructor and destructor
	LGC_DmlColumn();
	~LGC_DmlColumn();

public:
	//public meber functions
	int writeColData(const void* colData, const unsigned int colLen, const unsigned short colNo);

private:
	//private member functions
	bool checkColNo(const unsigned short colNo);
	int  writeColData(const void* colData, const unsigned int colLen);
	void setColNo(const unsigned short colNo);
public:
	//some get or set functions
	inline unsigned short getColDataLen() const{
		return m_colHeader.colLen;
	}

	inline unsigned short getColNo() const
	{
		return m_colHeader.colNo;
	}

	inline const char* getColData() const
	{
		return m_colData;
	}
};

#endif

