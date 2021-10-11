#include "lgc_RedoFile.h"
#include "lgc_readRedoFile.h"
#include "OciQuery.h"
/*
*redoFile: ��redoFileHeader��redoHeader��redoFileBody���;
*redoFileHeader��Ҫ��¼: blockSize, totalBlocks
*redoHeader��Ҫ��¼: thread#,sequence#,first_Change#,next_change#
*redoFileBody�ɶ��redoRecord���
*redoFile��Ҫ�ṩ��RedoFileBody�ж�ȡ��redorecord�����ݵĽӿ�
*
* ���RedoFileBody�ĵ�һ��Redorecord�����ݲ�����(��һ������������һ��RedoFile��), 
* ��������浽m_pFirstIncompleteRecordData��
*
*/

static BYTE8 calculateSCN(unsigned short highSCN, unsigned int lowSCN);

//.................................
//constructor and destructor
//.................................

/*
*���캯��
*����: redoFileInfo -- redoFile�Ļ�����Ϣ
*      pRedoBlkOp   -- ����־���ж�ȡ���ݵĹ�����
*/
LGC_RedoFile::LGC_RedoFile(const LGC_RedoFileInfo& redoFileInfo, LGC_RedoBlkOp* pRedoBlkOp)
{
	m_redoFileInfo				 = redoFileInfo;
	m_pRedoBlkOp				 = pRedoBlkOp;
	m_pFirstIncompleteRecordData = new LGC_IncompleteRedoRecordData;
	
	if(NULL == m_pFirstIncompleteRecordData){
		lgc_errMsg("new failed \n");
		exit(1);
	}

	return;
}

/*
*��������
*/
LGC_RedoFile::~LGC_RedoFile()
{
	if(m_pRedoBlkOp){
		delete m_pRedoBlkOp;
		m_pRedoBlkOp = NULL;
	}

	if(m_pFirstIncompleteRecordData){
		delete m_pFirstIncompleteRecordData;
		m_pFirstIncompleteRecordData = NULL;
	}
	return;
}

//........................................
//public member functions
//........................................

/*
*��redoFileBody�ж�ȡ��recordHeader������,���Ƕ�ȡָ�벻��
*
*/
int LGC_RedoFile::tryReadRecordHeader(void* recordHeader, int size)
{
	return m_pRedoBlkOp->tryReadRecordHeader(recordHeader,size);	
}


/*
*��redoFileBody�ж�ȡ��recordHeader������
*
*/
int LGC_RedoFile::readRecordHeader(void* recordHeader, int size)
{
	return m_pRedoBlkOp->readData(recordHeader,size);
}

/*
*��redoFileBody�ж�ȡ��recordBody������
*
*/
int LGC_RedoFile::readRecordBody(void* recordBody, int size)
{
	return m_pRedoBlkOp->readData(recordBody,size);
}

/*
*��ȡָ�붨λ����һ��redorecord����ʼλ��
*
*/
int LGC_RedoFile::skipToNextRecordPos()
{
	if(this->isNeedSkipBlock()){
		return this->skipToNextBlock();
	}
	return 0;
}

//..................................
//private member functions
//..................................

/*
*����redoFileHeader
*
*/
int LGC_RedoFile::loadRedoFileHeader()
{
	int iFuncRet = 0;

	const unsigned int blkNo = m_pRedoBlkOp->getBlkNo();
	const unsigned int blkSize = m_pRedoBlkOp->getBlkSize();

	//check: blkNo == 0 && sizeof(log_file_header) == blkSize
	if( !(blkNo == 0 && sizeof(log_file_header) == blkSize)){
		lgc_errMsg("check failed \n");
		return -1;
	}
	
	log_file_header logFileHeader;
	memset(&logFileHeader, 0, sizeof(logFileHeader));
	
	iFuncRet = m_pRedoBlkOp->readDataNoSkipHd(&logFileHeader,sizeof(logFileHeader));
	if(iFuncRet != sizeof(logFileHeader) ){
		lgc_errMsg("read redo file header failed \n");
		return -1;
	}

	//check: logFileHeader.block_size == m_redoRileInfo.blkSize
	//       && logFileHeader.block_count == m_redoFileInfo.totalBlks
	if(!(logFileHeader.block_size == m_redoFileInfo.blkSize
		   && logFileHeader.block_count == m_redoFileInfo.totalBlks))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}
	
	//success 
	return sizeof(logFileHeader);
}

