#ifndef LGC_RECORDINPUT_H
#define LGC_RECORDINPUT_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <list>

#include "lgc_Defines.h"
#include "lgc_Structure.h"

class LGC_RedoFile;
class LGC_RedoRecord;

class LGC_RedoRecordInput
{
private:
	//member variables
	LGC_RedoFile* m_pRedoFile;
	LGC_RedoRecord* m_pFirstRedoRecord;
	LGC_RedoRecord* m_pLastIncompRedoRecord;
private:
	//constructor and desctructor
	LGC_RedoRecordInput(LGC_RedoFile* pRedoFile);
public:
	~LGC_RedoRecordInput();

public:
	//public member functions
	int getNextRedoRecord(LGC_RedoRecord** ppRecord);
private:
	//private member functions
	int readNextRedoRecordFromFile(LGC_RedoRecord** pRedoRecord);
	int readRedoRecordHeaderFromFile(void* recordHeaderBuf, unsigned int* pRecordHeaderLen);
	int readRedoRecordBody(const log_record_header_minor* pRecordHeader, int recordHeaderLen, char** ppRecordBody, unsigned int* pRecordBodyLen);

	void saveLastIncompleteRecord(const LGC_RedoRecord* pRedoRecord);
	int  skipToNextRecord();

public:
	//some get or set properties functions
	inline bool 
		isLeftIncompleteRedoRecord() const
	{
		return !(m_pLastIncompRedoRecord == NULL);
	}

	inline LGC_RedoRecord* 
		getLastIncompleteRedoRecord() const
	{
		return m_pLastIncompRedoRecord;
	}

	inline void 
		setFirstRedoRecord(LGC_RedoRecord* pRedoRecord)
	{
		m_pFirstRedoRecord = pRedoRecord;
		return;
	}
	inline void detachLastIncompleteRedoRecord(){
		m_pLastIncompRedoRecord = NULL;
	}


public:
	//static member functions
	static LGC_RedoRecordInput* createRedoRecordInput(LGC_RedoRecordInput* pRedoRecordInput, LGC_RedoFile* pRedoFile);

private:
	static unsigned int calRecordHeaderLen(unsigned char vld);
};
#endif
