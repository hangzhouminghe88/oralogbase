#include "lgc_RedoFileInput.h"
#include "lgc_RedoFile.h"
#include "OciQuery.h"

/*
*class LGC_RedoFileInput
*
*从RedoFileInput中取出一个一个RedoFile,
*取出的RedoFile按sequence号严格排序，且sequence号增长步长是1
*
*/

//constructor and desctructor

/*
*构造函数
*/
LGC_RedoFileInput::LGC_RedoFileInput(LGC_RedoFileInfoList* pRedoFileInfoList,OciQuery* pQuery)
{
	m_pRedoFileInfoList = pRedoFileInfoList;
	m_pQuery = pQuery;
}

/*
*析构函数
*/
LGC_RedoFileInput::~LGC_RedoFileInput()
{
	if(m_pRedoFileInfoList){
		delete m_pRedoFileInfoList;
		m_pRedoFileInfoList = NULL;
	}
}

//public member functions

/*
*从RedoFileInput中获取下一个RedoFile
*/
int LGC_RedoFileInput::getNextRedoFile(LGC_RedoFile** ppRedoFile)
{	

	
	int              iFuncRet = 0;
	LGC_RedoFileInfo redoFileInfo;

	//init out variables
	*ppRedoFile = NULL;
	
	//从RedoFileInfoList中获取下一个RedoFile的基本信息
	iFuncRet = m_pRedoFileInfoList->getNextRedoFileInfo(&redoFileInfo);
	if(iFuncRet < 0){
		return -1;
	}else if(iFuncRet == 0){
		return 0;
	}
	
	fprintf(stdout, "fileName=%s \n", redoFileInfo.fileName);
	
	//根据redoFileInfo创建RedoFile
	*ppRedoFile = LGC_RedoFile::createRedoFile(redoFileInfo,m_pQuery);
	if(NULL == *ppRedoFile){
		return -1;
	}

	return 1;
}

//static functions

/*
*创建RedoFileInput对象
*输入: threadId -- rac环境下的节点号
*      startSCN -- 开始分析的起始scn号
*/
LGC_RedoFileInput* 
LGC_RedoFileInput::createRedoFileInput(int threadId, BYTE8 startSCN, OciQuery* pQuery)
{
	LGC_RedoFileInfoList* pRedoFileInfoList = NULL;
	LGC_RedoFileInput*    pRedoFileInput    = NULL;
	
	//创建RedoFileInfoList对象 里面的redoFileInfo按照sequence号严格排序
	pRedoFileInfoList = LGC_RedoFileInfoList::createRedoFileInfoList(threadId, startSCN, pQuery);
	if(pRedoFileInfoList == NULL){
		return NULL;
	}
	
	//根据RedoFileInfoList创建RedoFile
	pRedoFileInput = new LGC_RedoFileInput(pRedoFileInfoList,pQuery);
	if(pRedoFileInput == NULL){
		return NULL;
	}
	
	//success
	return pRedoFileInput;
}


//......................................
//tool classes for LGC_RedoFileInput
//......................................


//.......................................
//class LGC_RedoFileInfoList
//.......................................

static int loadArchiveFileList(void* _pArchiveFileList, const char* pVal, char* fieldNum);
static int loadOnlineFileList(void* _pArchiveFileList, const char* pVal, char* fieldNum);

//constructor and desctructor
LGC_RedoFileInfoList::LGC_RedoFileInfoList(int threadId, BYTE8 startSCN,OciQuery* pQuery)
{
	m_threadId = threadId;
	m_startSCN = startSCN;
	m_pQuery   = pQuery;
	
	//m_archiveFile_list 
	//m_onlineFile_list

	m_curSequence = 0;

	return;
}

LGC_RedoFileInfoList::~LGC_RedoFileInfoList()
{
	return;
}

//public member functions
int LGC_RedoFileInfoList::getNextRedoFileInfo(LGC_RedoFileInfo* pRedoFileInfo)
{
	int iFuncRet = 0;

	if(m_archiveFile_list.empty() && m_onlineFile_list.empty() ){
		iFuncRet = this->loadLeftRedoFileInfo();
	}
	
	if(m_archiveFile_list.empty() && m_onlineFile_list.empty() ){
		return 0;
	}

	list<LGC_RedoFileInfo>::iterator it;

	if(!m_archiveFile_list.empty() ){
		it = m_archiveFile_list.begin();
		*pRedoFileInfo = *it;
		m_archiveFile_list.erase(it);
	}else{
		//check: m_onlineFile_list.size() == 1

		it = m_onlineFile_list.begin();
		*pRedoFileInfo = *it;
		m_onlineFile_list.erase(it);
	}
	
	//check: m_threadId == pRedoFileInfo->threadId && m_curSequence == pRedoFileInfo->sequence

	//success
	m_curSequence++;
	return 1;
}

int LGC_RedoFileInfoList::redoFileInfo_pushBack(const LGC_RedoFileInfo& redoFileInfo)
{
	switch(redoFileInfo.fileType){
		case FILE_ARCHIVE_TYPE:
			m_archiveFile_list.push_back(redoFileInfo);
			break;
		case FILE_ONLINE_CURRENT_TYPE:
		case FILE_ONLINE_ACTIVE_TYPE:
		case FILE_ONLINE_DEACTIVE_TYPE:
			m_onlineFile_list.push_back(redoFileInfo);
			break;
		default:
			return -1;
			break;
	}

	return 1;
}

