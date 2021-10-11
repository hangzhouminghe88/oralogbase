
#include <string.h>
#include <errno.h>

#include "lgc_param.h"
#include "lgc_api.h"


Mutex LGC_Param::s_mutex;
LGC_Param* LGC_Param::s_instance = NULL;

LGC_Param::LGC_Param():m_ini(true,false,true), m_fileName(NULL)
{
	return;
}

LGC_Param::~LGC_Param()
{
	if(m_fileName){
		delete[] m_fileName;
		m_fileName = NULL;
	}

	return;
}

LGC_Param* LGC_Param::getInstance()
{
	
	if(s_instance == NULL){
		s_mutex.Lock();
		if(s_instance == NULL){
			s_instance = new LGC_Param;
			if(s_instance == NULL ){
				lgc_errMsg("new error:%s\n", strerror(errno));
				exit(1);
			}
		}
		s_mutex.Unlock();
	}
	return s_instance;
}

bool LGC_Param::loadParaFile(const char* fileName)
{
	bool bRet = true;
	int iFuncRet = 0;

	if(fileName == NULL){
		bRet = false;
		goto errOut;
	}

	iFuncRet = m_ini.LoadFile(fileName);
	if(iFuncRet != 0){//loadFile failed
		lgc_errMsg("loadFile failed:%s\n", fileName);
		bRet = false;
		goto errOut;
	}

	//copy param file name
	if(m_fileName){
		delete[] m_fileName;
		m_fileName = NULL;
	}
	m_fileName = new char[strlen(fileName)+1];
	if(m_fileName == NULL){
		lgc_errMsg("new failed\n");
		bRet = false;
		goto errOut;
	}
	strcpy(m_fileName,fileName);



	
errOut:
	return bRet;
}


const char* LGC_Param::getMediaFileDir()
{
	const char* p = NULL;
	p = getValue("MEDIAFILE","FILE_DIR");
	if(p == NULL || *p == '\0'){
		lgc_errMsg("getMediaFileDir failed \n");
		exit(1);
	}
	return p;
}
const char* LGC_Param::getTargetHost()
{
	const char* p = NULL;
	p = getValue("TARGET", "HOST");
	if(p == NULL || *p == '\0'){
		lgc_errMsg("getTargetHost failed\n");
		exit(1);
	}
	return p;
}

int LGC_Param::getTargetPort()
{
	int port = 0;
	const char* p = NULL;
	p = getValue("TARGET","PORT");
	if(p == NULL || *p == '\0'){
		lgc_errMsg("getTargetPort failed\n");
		exit(1);
	}
	
	sscanf(p,"%d", &port);

	return port;
}

int LGC_Param::getListenPort()
{
	int port = 0;
	const char* p = NULL;
	p = getValue("LISTEN","PORT");
	if(p == NULL || *p == '\0'){
		lgc_errMsg("getListenPort failed\n");
		exit(1);
	}

	sscanf(p,"%d",&port);

	return port;
}

const char* LGC_Param::getLogDir()
{
	const char* p= NULL;
	p = getValue("LOG", "DIR");
	if(p == NULL || *p == '\0'){
		lgc_errMsg("getLogDir failed\n");
		exit(1);
	}
	return p;
}

//private
const char* LGC_Param::getValue(const char* pSection, const char* pKey)
{
	return m_ini.GetValue(pSection, pKey);
}

bool LGC_Param::saveValue(const char* pSection, const char* pKey, const char* pValue)
{
	bool bRet = true;
	int iFuncRet = 0;
	iFuncRet = m_ini.SetValue(pSection, pKey, pValue);
	if(iFuncRet < 0){
		lgc_errMsg("saveValue failed:%d\n",iFuncRet);
		bRet = false;
		goto errOut;
	}
	
	iFuncRet = m_ini.SaveFile(m_fileName);
	if(iFuncRet != 0){
		lgc_errMsg("saveFile failed:%s\n", m_fileName);
		bRet = false;
		goto errOut;
	}
	
	//success
	bRet = true;

errOut:
	return bRet;
}


