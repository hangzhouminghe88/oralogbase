#include <string.h>
#include <errno.h>

#include "lgc_param.h"
#include "lgc_api.h"


Mutex LGC_Param::s_mutex;
LGC_Param* LGC_Param::s_instance = NULL;

LGC_Param::LGC_Param():m_ini(true,false,true), m_fileName(NULL)
{
	memset(m_archPathArray, 0, sizeof(m_archPathArray));
	m_maxLenOfMediaFile = 0;
	return;
}

LGC_Param::~LGC_Param()
{
	int i = 0;

	if(m_fileName){
		delete[] m_fileName;
		m_fileName = NULL;
	}

	for(i=0; i<sizeof(m_archPathArray)/sizeof(char*); i++){
		if(m_archPathArray[i] != NULL){
			delete m_archPathArray[i];
			m_archPathArray[i] = NULL;
		}
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
				fprintf(stderr,"new error:%s\n", strerror(errno));
				exit(1);
			}
		}
		s_mutex.Unlock();
	}
	return s_instance;
}

bool LGC_Param::loadParamFile(const char* fileName)
{
	bool bRet = true;
	int iFuncRet = 0;

	if(fileName == NULL){
		bRet = false;
		goto errOut;
	}

	iFuncRet = m_ini.LoadFile(fileName);
	if(iFuncRet != 0){//loadFile failed
		fprintf(stderr,"loadFile failed:%s\n", fileName);
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
		fprintf(stderr,"new failed\n");
		bRet = false;
		goto errOut;
	}
	strcpy(m_fileName,fileName);

errOut:
	return bRet;
}

const char* LGC_Param::getTableList()
{
	const char* p = this->getValue("TABLES","LIST");
	if(NULL == p){
		lgc_errMsg("getTableList failed\n");
		exit(1);
	}
	return p;

}

const char* LGC_Param::getExcludeTableList()
{
	const char *p = this->getValue("TABLES", "EXCLUDE_LIST");
	if(NULL == p){
		lgc_errMsg("getExcludeTableList failed\n");
		exit(1);
	}
	return p;
}
const char* LGC_Param::getLogDir()
{
	const char* p = this->getValue("LOG","LOG_DIR");
	if(p == NULL || *p == '\0'){
		lgc_errMsg("getLogDir failed\n");
		exit(1);
	}
	return p;

}

const char* LGC_Param::getOciUser()
{
	const char* p = this->getValue("OCI","USER");
	if(NULL == p){
		lgc_errMsg("getOciUser failed\n");
		exit(1);
	}
	return p;
}

const char* LGC_Param::getOciPasswd()
{
	const char* p = this->getValue("OCI","PASSWD");
	if(NULL == p){
		lgc_errMsg("getOciPasswd failed\n");
		exit(1);
	}
	return p;

}

const char* LGC_Param::getOciSid()
{
	const char* p = this->getValue("OCI","SID");
	if(NULL == p){
		lgc_errMsg("getOciSid failed\n");
		exit(1);
	}
	return p;
}

const char* LGC_Param::getASMUser()
{
	const char* p = this->getValue("ASM", "USER");
	if(NULL == p){
		lgc_errMsg("getASMUser failed\n");
		exit(1);
	}
	return p;
}

const char* LGC_Param::getASMPasswd()
{
	const char *p = this->getValue("ASM", "PASSWD");
	if(NULL == p){
		lgc_errMsg("getASMPasswd \n");
		exit(1);
	}

	return p;

}

const char* LGC_Param::getASMSid()
{
	const char *p = this->getValue("ASM", "ASM");
	if( NULL == p ){
		lgc_errMsg("getASMSid failed\n");
		exit(1);
	}

	return p;
}

const char* LGC_Param::getCkptFileName()
{
	const char* p = this->getValue("CKPT","FILE_NAME");
	if(NULL == p){
		lgc_errMsg("getCkptFileName failed\n");
		exit(1);
	}
	return p;
}

bool LGC_Param::isASM()
{
	bool isASM = true;
	const char* p = this->getValue("LOG","ASM");
	if(NULL == p){
		lgc_errMsg("isASM failed \n");
		exit(1);
	}

	if( 0 == strcmp(p,"YES")){
		
		isASM = true;
	}else{
		isASM = false;
	}

	return isASM;
}

BYTE8 LGC_Param::getStartSCNOfExtract()
{

	BYTE8 startSCN = 0;

	char scnStr[126] = { 0 };
	const char* p = NULL;
	p = this->getValue("EXTRACT", "START_SCN");
	if(p == NULL || *p == '\0'){
		startSCN = 0;
		goto errOut;
	}

	sscanf(p, "%lld", &startSCN);
	
errOut:
	return startSCN;

}