//private member functions
int LGC_RedoFileInfoList::init()
{
	bool bFuncRet = true;

	//check: m_archiveFile_list.empty() && m_onlineFile_list.empty()

	list<LGC_RedoFileInfo>::iterator it;

	bFuncRet = m_pQuery->lgc_getArchFileList(this, m_threadId,m_startSCN, loadArchiveFileList);

	if(m_archiveFile_list.empty()){
		bFuncRet = m_pQuery->lgc_getOnlineFileList(this, m_threadId, m_startSCN, loadOnlineFileList);
	}

	//check: !(m_archiveFile_list.empty() && m_onlineFile_list.empty() ) 
	
	if(!m_archiveFile_list.empty() ){
		it = m_archiveFile_list.begin();
	}else{
		//check m_onlineFile_list.size() == 1

		it = m_onlineFile_list.begin();
	}

	m_curSequence = (*it).sequence;

	return 0;
}

int LGC_RedoFileInfoList::loadLeftRedoFileInfo()
{
	bool bFuncRet = true;

	//check: m_archiveFile_list.empty() && m_onlineFile_list.empty()
	
	bFuncRet = m_pQuery->lgc_getArchFileListBySeq(this, m_threadId, m_curSequence, loadArchiveFileList);

	if(m_archiveFile_list.empty()){
		bFuncRet = m_pQuery->lgc_getOnlineFileListBySeq(this, m_threadId, m_curSequence, loadOnlineFileList);
	}

	return 0;
}

//static member functions
LGC_RedoFileInfoList* 
LGC_RedoFileInfoList::createRedoFileInfoList(int threadId, BYTE8 startSCN, OciQuery* pQuery)
{
	int iFuncRet = 0;
	LGC_RedoFileInfoList* pRedoFileInfoList = NULL;

	pRedoFileInfoList = new LGC_RedoFileInfoList(threadId, startSCN, pQuery);
	if(pRedoFileInfoList == NULL){
		return NULL;
	}

	iFuncRet = pRedoFileInfoList->init();
	if(iFuncRet < 0){
		delete pRedoFileInfoList;
		pRedoFileInfoList = NULL;
		return NULL;
	}
	
	//success
	return pRedoFileInfoList;
}


//........................................................
//tool functions for LGC_RedoFileInfoList
//........................................................

//tool functions for LGC_RedoFileInfoList
static int loadArchiveFileList(void* _pArchiveFileList, const char* pVal, char* fieldNum)
{
	if(pVal == NULL)//no data
		return QEURY_END_DATA;
	
	LGC_RedoFileInfoList* pRedoFileList = (LGC_RedoFileInfoList*)_pArchiveFileList;
	LGC_RedoFileInfo redoFileInfo;
	redoFileInfo.fileType = FILE_ARCHIVE_TYPE;

	stringstream sstr(pVal);
	
	sstr >> redoFileInfo.threadId;
	sstr >> redoFileInfo.sequence;
	sstr >> redoFileInfo.firstChange;
	sstr >> redoFileInfo.nextChange;
	sstr >> redoFileInfo.fileName;
	sstr >> redoFileInfo.blkSize;
	sstr >> redoFileInfo.totalBlks;

	if(pRedoFileList->redoFileInfo_pushBack(redoFileInfo) < 0){
		return -1;
	}

	return 1;
}



static int loadOnlineFileList(void* _pOnlineFileList, const char* pVal, char* fieldNum)
{
	if(pVal == NULL)
		return QEURY_END_DATA;

	LGC_RedoFileInfoList* pRedoFileList = (LGC_RedoFileInfoList*)_pOnlineFileList;
	LGC_RedoFileInfo redoFileInfo;

	char status[126] = {0};
	char archived[126] = {0};

	stringstream sstr(pVal);

	sstr >> redoFileInfo.threadId;
	sstr >> redoFileInfo.sequence;
	sstr >> redoFileInfo.firstChange;
	sstr >> redoFileInfo.nextChange;
	sstr >> redoFileInfo.fileName;
	sstr >> redoFileInfo.blkSize;
	sstr >> redoFileInfo.totalBlks;

	sstr >> status;
	sstr >> archived;

	if(strcmp(archived,"YES") == 0){//the online log have archived
		return QEURY_END_DATA;
	}
	
	if(strcmp(status,"CURRENT") == 0){
		redoFileInfo.fileType = FILE_ONLINE_CURRENT_TYPE;
	}else if(strcmp(status,"ACTIVE") == 0){
		redoFileInfo.fileType = FILE_ONLINE_ACTIVE_TYPE;
	}else if(strcmp(status,"DEACTIVE") == 0){
		redoFileInfo.fileType = FILE_ONLINE_DEACTIVE_TYPE;
	}else{
//		lgc_errMsg("online log status is invalid\n");
		return -1;
	}

	if(pRedoFileList->redoFileInfo_pushBack(redoFileInfo) < 0){
		//lgc_errMsg("pRedoFileList->onlineFileList_pushBack \n");
		return -1;
	}

	return QEURY_END_DATA;//just get one row
}


