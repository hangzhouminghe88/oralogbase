#ifndef LGC_PARAM_H
#define LGC_PARAM_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "Defines.h"
#include "Mutex.h"
#include "SimpleIni.h"


class LGC_Param
{
private:
	CSimpleIniA m_ini;
	char* m_fileName;
	
	char* m_archPathArray[10];
	DWORD m_maxLenOfMediaFile;

	static Mutex s_mutex;
	static LGC_Param* s_instance;

private:
	LGC_Param();//singleton mode

public:
	static LGC_Param* getInstance();
	~LGC_Param();

	bool loadParamFile(const char* fileName);

	const char* getLogDir();

	const char* getTableList();
	const char* getExcludeTableList();

	const char* getOciUser();
	const char* getOciPasswd();
	const char* getOciSid();

	const char* getASMUser();
	const char* getASMPasswd();
	const char* getASMSid();
	const WORD  getThreadCount();
	const WORD  getThreadId();
	
	bool isASM();

	const char* getCkptFileName();
	const char* getMediaFileDir();
	DWORD getMaxLenOfMediaFile();

	BYTE8 getStartSCNOfExtract();

	const char* getSubstitutePathOfArch(unsigned short threadId);

private:
	const char* getValue(const char* pSection, const char* pKey);
	bool saveValue(const char* pSection, const char* pKey, const char* pValue);	

public:
	inline char* getFileName(){
		return m_fileName;
	}


};
#endif
                                                                                                                                                                                                                                                             