/*
*����redoHeader
*
*/
int LGC_RedoFile::loadRedoHeader()
{
	int iFuncRet = 0;

	const unsigned int blkNo = m_pRedoBlkOp->getBlkNo();
	const unsigned int blkSize = m_pRedoBlkOp->getBlkSize();

	//check:blkNo == 1 && sizeof(log_redo_header) == blkSize
	if(!(blkNo == 1 && sizeof(log_redo_header) == blkSize)){
		lgc_errMsg("check failed \n");
		return -1;
	}

	log_redo_header logRedoHeader;
	memset(&logRedoHeader,0,sizeof(logRedoHeader));

	iFuncRet = m_pRedoBlkOp->readDataNoSkipHd(&logRedoHeader,sizeof(logRedoHeader));
	if(iFuncRet != sizeof(logRedoHeader)){
		lgc_errMsg("read redoHeader failed \n");
		return -1;
	}
	
	const BYTE8 firstSCN = ::calculateSCN(logRedoHeader.low_scn_major, logRedoHeader.low_scn_minor);
	 
	//check: logRedoHeader.thread == m_redoFileInfo.trheadNo
	//       && logRedoHeader.file_number == m_redoFileInfo.sequenceNo
	//       && firstSCN == m_redoFileInfo.firstChange
	if(!(logRedoHeader.thread == m_redoFileInfo.threadId
		  && logRedoHeader.block_head.log_file_sequence == m_redoFileInfo.sequence
		  && firstSCN == m_redoFileInfo.firstChange))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}

	//success
	return sizeof(logRedoHeader);
}