const WORD  LGC_Param::getThreadCount()
{
	WORD threadCount = 0;
	const char* p = NULL;
	p = this->getValue("EXTRACT", "THREADID");
	if(p == NULL || *p == '\0'){
		lgc_errMsg("getThreadCount failed\n");
		exit(1);
	}
	printf("%s\n", p);
	sscanf(p, "%hu", &threadCount);
	if(threadCount > 5 || threadCount == 0){
		lgc_errMsg("threadCount invalid:threadCount=%u fileName=%s\n", threadCount, m_fileName);
		exit(1);
	}

	return threadCount;
}

const WORD  LGC_Param::getThreadId()
{
	WORD threadCount = 0;
	const char* p = NULL;
	p = this->getValue("EXTRACT", "THREADID");
	if(p == NULL || *p == '\0'){
		lgc_errMsg("getThreadCount failed\n");
		exit(1);
	}
	printf("%s\n", p);
	sscanf(p, "%hu", &threadCount);
	if(threadCount > 5 || threadCount == 0){
		lgc_errMsg("threadCount invalid:threadCount=%u fileName=%s\n", threadCount, m_fileName);
		exit(1);
	}

	return threadCount;
}


//const WORD  LGC_Param::getThreadCount()
//{
//	WORD threadCount = 0;
//	const char* p = NULL;
//	p = this->getValue("EXTRACT", "THREADS");
//	if(p == NULL || *p == '\0'){
//		lgc_errMsg("getThreadCount failed\n");
//		exit(1);
//	}
//	printf("%s\n", p);
//	sscanf(p, "%hu", &threadCount);
//	if(threadCount > 5 || threadCount == 0){
//		lgc_errMsg("threadCount invalid:threadCount=%u fileName=%s\n", threadCount, m_fileName);
//		exit(1);
//	}
//
//	return threadCount;
//}

const char* LGC_Param::getMediaFileDir()
{
	const char* p = this->getValue("MEDIA_FILE","FILE_DIR");
	if(NULL == p){
		lgc_errMsg("getCkptFileName failed\n");
		exit(1);
	}
	return p;
}

DWORD LGC_Param::getMaxLenOfMediaFile()
{
	
	if(m_maxLenOfMediaFile > 0){
		return m_maxLenOfMediaFile;
	}

	DWORD maxLen = 0;
	const char* p = this->getValue("MEDIA_FILE", "MAXLEN");
	if(p == NULL || *p == '\0'){
		lgc_errMsg("getMaxLenOfMediaFile failed: p=%s \n", p == NULL? "NULL":p);
		exit(1);
	}

	maxLen = (DWORD)atoi(p);
	m_maxLenOfMediaFile = maxLen;
	return maxLen;
}

const char* LGC_Param::getSubstitutePathOfArch(unsigned short threadId)
{
	if(m_archPathArray[threadId] != NULL){//return from the cache
		if(strlen(m_archPathArray[threadId]) == 0){
			return NULL;
		}else{
			return m_archPathArray[threadId];
		}
	}

	const char* p = this->getValue("ARCH_FILE", "SUBSTITUTE_PATH");
	if(NULL == p){
		lgc_errMsg("getSubstitutePathOfArch failed\n");
		exit(1);
	}

	while( (p = strchr(p+1,'|')) != NULL){
		if( *(p-1) == '0'+threadId){//find the thread archive dir
			break;
		}
	}

	//assert
	if(m_archPathArray[threadId] != NULL){
		lgc_errMsg("m_archPathArray[threadId] != NULL \n");
		exit(1);
	}

	if(p != NULL){//find
		p++;
		const char* q = strchr(p, ',');
		if( q == NULL){
			lgc_errMsg("ARCH_FILE SUBSTITUTE_PATH invalied\n");
			exit(1);
		}
		m_archPathArray[threadId]= new char[q-p+1];
		if(m_archPathArray[threadId] == NULL){
			lgc_errMsg("new failed\n");
			exit(1);
		}
		memset(m_archPathArray[threadId],0,q-p+1);
		strncpy(m_archPathArray[threadId], p, q-p);
		return m_archPathArray[threadId];		
	}else{// not find 
		m_archPathArray[threadId] = new char[1];
		if(m_archPathArray[threadId] == NULL){
			lgc_errMsg("new failed\n");
			exit(1);
		}
		memset(m_archPathArray[threadId], 0, 1);

		return NULL;
	}
}


const char* LGC_Param::getValue(const char* pSection, const char* pKey)
{
	const char* p = m_ini.GetValue(pSection, pKey);
	if(p == NULL || *p == '\0'){
		lgc_errMsg("getVelue failed:seciont=%s key=%s\n", pSection, pKey);
		exit(1);
	}
	return p;
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



