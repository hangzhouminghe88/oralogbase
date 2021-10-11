#include "lgc_RedoRecordInput.h"
#include "lgc_RedoFile.h"
#include "lgc_RedoRecord.h"

/*
*迭代地取出RedoRecord
*
*/

//...............................
//constructor and desctructor
//...............................

LGC_RedoRecordInput::LGC_RedoRecordInput(LGC_RedoFile* pRedoFile)
{
	m_pRedoFile				= pRedoFile;
	m_pFirstRedoRecord		= NULL;
	m_pLastIncompRedoRecord = NULL;
}

LGC_RedoRecordInput::~LGC_RedoRecordInput()
{
	if(m_pFirstRedoRecord != NULL ){
		delete m_pFirstRedoRecord;
		m_pFirstRedoRecord = NULL;
	}

	if( m_pLastIncompRedoRecord != NULL ){
		delete m_pLastIncompRedoRecord;
		m_pLastIncompRedoRecord = NULL;
	}

	return;
}

//...............................
//public member functions
//...............................

/*
*功能:		迭代地取出RedoRecord
*输出:		取出的RecordRecord赋值给ppRedoRecord(在内部new的)
*返回值:	< 0 -- 失败 0--没有了 1--成功
*/
int LGC_RedoRecordInput::getNextRedoRecord(LGC_RedoRecord** ppRedoRecord)
{
	
	if(m_pFirstRedoRecord){//第一个RedoRecord还没被读出
		
		//读出第一个RedoRecord
		*ppRedoRecord		= m_pFirstRedoRecord;
		m_pFirstRedoRecord	= NULL;
		return 1;
	}else{//第一个RedoRecord已经被读出

		//从RedoFile中读出RedoRecord
		return this->readNextRedoRecordFromFile(ppRedoRecord);
	}
}

//............................
//private member functions
//............................

/*
*功能:		从redoFile中读取出RecordRecord
*前提条件:	读取指针已经指向了RedoRecord的起始位置
*输出:		读出的RedoRecord赋值给ppRedoRecord (在内部new的)
*返回值:	<0--失败 0--没有了 1--成功
*/
int LGC_RedoRecordInput::readNextRedoRecordFromFile(LGC_RedoRecord** ppRedoRecord)
{
	int iRet		= 0;
	int bytesReaded = 0;

	log_record_header_major		logRecordHeader;
	unsigned int				logRecordHeaderLen	= sizeof(logRecordHeader);
	char*						recordBody			= NULL;
	unsigned int				recordBodyLen		= 0;
	LGC_RedoRecord*				pRedoRecord			= NULL;
	
	//获取当前的读取指针的位置
	RBA rba = m_pRedoFile->getCurRBA();

	//从redofile读取出redoRecordHeader
	bytesReaded = this->readRedoRecordHeaderFromFile(&logRecordHeader,&logRecordHeaderLen);
	if(!(bytesReaded == 0 
		  || (bytesReaded > 0 && bytesReaded == logRecordHeaderLen)))
	{//要么什么都没读出 要么整个读出 否则异常

		lgc_errMsg("readRedoRecordHeaderFromFile failed \n");
		goto errOut;
	}

	if(bytesReaded == 0){//Record已经被读完了
		iRet = 0;
		goto successOut;
	}
	
	//recordHeader的长度只能为sizeof(log_record_header_minor) 
	//或者sizeof(log_record_header_major)
	if(!lgc_check(	   logRecordHeaderLen == sizeof(log_record_header_minor) 
					|| logRecordHeaderLen == sizeof(log_record_header_major) ))
	{
		lgc_errMsg("check failed \n");
		goto errOut;
	}

	//从redofile中读出recordBody的数据
	bytesReaded = this->readRedoRecordBody((const log_record_header_minor*)&logRecordHeader,logRecordHeaderLen,&recordBody,&recordBodyLen);
	if(!(   (bytesReaded == 0 && bytesReaded == recordBodyLen && recordBody == NULL ) 
		 || (bytesReaded >  0 && bytesReaded == recordBodyLen && recordBody != NULL ) ))
	{//没有读到数据必须没有数据返回
	 //读到数据必须有数据返回

		lgc_errMsg("readRedoRecordBody failed \n");
		goto errOut;
	}

	
	//根据recordheader和recordbody创建出RedoRecord
	pRedoRecord = LGC_RedoRecord::createRedoRecord(m_pRedoFile, rba,&logRecordHeader, logRecordHeaderLen, recordBody, recordBodyLen);
	if(!(pRedoRecord != NULL)){

		lgc_errMsg("createRedoRecord failed \n");
		goto errOut;
	}
	
	//如果record的数据还没有被读完，则把它保留下来
	//以便从下一个RedoFile读取剩下的数据
	if ( pRedoRecord->recordBodyFinish() == false){

		//将recordbody数据不完整的redorecord保留下来
		//剩下的数据可以从下一个redofile中取
		this->saveLastIncompleteRecord(pRedoRecord);
		pRedoRecord = NULL;
		iRet = 0;
		goto successOut;
	}
	
	//跳到下一个redoRecord位置，以便下次读取新的redorecord
	if ( this->skipToNextRecord() < 0){
		lgc_errMsg("skipToNextRecord failed \n");
		goto errOut;
	}


	//success 
	iRet = 1;
	*ppRedoRecord	= pRedoRecord;
	pRedoRecord		= NULL;
	goto successOut;

successOut:
    if(recordBody){
		delete[] recordBody;
		recordBody = NULL;
	}
	if(pRedoRecord){
		delete pRedoRecord;
		pRedoRecord = NULL;
	}
	return iRet;

errOut:
		
    if(recordBody){
		delete[] recordBody;
		recordBody = NULL;
	}
	if(pRedoRecord){
		delete pRedoRecord;
		pRedoRecord = NULL;
	}
	
	return -1;
}