/*
* ��redoBody���ҵ���һ��������RedoRecord����ʼλ��
*
*/
int LGC_RedoFile::skipToFirstRecordPos()
{
	int bytesReaded = 0;
	int bytesToRead = 0;
	int bytesWrited = 0;
	int bytesToWrite = 0;

	const bool fileIsEnd = m_pRedoBlkOp->fileIsEnd();
	if(fileIsEnd){
		return 0;
	}

	unsigned int blkNo = m_pRedoBlkOp->getBlkNo();
	const unsigned int blkHeaderSize = m_pRedoBlkOp->getBlkHeaderSize();
	const unsigned int blkSize = m_pRedoBlkOp->getBlkSize();
	const unsigned int blkBodySize = m_pRedoBlkOp->getBlkBodySize();

	unsigned int curOffsetInBlk = m_pRedoBlkOp->getCurOffsetInBlk();
	unsigned int firstRecordOffsetInBlk = m_pRedoBlkOp->getFirstRecordOff();
	
	char* blkBodyBuf = new char[blkBodySize];
	if(blkBodyBuf == NULL){
		lgc_errMsg("new failed \n");
		return -1;
	}
	memset(blkBodyBuf,0,blkBodySize);

	
	//check:  blkNo == 2 && curOffsetInBlk == blkHeaderSize 
	//         && (firstRecordOffsetInBlk == 0 ||(firstRecordOffsetInBlk >= blkHeaderSize && firstRecordOffsetInBlk < blkSize))
	if(!(blkNo == 2 && curOffsetInBlk == blkHeaderSize
		   && (firstRecordOffsetInBlk == 0 ||(firstRecordOffsetInBlk >= blkHeaderSize && firstRecordOffsetInBlk < blkSize))))
	{
		lgc_errMsg("check failed \n");
		goto errOut;
	}
	
	while( 0 == m_pRedoBlkOp->getFirstRecordOff()){
		blkNo = m_pRedoBlkOp->getBlkNo();
		curOffsetInBlk = m_pRedoBlkOp->getCurOffsetInBlk();

		//check: curOffsetInBlk == blkHeaderSize
		if(!(curOffsetInBlk == blkHeaderSize)){
			lgc_errMsg("check failed \n");
			goto errOut;
		}

		bytesReaded = m_pRedoBlkOp->readData(blkBodyBuf,blkBodySize);
		if(bytesReaded != blkBodySize ){
			lgc_errMsg("read failed \n");
			goto errOut;
		}

		//check: blkNo+1 == m_pRedoBlkOp->getBlkNo()
		if(!(blkNo+1 == m_pRedoBlkOp->getBlkNo())){
			lgc_errMsg("check failed \n");
			goto errOut;
		}

		//write to member buf
		bytesWrited = m_pFirstIncompleteRecordData->write(blkBodyBuf, blkBodySize);
		if(bytesWrited != blkBodySize){
			lgc_errMsg("write failed \n");
			goto errOut;
		}
	}
	
	curOffsetInBlk = m_pRedoBlkOp->getCurOffsetInBlk();
	firstRecordOffsetInBlk = m_pRedoBlkOp->getFirstRecordOff();

	//check: curOffsetInBlk == blkHeaderSize && (firstRecordOffsetInBlk >= blkHeaderSize && firstRecordOffsetInBlk < blkSize)
	if( !(curOffsetInBlk == blkHeaderSize && (firstRecordOffsetInBlk >= blkHeaderSize && firstRecordOffsetInBlk < blkSize)) ){
		lgc_errMsg("check failed \n");
		goto errOut;
	}

	if(firstRecordOffsetInBlk > curOffsetInBlk){
		bytesToRead = (firstRecordOffsetInBlk - curOffsetInBlk);
		bytesReaded  = m_pRedoBlkOp->readData(blkBodyBuf,bytesToRead);
		if(bytesReaded != bytesToRead ){
			lgc_errMsg("read failed \n");
			goto errOut;
		}

		//check: m_pRedoBlkOp->getFirstRecordOff == m_pRedoBlkOp->getCurOffsetInBlk()
		if(!(m_pRedoBlkOp->getFirstRecordOff() == m_pRedoBlkOp->getCurOffsetInBlk())){
			lgc_errMsg("check failed \n");
			goto errOut;
		}

		//write
		bytesToWrite= bytesReaded;
		bytesWrited = m_pFirstIncompleteRecordData->write(blkBodyBuf,bytesToWrite);
		if(bytesWrited != bytesToWrite){
			lgc_errMsg("write failed \n");
			goto errOut;
		}
	}

	//success
	if(blkBodyBuf){
		delete[] blkBodyBuf;
		blkBodyBuf = NULL;
	}
	return 0;

errOut:
	if(blkBodyBuf){
		delete[] blkBodyBuf;
		blkBodyBuf = NULL;
	}
	return -1;
}

//private member tool functions 

