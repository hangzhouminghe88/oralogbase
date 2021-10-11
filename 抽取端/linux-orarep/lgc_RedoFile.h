#ifndef LGC_REDOFILE_H
#define LGC_REDOFILE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "lgc_Defines.h"
#include "lgc_Structure.h"
#include "lgc_api.h"

class LGC_RedoFile;
class LGC_RedoBlkOp;
class LGC_IncompleteRedoRecordData;
class LGC_ReadRedoFile;


class LGC_RedoFile
{
private:
	//member variables
	LGC_RedoFileInfo m_redoFileInfo;
	LGC_RedoBlkOp*   m_pRedoBlkOp;
	LGC_IncompleteRedoRecordData* m_pFirstIncompleteRecordData;

private:
	//constructor and desctructor 
	LGC_RedoFile(const LGC_RedoFileInfo& redoFileInfo, LGC_RedoBlkOp* pRedoBlkOp );
public:
	~LGC_RedoFile();

public:
	//public member functions
	int tryReadRecordHeader(void* recordHeader, int size);
	int readRecordHeader(void* recordHeader, int size);
	int readRecordBody(void* recordBody, int size);
	int skipToNextRecordPos();

private:
	//protected member functions
	
	//some init member functions
	int loadRedoFileHeader();
	int loadRedoHeader();
	int skipToFirstRecordPos();

	
	//private member tool functions 
	bool isNeedSkipBlock();
	int  skipToNextBlock();

public:
	//some get or set properties functions
	LGC_IncompleteRedoRecordData*	getFirstIncompleteRedoRecordData() const;
	const RBA						getCurRBA() const;
	const unsigned int				getThreadId() const;

public:
	//static member functions
	static LGC_RedoFile* createRedoFile(const LGC_RedoFileInfo& redoFileInfo,OciQuery* pNormalQuery);
};

//.........................................
//tool classes for LGC_RedoFile
//.........................................


//.......................................
//struct LGC_IncompleteRedoRecordData
//.......................................

struct LGC_IncompleteRedoRecordData
{
private:
	char* data;
	DWORD dataLen;
	LGC_RedoFile* pRedoFile;

public:
	LGC_IncompleteRedoRecordData(){
		data=NULL;
		dataLen = 0;
		pRedoFile = NULL;
	}
	~LGC_IncompleteRedoRecordData(){
		if(data){
			delete[] data;
		}
	}

	int write(void* buf, unsigned int size){
		return 0;
	}
	
	char* getDataBuf(){
		return data;
	}
	inline unsigned int getDataLen(){
		return dataLen;
	}
	inline LGC_RedoFile* getRedoFile(){
		return pRedoFile;
	}
};

//.........................................
//class LGC_RedoBlkOp
//.........................................

class LGC_RedoBlkOp
{
private:
	//member variables
	LGC_RedoFileInfo m_redoFileInfo;
	LGC_ReadRedoFile* m_pReadRedoFile;
	

	char* m_blkBuf;
	char* m_blkBufRdPos;
	unsigned int m_blkNo;

private:
	//constructor and desctructor
	LGC_RedoBlkOp(const LGC_RedoFileInfo& redoFileInfo, LGC_ReadRedoFile* pReadRedoFile);
public:
	~LGC_RedoBlkOp();

public:
	//public member functions
	int readDataNoSkipHd(void* buf, unsigned int size);
	int readData(void* buf, unsigned int size);
	int skipSomeData(unsigned int size);
	int tryReadRecordHeader(void *buf, unsigned int size);

private:
	//private member functions
	int init();

	int loadNextBlk();
	int readDataFromCurBlk(void* buf, unsigned int size);
	int readOneBlkFromFile(void* buf, unsigned int blkNo);

public:
	//some get or set properties functions
	unsigned int getBlkNo();
	unsigned int getSequence();
	unsigned int getBlkSize();
	unsigned int getBlkHeaderSize();
	unsigned int getBlkBodySize();

	unsigned int getFirstRecordOff();
	unsigned int getCurOffsetInBlk();
	unsigned int getLeftBytesInBlk();

	const RBA getCurRBA();
	bool  fileIsEnd();
private:
	void setFileEnd();

public:
	//static member functions
	static LGC_RedoBlkOp* createRedoBlkOp(const LGC_RedoFileInfo& redoFileInfo,OciQuery* pNormalQuery);
};

#endif
