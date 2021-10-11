#ifndef _LGC_READREDOFILE_H
#define _LGC_READREDOFILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>
#include <sstream>
#include <list>
using namespace std;

#include "Defines.h"
#include "lgc_Structure.h"
#include "OciQuery.h"
#include "tw_api.h"
#include "tw_rwDataBlocks.h"


//////////////////////////////LGC_ReadRedoFile/////////////////////////
//this is a supper class, it should never be instantiated
class LGC_ReadRedoFile
{
protected:
	enum ERRNO {ERR_OK=0,ERR_FATAL,ERR_HEADER_SEQ_HIGHER=1, ERR_HEADER_SEQ_LOWER=2, ERR_SEQ_HIGHER};
protected:
	OciQuery* m_pQuery;
	OciQuery* m_pASMQuery;
	LGC_RedoFileInfo m_redoFileInfo;
	RWDataBlocks* m_pRWDataBlocks;
	
	char* m_fileContent;
	DWORD m_validBlks;

	log_file_header m_logFileHeader;
	log_redo_header m_logRedoHeader;
	ERRNO m_errNo;

public:
	LGC_ReadRedoFile(OciQuery* pQuery);
	virtual ~LGC_ReadRedoFile();

	virtual bool openRedoFile(const LGC_RedoFileInfo& redoFileInfo)=0;
	virtual int  readBlks(char* buf, int blkOffOfRead, int blksToRead);

protected:
	 bool readAllRedoBlk();

//inline public
public:
	DWORD getTotalBlks(){
		return m_redoFileInfo.totalBlks;
	}

public:
	//public static functions
	static LGC_ReadRedoFile* createReadRedoFile(const LGC_RedoFileInfo& redoFileInfo, OciQuery* pNormalQuery);
};

/////////////////////////////LGC_ReadArchiveFile///////////////////////
class LGC_ReadArchiveFile:public LGC_ReadRedoFile
{
protected:

public:
	LGC_ReadArchiveFile(OciQuery* pQuery);
	virtual ~LGC_ReadArchiveFile();

	virtual bool openRedoFile(const LGC_RedoFileInfo& redoFileInfo);
	virtual int readBlks(char* buf, int blkOffOfRead, int blksToRead);
};

///////////////////////////LGC_ReadDeactiveOnlineFile//////////////////
class LGC_ReadDeactiveOnlineFile:public LGC_ReadRedoFile
{
protected:
	LGC_ReadRedoFile* m_pReadArchiveFile;
public:
	LGC_ReadDeactiveOnlineFile(OciQuery* pQuery);
	virtual ~LGC_ReadDeactiveOnlineFile();

	virtual bool openRedoFile(const LGC_RedoFileInfo& redoFileInfo);
	virtual int  readBlks(char* buf, int blkOffOfRead, int blksToRead);

protected:
	bool readAllRedoBlk();
	bool handleSeqHigher();
};

/////////////////////////////LGC_ReadCurrentOnlineFile/////////////////
class LGC_ReadCurrentOnlineFile:public LGC_ReadRedoFile
{
protected:
	LGC_ReadRedoFile* m_pReadNotCurrentOnlineFile;
public:
	LGC_ReadCurrentOnlineFile(OciQuery* pQuery);
	virtual ~LGC_ReadCurrentOnlineFile();
	
	virtual bool openRedoFile(const LGC_RedoFileInfo& redoFileInfo);
	virtual int readBlks(char* buf, int blkOffOfRead, int blksToRead);

protected:
	bool readAllRedoBlk();
	bool handleSeqHigher();

	bool isArchived();
	bool handleArchived();
};

#endif