/*
* ��һ��RedoRecord������Ѱ����һ��RedoRecord����ʼλ��ʱ��
* ��Ҫ�ж��Ƿ���Ҫ���µ�ǰ���ݿ�ʣ�µ����ݣ�������һ�����ݿ�
*
*/
bool LGC_RedoFile::isNeedSkipBlock()
{
	if(m_pRedoBlkOp->fileIsEnd()){
		return false;
	}

	const unsigned int blkSize = m_pRedoBlkOp->getBlkSize();
	const unsigned int blkHeaderSize = m_pRedoBlkOp->getBlkHeaderSize();
	const unsigned int blkBodySize = m_pRedoBlkOp->getBlkBodySize();

	const unsigned int minRecordHeaderSize = sizeof(log_record_header_minor);
	const unsigned int maxRecordSize = 0xffffff;

	const unsigned int leftBytesInBlk = m_pRedoBlkOp->getLeftBytesInBlk();
	const unsigned int curOffsetInBlk = m_pRedoBlkOp->getCurOffsetInBlk();
	

	if(!lgc_check(curOffsetInBlk >= blkHeaderSize 
		           && curOffsetInBlk < blkSize
		           && leftBytesInBlk >0 
		           && leftBytesInBlk <= blkBodySize)){
		lgc_errMsg("check failed: curOffsetInBlk=%u leftBytesInBlk=%u\n", curOffsetInBlk, leftBytesInBlk);
		exit(1);
	}
	
	//��ǰ��־���Ƿ����㹻��������������С��recordHeader
	const bool canContainMinRecHeader = (leftBytesInBlk >= minRecordHeaderSize);
	bool isNeedSkipBlock = false;

	if(canContainMinRecHeader == false){
		isNeedSkipBlock = true;
	}else{
		log_record_header_minor minRecordHeader;
		memset(&minRecordHeader,0,sizeof(minRecordHeader));
		int bytesCopied = m_pRedoBlkOp->tryReadRecordHeader(&minRecordHeader, sizeof(minRecordHeader));
		if(bytesCopied != sizeof(minRecordHeader)){
			lgc_errMsg("tryReadRecordHeader failed \n");
			exit(1);
		}
		isNeedSkipBlock = !(minRecordHeader.record_length >= minRecordHeaderSize && minRecordHeader.record_length <= maxRecordSize);	
	}
	return isNeedSkipBlock;	
}

/*
*������һ�����ݿ�
*
*/
int LGC_RedoFile::skipToNextBlock()
{
	//skip to next blk
	const unsigned int blkNo = m_pRedoBlkOp->getBlkNo();
	const unsigned int leftBytesInBlk = m_pRedoBlkOp->getLeftBytesInBlk();
	const int bytesSkiped = m_pRedoBlkOp->skipSomeData(leftBytesInBlk);
	if(bytesSkiped != leftBytesInBlk){
		lgc_errMsg("skipSomeData failed \n");
		return -1;
	}
		
	const bool fileIsEnd = m_pRedoBlkOp->fileIsEnd();
	if(fileIsEnd){
		return bytesSkiped;
	}

	const unsigned int newBlkNo = m_pRedoBlkOp->getBlkNo();
	const unsigned int curOffsetInBlk = m_pRedoBlkOp->getCurOffsetInBlk();
	const unsigned int firstRecordOffsetInBlk = m_pRedoBlkOp->getFirstRecordOff();
	const unsigned int blkHeaderSize = m_pRedoBlkOp->getBlkHeaderSize();

	if(!lgc_check(newBlkNo == (blkNo+1)
					&& curOffsetInBlk == blkHeaderSize
					&& firstRecordOffsetInBlk == blkHeaderSize
					))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}

	return bytesSkiped;

}

//...................................................
//some get or set properties functions
//...................................................

LGC_IncompleteRedoRecordData* 
LGC_RedoFile::getFirstIncompleteRedoRecordData() const
{
	return NULL;
}

const RBA 
LGC_RedoFile::getCurRBA() const
{
	RBA rba = m_pRedoBlkOp->getCurRBA();
	return rba;
}

const unsigned int 
LGC_RedoFile::getThreadId() const
{
	return m_redoFileInfo.threadId;
}

//..........................................
//static member functions
//..........................................