/*
*功能:		从redofile中读取出recordHeader
*前提条件:	读取指针已近位于record的起始位置
*输入：		pRecordHeaderLen -- recordHeadBuf的最大大小
*输出:		recordHeaderBuf  -- 存放recordheader的数据
*			pRecordHeaderLen -- recordHeader的真实大小
*返回:		<0 -- 失败 0--文件结束 >0 --成功
*/
int LGC_RedoRecordInput::readRedoRecordHeaderFromFile(void* recordHeaderBuf, unsigned int* pRecordHeaderLen)
{
	int	bytesReaded	= 0;

	//recordHeaderBuf不能为NULL
	//且*pRecordHeaderLen大于等于RecordHeader的最小大小
	if(!lgc_check(   recordHeaderBuf 
		          && *pRecordHeaderLen >= sizeof(log_record_header_minor) ))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}
	
	//尝试从redofile中读取出minor recordheader, 
	//但是读取位置不会变
	bytesReaded = m_pRedoFile->tryReadRecordHeader(recordHeaderBuf, sizeof(log_record_header_minor));
	if(!(  bytesReaded == 0 
		|| bytesReaded == sizeof(log_record_header_minor) ))
	{//要么什么都没读到，要么整体读出来 否则异常

		lgc_errMsg("tryReadRecordHeader failed \n");
		return -1;
	}
	
	//如果什么都没读到，则读完了
	if(bytesReaded == 0){
		return 0;
	}

	//计算recordheaderlen
	const unsigned  char vld				= ((log_record_header_minor*)recordHeaderBuf)->valid;
	const unsigned  int	 recordHeaderLen	= LGC_RedoRecordInput::calRecordHeaderLen(vld);

	
	//计算出的record的长度 应该小于等于 record的最大大小
	if(!(*pRecordHeaderLen >= recordHeaderLen)){
		lgc_errMsg("check failed \n");
		return -1;
	}
	
	//从redo file中读取出recordheader
	bytesReaded = m_pRedoFile->readRecordHeader(recordHeaderBuf, recordHeaderLen);
	if(!(    bytesReaded == 0 
		  || bytesReaded == recordHeaderLen ))
	{//要么什么都没读到 要么整体读出来 否则异常

		lgc_errMsg("readRecordHeader failed \n");
		return 1;
	}

	//什么都没读到，则没有record了
	if(bytesReaded == 0){
		return 0;
	}

	//success
	*pRecordHeaderLen = recordHeaderLen;
	return *pRecordHeaderLen;
}

/*
*功能:		从readFile中读取出recordbody的数据
*输出:		ppRecordBody   -- 指向new出的存有recordbody数据的一块内存
*			pRecordBodyLen -- recordBody读到的数据的长度
*返回:		<0-- 失败 0-- 文件结束 >0-- 成功
*附加说明:	里面会根据RecordBody的长度，创建recordBody
*/
int 
LGC_RedoRecordInput::readRedoRecordBody(const log_record_header_minor* pRecordHeader, int recordHeaderLen, char** ppRecordBody, unsigned int* pRecordBodyLen)
{
	int bytesReaded = 0;

	//pRecordHeader不能为空
	//且recordHeader的长度只能是sizeof(log_record_header_minor) 或者sizeof(log_record_header_major) 
	if(!(    pRecordHeader 
		  && (    recordHeaderLen == sizeof(log_record_header_minor) 
		       || recordHeaderLen == sizeof(log_record_header_major) 
		     ) ))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}
	
	const unsigned int recordLen     = pRecordHeader->record_length;
	const unsigned int recordBodyLen = recordLen - recordHeaderLen;

	//record的长度大于recordHeader的长度
	if(!(recordLen > recordHeaderLen)){
		lgc_errMsg("check failed \n");
		return -1;
	}
	
	//new recordBody
	char* pRecordBody = new char[recordBodyLen];
	if(pRecordBody == NULL){
		lgc_errMsg("new failed \n");
		return -1;
	}
	memset(pRecordBody,0,recordBodyLen);
	
	//从redoFile中读取RecordBody
	bytesReaded = m_pRedoFile->readRecordBody(pRecordBody,recordBodyLen);
	if( !(    bytesReaded >= 0 
		   && bytesReaded <= recordBodyLen) )
	{//读取的长度不能越界 否则异常

		lgc_errMsg("readRecordBody failed \n");
		delete[] pRecordBody;
		pRecordBody = NULL;
		return -1;
	}

	//success
	*ppRecordBody = pRecordBody;
	pRecordBody =NULL;
	*pRecordBodyLen = recordBodyLen;
	if(*pRecordBodyLen == 0 && *ppRecordBody != NULL){
		delete[] *ppRecordBody;
		*ppRecordBody=NULL;
	}
	return *pRecordBodyLen;
}

