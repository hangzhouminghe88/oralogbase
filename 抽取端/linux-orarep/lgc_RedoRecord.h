#ifndef LGC_REDORECORD_H
#define LGC_REDORECORD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "lgc_Defines.h"
#include "lgc_Structure.h"

using namespace std;

class LGC_RedoFile;
struct LGC_IncompleteRedoRecordData;


class LGC_RedoRecord
{
private:
	//member variables
	LGC_RedoFile* m_pRedoFile;
	
	RBA m_rba;
	log_record_header_major m_recordHeader;
	unsigned int m_recordHeaderLen;

	char* m_recordBody;
	unsigned int m_recordBodyLen;
	char* m_recordBodyRdPos;
	
	//use for dump member variables 
	char* m_dumpText;
private:
	//constructor and desctructor
	LGC_RedoRecord(LGC_RedoFile* pRedoFile, const RBA& rba, const void* recordHeader, unsigned int recordHeaderLen, const void* recordBody, unsigned int recordBodyLen);
public:
	~LGC_RedoRecord();

public:
	//public member variables
	int loadLeftRedoRecordData(LGC_IncompleteRedoRecordData* pIncompleteRedoRecordData);

	//interface for change read data from record body
	int readChangeHeader(void* changeHeader, unsigned int size);
	int tryReadLenOfChangeLenArray(void* lenOfChangeArray, unsigned int size);
	int readChangeLenArray(void* changeLenArray, unsigned int size);
	int readChangeData(void* changeData, unsigned int size);

private:
	//private member functions
	int read(void* buf, unsigned int size);
	int tryRead(void* buf, unsigned int size) const;
public:
	//some get or set properties functions
	unsigned int getThreadId() const;
	unsigned int getRecordLen() const;
	unsigned int getRecordHeaderLen() const;
	unsigned int getRecordBodyLen() const;
	bool recordBodyFinish() const;
	BYTE8 getRecordSCN() const;

	inline RBA getRBA() const
	{
		return m_rba;
	}

private:
	char* getClearedDumpTextBuf();


public:
	//functions that convert properties to string 
	const char* recordHeaderToString();
	const char* recordBodyToString();
	const char* toString();
public:
	//public static functions
	static LGC_RedoRecord* createRedoRecord(LGC_RedoFile* pRedoFile, const RBA& rba,const void* recordHeader, unsigned int recordHeaderLen, 
		                                    const void* recordBody, unsigned int recordBodyLen);

};
#endif