/*
* ����RedoFile��ʵ��;
* 
* new ��RedoFileʵ������Ҫִ��һЩ��ʼ��������
* ��ʹ��ʵ���ܹ��ﵽΪ����ṩ�����״̬;
*/
LGC_RedoFile*
LGC_RedoFile::createRedoFile(const LGC_RedoFileInfo& redoFileInfo,OciQuery* pNormalQuery)
{
	int iFuncRet = 0;

	LGC_RedoFile*  pRedoFile  = NULL;
	LGC_RedoBlkOp* pRedoBlkOp = NULL;
	
	//�������߶���RedoBlkOp���� ���ڶ���־���е�����
	pRedoBlkOp = LGC_RedoBlkOp::createRedoBlkOp(redoFileInfo,pNormalQuery);
	if(pRedoBlkOp == NULL){
		lgc_errMsg("createRedoBlkOp failed \n");
		return NULL;
	}
	
	//���� RedoFile����
	pRedoFile = new LGC_RedoFile(redoFileInfo, pRedoBlkOp);
	if(pRedoFile == NULL){
		lgc_errMsg("new failed \n");
		return NULL;
	}
	pRedoBlkOp = NULL;
	
	//����RedoFile��FileHeader
	iFuncRet = pRedoFile->loadRedoFileHeader();
	if(iFuncRet < 0){
		lgc_errMsg("loadRedoFileHeader failed \n");
		goto errOut;
	}
	
	//����RedoFile��RedoHeader
	iFuncRet = pRedoFile->loadRedoHeader();
	if(iFuncRet < 0){
		lgc_errMsg("loadRedoHeader failed \n");
		goto errOut;
	}
	
	//���RedoFileBody�ĵ�һ��RedoRecord�����ݲ�����(������������һ��redoFile��)
	//��ȡ�ⲿ�����ݣ����浽RedoFile��m_pFirstIncompleteRecordData��
	//�Ա�����������һ��redoFile�е����ݺϲ���һ��������RedoRecord
	iFuncRet = pRedoFile->skipToFirstRecordPos();
	if(iFuncRet < 0){
		lgc_errMsg("skipToFirstRecordPos failed \n");
		goto errOut;
	}

	//success
	return pRedoFile;

errOut:
	if(pRedoFile ){
		delete pRedoFile;
		pRedoFile = NULL;
	}
	return NULL;
}




//.........................................
//tool classes for LGC_RedoFile
//.........................................


//.........................................
//struct LGC_IncompleteRedoRecordData
//.........................................


//.........................................
//class LGC_RedoBlkOp
//.........................................

/*
*����־���ж�����, �����ֶ����� �������ݿ�ͷ������ �� ��������־��ͷ������
* ��������־��ͷ������ʱ��ֻ�ܶ�һ����־������������־��һ���
* ������־��ͷ������ʱ���������ݿ��ܿ�����־��
*/

//........................................,,
//constructor and desctructor
//..........................................

LGC_RedoBlkOp::LGC_RedoBlkOp(const LGC_RedoFileInfo& redoFileInfo, LGC_ReadRedoFile* pReadRedoFile)
{
	m_redoFileInfo = redoFileInfo;
	m_pReadRedoFile = pReadRedoFile;
	
	const unsigned int blkSize = m_redoFileInfo.blkSize;
	m_blkBuf = new char[blkSize];
	if(m_blkBuf == NULL){
		exit(1);
	}
	memset(m_blkBuf,0,blkSize);
	m_blkBufRdPos = NULL;

	m_blkNo = 0;

	return;
}

LGC_RedoBlkOp::~LGC_RedoBlkOp()
{
	if(m_blkBuf){
		delete[] m_blkBuf;
		m_blkBuf = NULL;
	}

	if(m_pReadRedoFile){
		delete m_pReadRedoFile;
	}
	return;
}

//.........................................
//public member functions
//.........................................

/*
*������(��������־��ͷ)
*ֻ�ܶ�һ����־������������־��
*/
int LGC_RedoBlkOp::readDataNoSkipHd(void* buf, unsigned int size)
{
	//check: m_blkBuf != NULL && m_blkBufRdPos != NULL 
	//       && this->getBlkSize() == size
	if(!(m_blkBuf != NULL && m_blkBufRdPos != NULL && this->getBlkSize() == size)){
		lgc_errMsg("check failed \n");
		return -1;
	}

	//cpy data from m_blkBuf
	memcpy(buf,m_blkBuf,size);
	m_blkBufRdPos = m_blkBuf+size;

	//load next blk
	if(this->loadNextBlk() < 0){
		lgc_errMsg("loadNextBlk failed \n");
		return -1;
	}
	
	//success
	return size;
}