/*
*功能: 将最后的数据不完整的redorecord保留下来
*输入: pRedoRecord -- 要保持的record
*/
void 
LGC_RedoRecordInput::saveLastIncompleteRecord(const LGC_RedoRecord* pRedoRecord)
{
	//要保存的Record不为空
	//且数据还没读完
	if(!(   pRedoRecord != NULL 
		 && pRedoRecord->recordBodyFinish() == false))
	{
		lgc_errMsg("check failed \n");
		exit(1);
	}
	
	//保存
	m_pLastIncompRedoRecord = (LGC_RedoRecord*)pRedoRecord;
	return;
}

/*
*功能: 跳到下一个redorecord
*      也就是读取位置跳到下一个Record的起始位置
*
*/
int  LGC_RedoRecordInput::skipToNextRecord()
{
	return m_pRedoFile->skipToNextRecordPos();
}


//.............................
//static member functions
//.............................

/*
*功能: 创建RedoRecordInput对象
*输入: pPrevRedoRecordInput -- 上一个RedoFile对应的RedoRecordInput对象
*      pRedoFile            -- 当前redoFile
*/
LGC_RedoRecordInput* 
LGC_RedoRecordInput::createRedoRecordInput(LGC_RedoRecordInput* pPrevRedoRecordInput, LGC_RedoFile* pRedoFile)
{
	int iFuncRet = 0;

	LGC_RedoRecordInput* pRedoRecordInput = NULL;
	LGC_RedoRecord* pPrevLastIncompRedoRecord = NULL;
	LGC_IncompleteRedoRecordData *pIncompleteRedoRecordData = NULL;

	//创建RedoRecordInput
	pRedoRecordInput = new LGC_RedoRecordInput(pRedoFile);
	if(pRedoRecordInput == NULL){
		lgc_errMsg("new failed \n");
		return NULL;
	}
	
	//如果上一个redoRecordInput中保留有数据不完整的redorecord
	//则从现在这个redofile中加载剩余的数据
	if(pPrevRedoRecordInput != NULL
	        && pPrevRedoRecordInput->isLeftIncompleteRedoRecord() )
	{
		//从当前RedoFile中获取第一个RedoRecordData，该recorddata没有头只有数据
		//需要和上一个redoFile的最后一个RedoRecord合并
		pIncompleteRedoRecordData = pRedoFile->getFirstIncompleteRedoRecordData();
		if(pIncompleteRedoRecordData == NULL ){
			lgc_errMsg("pIncompleteRedoRecordData is NULL \n");
			return NULL;
		}

		//从上一个RedoFile对应的RedoRecordInputz中回去最后一个RedoRecord
		//该RedoRecord的数据没有读完
		pPrevLastIncompRedoRecord = pPrevRedoRecordInput->getLastIncompleteRedoRecord();
		if(pPrevLastIncompRedoRecord == NULL ){
			lgc_errMsg("pPrevLastIncompRedoRecord is NULL \n");
			return NULL;
		}

		//合并上一个RedoFile的最后一个RedoRecord和当前RedoFile的第一个RedoRecord
		iFuncRet = pPrevLastIncompRedoRecord->loadLeftRedoRecordData(pIncompleteRedoRecordData);
		if(iFuncRet < 0)
		{
			lgc_errMsg("loadLeftRedoRecordData failed \n");
			return NULL;
		}
		
		//将合并后的RedoRecord从上一个RedoRecordInput转移到当前的RedoRecordInput
		pPrevRedoRecordInput->detachLastIncompleteRedoRecord();
		pRedoRecordInput->setFirstRedoRecord(pPrevLastIncompRedoRecord);
		pPrevLastIncompRedoRecord = NULL;
	}
	
	//success
	return pRedoRecordInput;
}


/*
*功能: 计算recordHeader的长度
*返回: recordHeader的长度
*/
unsigned int 
LGC_RedoRecordInput::calRecordHeaderLen(unsigned char vld)
{
	unsigned int recordHeaderLen = 0;

	switch( vld )
	{
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x0d:
		case 0x14://tw add 20150718
			recordHeaderLen = sizeof(log_record_header_major);
			break;
		case 0x01:
		case 0x02:
		case 0x09:
			recordHeaderLen = sizeof(log_record_header_minor);
			break;
		case 0x00:
		case 0x03:
			recordHeaderLen = sizeof(log_record_header_minor);
			break;
		default:
			recordHeaderLen = sizeof(log_record_header_minor);
			break;
	}

	return recordHeaderLen;
}