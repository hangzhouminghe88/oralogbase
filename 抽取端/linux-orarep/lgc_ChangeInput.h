#ifndef LGC_CHANGEINPUT_H
#define LGC_CHANGEINPUT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <list>

#include "lgc_Defines.h"
#include "lgc_Structure.h"
#include "lgc_api.h"

using namespace std;

class LGC_RedoRecord;
class LGC_Change;

class LGC_ChangeInput
{
private:
	//member variables
	unsigned int m_changeNo;
	LGC_RedoRecord* m_pRedoRecord;
	list<LGC_Change*> m_changeList;

private:
	//constructor and desctructor
	LGC_ChangeInput(LGC_RedoRecord* pRecord);
public:
	~LGC_ChangeInput();

public:
	//public member functions
	int getNextChange(LGC_Change** ppChange);

private:
	//private member functions
	bool generateChangeList();
	bool generateUndoReferences();
	bool initChangesInChangeList();

	int readChangeFromRecord(LGC_Change** ppChange);
	int readChangeHeaderFromRecord(void* changeHeader, const unsigned int changeHeaderLen);
	int readChangeLenArrayFromRecord(unsigned short** pLenArray, unsigned short* pLenOfChangeLenArray);
	int readDataArrayFromRecord(const unsigned short* lenArray,const unsigned short* lenArrayAlign,
		                       const unsigned short lenOfChangeLenArray, char** dataArray);
	void addChangeToList(const LGC_Change* pChange);
	LGC_Change* popFromChangeList();

public:
	//static member functions
	static LGC_ChangeInput* createChangeInput(LGC_RedoRecord* pRedoRecord);
private:
	static unsigned short* createAlignChangeLenArray(const unsigned short* lenArray, const unsigned short lenOfChangeLenArray);
	static char** createDataArray(const unsigned short* lenArray, const unsigned short* lenArrayAlign, const unsigned short lenOfChangeLenArray);
	static void freeDataArray(const unsigned short* lenArray, const unsigned short* lenArrayAlign, const unsigned short lenOfChangeLenArray, char** dataArray);
	static unsigned short getAlignValue(const unsigned short value);
	
};
#endif