/*
*������(������־��ͷ������)
*���ݿ��ܿ�����־��
*/
int LGC_RedoBlkOp::readData(void* buf, unsigned int size)
{
	if(this->fileIsEnd()){
		return 0;
	}

	//check:  m_blkBuf != NULL && m_blkBufRdPos != NULL 
	//        && size > 0
	if(!(m_blkBuf != NULL && m_blkBufRdPos != NULL && size > 0)){
		lgc_errMsg("check failed \n");
		return -1;
	}

	const unsigned int blkSize = this->getBlkSize();
	const unsigned int blkBodySize = this->getBlkBodySize();

	unsigned int leftBytesInBlk = this->getLeftBytesInBlk();
	
	if(size <= leftBytesInBlk){
		return this->readDataFromCurBlk(buf,size);
	}

	const unsigned int mediaBlks = (size - leftBytesInBlk)/blkBodySize;
	const unsigned int bytesInLastBlk = (size - leftBytesInBlk)%blkBodySize;
	
	int bytesReaded = 0;
	unsigned int bytesTotalReaded = 0;
	unsigned int blkNo = 0;

	//read data from current blk
	bytesReaded = this->readDataFromCurBlk((char*)buf+bytesTotalReaded, leftBytesInBlk);
	if( !(bytesReaded == leftBytesInBlk)){
		return -1;
	}
	bytesTotalReaded += bytesReaded;
	
	//read data from media blks
	for(int i=0; i < mediaBlks; i++){
		bytesReaded = this->readDataFromCurBlk((char*)buf+bytesTotalReaded, blkBodySize);
		if(!(bytesReaded == 0 || bytesReaded == blkBodySize)){
			return -1;
		}
		if(bytesReaded == 0){
			return bytesTotalReaded;
		}
		bytesTotalReaded += bytesReaded;
	}

	//read data from last blk
	bytesReaded = this->readDataFromCurBlk((char*)buf+bytesTotalReaded, bytesInLastBlk);
	if(!(bytesReaded == 0 || bytesReaded == bytesInLastBlk)){
		return -1;
	}
	bytesTotalReaded += bytesReaded;

	//success
	return bytesTotalReaded;


	return 0;
}

/*
*����һЩ����
*
*/
int LGC_RedoBlkOp::skipSomeData(unsigned int size)
{
	if(this->fileIsEnd()){
		return 0;
	}

	//check:  m_blkBuf != NULL && m_blkBufRdPos != NULL 
	//        && size > 0
	if(!(m_blkBuf != NULL && m_blkBufRdPos != NULL && size > 0)){
		lgc_errMsg("check failed \n");
		return -1;
	}

	int iRet = 0;
	char* buf = new char[size];
	if(buf == NULL ){
		lgc_errMsg("new failed \n");
		exit(1);
	}

	iRet = this->readData(buf,size);

	//free
	delete[] buf;
	buf = NULL;

	return iRet;

}

/*
*��recordHeader������
*���ƶ���ȡλ��
*m_blkBuf��ʣ�µ�����һ���ܹ�����recordHeader������
*/
int LGC_RedoBlkOp::tryReadRecordHeader(void *buf, unsigned int size)
{
	if(this->fileIsEnd()){
		return 0;
	}

	//cpyRecordHeaderֻ�ܴӵ�ǰ��־���п������ݣ��Ҳ��ƶ���ȡλ��
	
	//check: m_blkBuf != NULL && m_blkBufRdPos != NULL 
	//       && size > 0 && this->getLeftBytesInBlk() >= size
	if(!(m_blkBuf != NULL && m_blkBufRdPos != NULL
		&& size > 0 && this->getLeftBytesInBlk() >= size))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}

	//cpy *���ƶ���ȡλ��
	memcpy(buf,m_blkBufRdPos,size);

	//success 
	return size;
}


//......................................
//private member functions
//......................................

