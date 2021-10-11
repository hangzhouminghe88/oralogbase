#ifndef LGC_PARAM_H
#define LGC_PARAM_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "Mutex.h"
#include "SimpleIni.h"

class LGC_Param
{
private:
	CSimpleIniA m_ini;
	char* m_fileName;

	static Mutex s_mutex;
	static LGC_Param* s_instance;

private:
	LGC_Param();//singleton mode

public:
	static LGC_Param* getInstance();
	~LGC_Param();

	bool loadParaFile(const char* fileName);

	//get functions
	const char* getMediaFileDir();
	const char* getTargetHost();
	int getTargetPort();
	int getListenPort();
	const char* getLogDir();

private:
	const char* getValue(const char* pSection, const char* pKey);
	bool saveValue(const char* pSection, const char* pKey, const char* pValue);	

};
#endif