/*
*��ʼ��
*����־�ļ��ĵ������ص�m_blkBuf
*/
int LGC_RedoBlkOp::init()
{
	//check: m_blkNo == 0 && m_blkBufRdPos == NULL
	if(!(m_blkNo == 0 && m_blkBufRdPos == NULL)){
		lgc_errMsg("check failed \n");
		return -1;
	}
	
	if (1 != this->readOneBlkFromFile(m_blkBuf,m_blkNo) ){
		lgc_errMsg("read failed \n");
		return -1;
	}

	//success
	m_blkBufRdPos = m_blkBuf + this->getBlkHeaderSize();

	return 0;
}

/*
*�ӵ�ǰ�������־���ж�ȡ����
*
*/
int LGC_RedoBlkOp::readDataFromCurBlk(void* buf, unsigned int size)
{
	if(this->fileIsEnd()){
		return 0;
	}

	const unsigned int blkSize = this->getBlkSize();
	const unsigned int blkHeaderSize = this->getBlkHeaderSize();
	
	//check: m_blkBuf && m_blkBufRdPos 
	//       && (m_blkBufRdPos-m_blkBuf >= blkHeaderSize && m_blkBufRdPos-m_blkBuf <= blkSize)
	if(!(m_blkBuf && m_blkBufRdPos 
		  && (m_blkBufRdPos-m_blkBuf >= blkHeaderSize && m_blkBufRdPos-m_blkBuf <= blkSize)))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}

	const unsigned int leftBytesInCurBlk = this->getLeftBytesInBlk();
	
	//check size <= leftBytesInCurBlk
	if(!(size <= leftBytesInCurBlk)){
		lgc_errMsg("check failed \n");
		return -1;	
	}

	memcpy(buf,m_blkBufRdPos,size);
	m_blkBufRdPos +=size;
	
	//�����ǰ�������־������ݶ�����
	//�򻺴���һ����־�������
	if(this->getLeftBytesInBlk() == 0){
		int blkLoaded = this->loadNextBlk();
		if(!(blkLoaded == 0 || blkLoaded == 1)){
			lgc_errMsg("loadNextBlk failed \n");
			return -1;
		}
	}
	
	//success
	return size;
}

/*
*������һ����־�鵽������
*/
int LGC_RedoBlkOp::loadNextBlk()
{
	int blksReaded = this->readOneBlkFromFile(m_blkBuf,m_blkNo+1);
	if(!(blksReaded == 0 || blksReaded == 1)){
		return -1;
	}
	
	//success
	if(blksReaded == 0){
		this->setFileEnd();
	}else{
		m_blkNo += blksReaded;
		m_blkBufRdPos = m_blkBuf + this->getBlkHeaderSize();
	}

	return blksReaded;
}

/*
*�������ļ��ж�ȡһ����־��
*/
int LGC_RedoBlkOp::readOneBlkFromFile(void* buf, unsigned int blkNo)
{
	const unsigned int totalBlks  = m_redoFileInfo.totalBlks;
	const unsigned int blksToRead = (blkNo <= totalBlks ? 1:0);

	if(blksToRead == 0){
		return 0;
	}

	int blksReaded = m_pReadRedoFile->readBlks((char*)buf,blkNo,blksToRead);
	if(!(blksReaded >=0 && blksReaded <= blksToRead)){
		lgc_errMsg("readBlks failed \n");
		return -1;
	}

	//success
	return blksReaded;

}

//some get or set properties functions
unsigned int LGC_RedoBlkOp::getBlkNo()
{
	//check: if m_blkNo != 0 then m_blkNo == ((log_block_header*)m_blkBuf)->block_id
	if( !(m_blkNo == 0 || m_blkNo == ((log_block_header*)m_blkBuf)->block_id) ){
		t_errMsg("check failed \n");
		exit(1);
	}
	return m_blkNo;
}
unsigned int LGC_RedoBlkOp::getSequence()
{
	//check: this->getBlkNo() == 0 || (log_block_header*)m_blkBuf->log_file_sequence == m_redoFileInfo.sequence
	if(!(this->getBlkNo() == 0 || ((log_block_header*)m_blkBuf)->log_file_sequence == m_redoFileInfo.sequence)){
		lgc_errMsg("check failed \n");
		exit(1);
	}

	return m_redoFileInfo.sequence;
}
unsigned int LGC_RedoBlkOp::getBlkSize()
{
	return m_redoFileInfo.blkSize;
}

unsigned int LGC_RedoBlkOp::getBlkHeaderSize()
{
	return sizeof(log_block_header);
}

unsigned int LGC_RedoBlkOp::getBlkBodySize()
{
		return (this->getBlkSize() - this->getBlkHeaderSize());
}

unsigned int LGC_RedoBlkOp::getFirstRecordOff()
{
	
	if(this->getBlkNo() == 0){
		lgc_errMsg("blkNO is 0 \n");
		exit(1);
	}

	return ((log_block_header*)m_blkBuf)->firstNewRecordOffset();
}
	
unsigned int LGC_RedoBlkOp::getCurOffsetInBlk()
{
	//check: m_blkBuf && m_blkBufRdPos 
	//       && ((m_blkBufRdPos - m_blkBuf) >= this->getBlkHeaderSize() && (m_blkBufRdPos - m_blkBuf) <= this->blkSize())
	
	if(this->fileIsEnd()){
		lgc_errMsg("fileIsEnd \n");
		exit(1);
	}

	return (m_blkBufRdPos - m_blkBuf);
}

unsigned int LGC_RedoBlkOp::getLeftBytesInBlk()
{
	return this->getBlkSize() - this->getCurOffsetInBlk();
}

const RBA LGC_RedoBlkOp::getCurRBA()
{
	RBA rba;
	if( this->fileIsEnd() ){
		memset(&rba,0,sizeof(RBA));
	}else{
		rba.sequence     = this->getSequence();
		rba.blockNo      = this->getBlkNo();
		rba.offsetInBlk  = this->getCurOffsetInBlk();
	}

	return rba;
}

bool  LGC_RedoBlkOp::fileIsEnd()
{
		return (m_blkBufRdPos == NULL);
}

void LGC_RedoBlkOp::setFileEnd()
{
	m_blkBufRdPos = NULL;
	return;
}


//.........................................................
//static member functions
//.........................................................

/*
*����RedoBlkOp��ʵ��
*
*/
LGC_RedoBlkOp* LGC_RedoBlkOp::createRedoBlkOp(const LGC_RedoFileInfo& redoFileInfo, OciQuery* pNormalQuery)
{
	bool bFuncRet = false;

	LGC_ReadRedoFile* pReadRedoFile = NULL;
	LGC_RedoBlkOp* pRedoBlkOp = NULL;
	
	//create ReadRedoFile��ʵ��
	pReadRedoFile = LGC_ReadRedoFile::createReadRedoFile(redoFileInfo,pNormalQuery);
	if(pReadRedoFile == NULL){
		goto errOut;
	}
	
	//create RedoBlkOp��ʵ��
	pRedoBlkOp = new LGC_RedoBlkOp(redoFileInfo,pReadRedoFile);
	if(pRedoBlkOp == NULL){
		goto errOut;
	}
	pReadRedoFile = NULL;
	
	//��ʼ�� RedoBlkOpen��ʵ��
	if(pRedoBlkOp->init() < 0 ){
		goto errOut;
	}

	//success
	return pRedoBlkOp;

errOut:
	if(pRedoBlkOp){
		delete pRedoBlkOp;
		pRedoBlkOp = NULL;
	}
	if(pReadRedoFile){
		delete pReadRedoFile;
		pReadRedoFile = NULL;
	}

	return NULL;
}



//................................................
//global static tool functions 
//................................................
static BYTE8 calculateSCN(unsigned short highSCN, unsigned int lowSCN)
{
	BYTE8 scn = 0;
	scn = highSCN;
	scn <<= 32;
	scn += lowSCN;

	return scn;
}


