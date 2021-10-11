#include "tw_rwDataBlocks.h"
#include "tw_ASM.h"
#include "tw_file.h"

///////////////////////////////class RWDataBlocks_base��ʵ�ֲ���////////////////////////////////////////
RWDataBlocks_base::RWDataBlocks_base()
{
	m_fileName=NULL;
	m_blkSize=0;
	return;
}
RWDataBlocks_base::~RWDataBlocks_base()
{
	if(m_fileName){
		delete[] m_fileName;
		m_fileName=NULL;
	}
	m_blkSize=0;

	return;
}

bool RWDataBlocks_base::setFileInfo(const RWInfoOfFile* pRWInfoOfFile)
{
	bool bRet=true;

	const char* fileName=pRWInfoOfFile->fileName;
	const int blkSize=pRWInfoOfFile->blkSize;
	const int totalBlks=pRWInfoOfFile->totalBlks;
	const int fileType=pRWInfoOfFile->fileType;
	
	//�������ļ���
	if(m_fileName){
		delete[] m_fileName;
		m_fileName=NULL;
	}
	m_fileName=new char[strlen(fileName)+1];
	if(!m_fileName){
		bRet=false;
		goto errOut;
	}
	strcpy(m_fileName,fileName);
	
	//�������ļ������ݿ��С
	m_blkSize=blkSize;
	//���������ļ��������ݿ���
	m_totalBlks=totalBlks;
	//���������ļ�������
	m_fileType=fileType;

errOut:
	return bRet;
}

int RWDataBlocks_base::readBlks(const int blkOffInFile,void* buf, const int blksToRdFromFile)
{
	int iRet=0;
errOut:
	return iRet;
}

int RWDataBlocks_base::writeBlks(const int blkOffInFile,void* buf, const int blksToWriteToFile)
{
	int iRet=0;

errOut:
	return iRet;
}
//////////////////////////////////////////////////////////////////////////////////////////////////




/////////////////////////////////////////////////class RWDataBlocks_commonFile��ʵ�ֲ���///////////////////////////////////////////
RWDataBlocks_commonFile::RWDataBlocks_commonFile()
{
	m_fd=-1;
	m_openFlag=0;
	return;
}

RWDataBlocks_commonFile::~RWDataBlocks_commonFile()
{
	if(m_fd>0){//�رմ򿪵������ļ�
		tw_close(m_fd);
		m_fd=-1;
	}

	return;
}


bool RWDataBlocks_commonFile::setFileInfo(const RWInfoOfFile* pRWInfoOfFile)
{
	bool bRet=true;
	const int openFlag=TW_OPEN_READ;
	bRet=RWDataBlocks_base::setFileInfo(pRWInfoOfFile);
	if(!bRet){
		bRet=false;
		goto errOut;
	}

	if(m_fd>0){//�رմ򿪵������ļ�
		tw_close(m_fd);
		m_fd=-1;
	}
	m_openFlag=0;

errOut:
	return bRet;
}

int RWDataBlocks_commonFile::readBlks(const int blkOffInFile, void* buf, const int blksToRdFromFile)
{
	int iRet= -1;
	UBYTE8 ull;
	BYTE8 bytesReaded;
	BYTE8 blksReaded;
	const BYTE8 blkSize=m_blkSize;
	const UBYTE8 byteOffInFile=blkOffInFile*blkSize;
	const BYTE8 bytesToRd = blksToRdFromFile*blkSize;

	const int readOpenFlag=TW_OPEN_READ;
	const char* fileName = m_fileName;
	

	//�Զ��ķ�ʽ�������ļ�
	if(m_fd>0 ){//�ļ��Ѿ���
		if(m_openFlag != readOpenFlag){ //�ļ��򿪵�ģʽ����
			tw_close(m_fd);
			m_fd=tw_open(fileName,readOpenFlag);
			if(m_fd<0){
				iRet=-1;
				goto errOut;
			}
		}
	}else{// �ļ���û��
		m_fd=tw_open(fileName,readOpenFlag);
		if(m_fd<0){
			iRet=-1;
			goto errOut;
		}
	}
	m_openFlag=readOpenFlag;

	//����дλ��
	ull=tw_lseek(m_fd,byteOffInFile,SEEK_SET);
	if(ull != byteOffInFile){
		iRet=-1;
		goto errOut;
	}

	//������
	bytesReaded=tw_read_times(m_fd,buf,bytesToRd);
	if(bytesReaded<0){
		iRet=-1;
		goto errOut;
	}else if(bytesReaded==0){
		iRet = 0;
		goto errOut;
	}

	blksReaded=bytesReaded/blkSize;
//	if(bytesReaded%blkSize){//��ȡ�����ݲ������ݿ��С�������� �Ƿ�
//		iRet=-1;
//		goto errOut;
//	}

	iRet=blksReaded;

errOut:
	return iRet;
}

int RWDataBlocks_commonFile::writeBlks(const int blkOffInFile, void* buf,const int blksToWriteToFile)
{
	int iRet= -1;
	UBYTE8 ull;
	BYTE8 bytesWrited;
	BYTE8 blksWrited;
	const BYTE8 blkSize=m_blkSize;
	const UBYTE8 byteOffInFile=blkOffInFile*blkSize;
	const BYTE8 bytesToWrite = blksToWriteToFile*blkSize;

	const int writeOpenFlag=TW_OPEN_WRITE;
	const char* fileName = m_fileName;
	
	//��д�ķ�ʽ�������ļ�
	if(m_fd>0 ){//�ļ��Ѿ���
		if(m_openFlag != writeOpenFlag){ //�ļ��򿪵�ģʽ����
			tw_close(m_fd);
	
			m_fd=tw_open(fileName,writeOpenFlag);
			if(m_fd<0){
				t_errMsg("��д�ķ�ʽ�������ļ�ʧ��: fileName=%s\n", fileName);
				iRet=-1;
				goto errOut;
			}
		}
	}else{// �ļ���û��
		m_fd=tw_open(fileName,writeOpenFlag);
		if(m_fd<0){
			t_errMsg("��д�ķ�ʽ�������ļ�ʧ��: fileName=%s\n", fileName);
			iRet=-1;
			goto errOut;
		}
	}
	m_openFlag=writeOpenFlag;
	
	//����дλ��
	ull=tw_lseek(m_fd,byteOffInFile,SEEK_SET);
	if(ull != byteOffInFile){
		t_errMsg("��λд��λ��ʧ��: fileName=%s bytesOff=%lld\n", fileName,ull);
		iRet=-1;
		goto errOut;
	}
	//д����
	bytesWrited=tw_write_times(m_fd,buf,bytesToWrite);
	if(bytesWrited != bytesToWrite){
		t_errMsg("д����: fileName=%s, bytesToWrite=%d bytesWrited=%d blksToWriteToFile=%d blkOffInFile=%s \n",fileName,bytesToWrite, bytesWrited,blksToWriteToFile,blkOffInFile);
		iRet=-1;
		goto errOut;
	}
	
	blksWrited=bytesWrited/blkSize;
	if(bytesWrited%blkSize){//д�����ݲ������ݿ��С�������� �Ƿ�
		t_errMsg("д�����ݲ������ݿ��С��������: bytesWrited=%d blkSize=%d\n",bytesWrited, blkSize);
		iRet=-1;
//		exit(1);
		goto errOut;
	}
	
	if(blksWrited != blksToWriteToFile){
		t_errMsg("blksWrited != blksToWriteToFile: blksWrited=%d blksToWriteToFile=%d\n",blksWrited, blksToWriteToFile);
		iRet = -1;
		goto errOut;
	}

	iRet=blksWrited;

errOut:
	return iRet;
}

void RWDataBlocks_commonFile::updateBlocksOfFile(const int blocks)
{
	return;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////RWDataBlocks_asmFile��ʵ�ֲ���//////////////////////////////////////////////////////////
OciQuery* RWDataBlocks_asmFile::s_pASMQuery_data = NULL;
OciQuery* RWDataBlocks_asmFile::s_pASMQuery_arch = NULL;

RWDataBlocks_asmFile::RWDataBlocks_asmFile(OciQuery* pASMQuery)
{
	m_pASM=new tw_ASM();
	if(!m_pASM){
		exit(1);
	}

	t_debugMsg(t_rwDataBlksLvl,"RWDataBlocks_asmFile::RWDataBlocks_asmFile\n");	
	return;
}


RWDataBlocks_asmFile::~RWDataBlocks_asmFile()
{
	if(m_pASM){
		delete m_pASM;
		m_pASM=NULL;
	}

	t_debugMsg(t_rwDataBlksLvl,"RWDataBlocks_asmFile::~RWDataBlocks_asmFile\n");
	t_errMsg("RWDataBlocks_asmFile::~RWDataBlocks_asmFile\n");
	return;
}


bool RWDataBlocks_asmFile::setFileInfo(const RWInfoOfFile* pRWInfoOfFile)
{
	bool bRet=true;
	bool isDataFile=false;
	m_pASMQuery = pRWInfoOfFile->pQuery;
	
	if(!m_pASMQuery){
		t_errMsg("pASMQuery=NULL\n");
		exit(1);
	}

	t_debugMsg(t_rwDataBlksLvl,"RWDataBlocks_asmFile::setFileInfo: %s\n",pRWInfoOfFile->fileName);
	//���û����������ļ���Ϣ
	bRet=RWDataBlocks_base::setFileInfo(pRWInfoOfFile);
	if(!bRet){
		goto errOut;
	}

	isDataFile = m_fileType==FILE_DATA_TYPE?true:false;

	//����ASM�ļ����е���Ϣ
	bRet=m_pASM->setASMFileInfo(m_fileName,m_blkSize, m_pASMQuery,m_fileType);
	if(!bRet){
		goto errOut;
	}

errOut:
	return bRet;
}

int RWDataBlocks_asmFile::readBlks(const int blkOffInFile, void* buf, const int blksToRdFromFile)
{
	int iRet=-1;
	int blksReaded;
	
	t_debugMsg(t_rwDataBlksLvl2,"RWDataBlocks_asmFile::readBlks start: fileName=%s blkOffInFile=%d blksToRdFromFile=%d blkSize=%d buf=%p\n",m_fileName, blkOffInFile, blksToRdFromFile, m_blkSize,buf);
	blksReaded = m_pASM->readBlksFromFile(blkOffInFile,buf,blksToRdFromFile,m_pASMQuery);
	if(blksReaded<0){
		iRet=-1;
		goto errOut;
	}
	t_debugMsg(t_rwDataBlksLvl2,"RWDataBlocks_asmFile::readBlks end: fileName=%s blkOffInFile=%d blksToRdFromFile=%d blkSize=%d buf=%p blksReaded=%d\n",m_fileName, blkOffInFile, blksToRdFromFile, m_blkSize,buf,blksReaded);
	iRet=blksReaded;
errOut:
	return iRet;
}

int RWDataBlocks_asmFile::writeBlks(const int blkOffInFile, void* buf, const int blksToWriteToFile)
{
	bool iRet=0;
	
errOut:
	return iRet;
}

void RWDataBlocks_asmFile::updateBlocksOfFile(const int blocks)
{
	return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////RWDataBlocks���ʵ��////////////////////////////////////////////////////////////////
RWDataBlocks::RWDataBlocks()
{
	m_fileName = NULL;
	m_totalBlks = 0;
	m_blkSize = 0;
	m_fileType = FILE_UNKOWN_TYPE;

	m_pRWDataBlocks_adapted= NULL;

	t_debugMsg(t_rwDataBlksLvl,"RWDataBlocks::RWDataBlocks\n");
	return;
}

RWDataBlocks::~RWDataBlocks()
{
	if(m_fileName){
		delete[] m_fileName;
		m_fileName = NULL;
	}

	if(m_pRWDataBlocks_adapted){
		delete m_pRWDataBlocks_adapted;
		m_pRWDataBlocks_adapted=NULL;
	}
	
	t_debugMsg(t_rwDataBlksLvl,"RWDataBlocks::~RWDataBlocks\n");
	return;
}

// virtual public

bool RWDataBlocks::setFileInfo(RWInfoOfFile *pRWInfoOfFile)
{
	bool bRet = true;
	
	char* fileName = pRWInfoOfFile->fileName;
	int blkSizeInFile = pRWInfoOfFile->blkSize;
	OciQuery* pASMQuery = pRWInfoOfFile->pQuery;
	
	bool isSameFileType = false;
	t_debugMsg(t_rwDataBlksLvl,"RWDataBlocks::setFileInfo: m_fileName=%s fileName=%s\n", !m_fileName?"NULL":m_fileName,fileName);

	//edit by tw 20140611
	if(m_fileName != NULL && m_fileName[0] == pRWInfoOfFile->fileName[0]){//����ǰ���Ǹ��ļ����ļ�������ͬ ����common�ļ�����asm�ļ�
		isSameFileType = true;
		t_debugMsg(t_rwDataBlksLvl,"RWDataBlocks::setFileInfo: is same file type with old\n");
	}else{//����ǰ���Ǹ��ļ����ļ����Ͳ�ͬ һ����common�ļ�����asm�ļ�
		isSameFileType = false;
		t_debugMsg(t_rwDataBlksLvl,"RWDataBlocks::setFileInfo: is not same file type with old\n");
	}
	


	//���û������ļ���Ϣ
	bRet=setFileInfo_(pRWInfoOfFile);
	if(!bRet){
		bRet=false;
		t_errMsg("setFileInfo error\n");
		goto errOut;
	}
	

	if(!isSameFileType){//����ǰ���ļ����Ͳ�ͬ edit by tw 20140611
//	if(true){//����ǰ���ļ����Ͳ�ͬ
		if(m_pRWDataBlocks_adapted){//ɾ����ǰ��
			delete m_pRWDataBlocks_adapted;
			m_pRWDataBlocks_adapted=NULL;
		}
		
		//t_errMsg("isSameFileType:%d\n",isSameFileType);
		if(fileName[0]=='+'){//�����ļ���asm�ļ�
			if(!pASMQuery){
				bRet = false;
				t_errMsg("pASMQuery is NULL\n");
				goto errOut;
			}
			m_pRWDataBlocks_adapted = new RWDataBlocks_asmFile(pASMQuery);
		}else{//�����ļ�����ͨ�����ļ�
			m_pRWDataBlocks_adapted = new RWDataBlocks_commonFile();
		}

		if(!m_pRWDataBlocks_adapted){
			bRet=false;
			t_errMsg("new failed\n");
			goto errOut;
		}
	}


	bRet=m_pRWDataBlocks_adapted->setFileInfo(pRWInfoOfFile);
	if(!bRet){
		bRet=false;
		t_errMsg("m_pRWDataBlocks_adapted\n");
		goto errOut;
	}


errOut:
	return bRet;
}

int RWDataBlocks::readBlks(const int blkOffInFile, void* buf, const int blksToRdFromFile)
{
	int iRet = -1;
//	const int totalBlksInFile = m_totalBlks;
//	const int blksCouldRead = totalBlksInFile-blkOffInFile;
//	const int blksToRdFromFile = blksCouldRead>blksToRdFromFile_? blksToRdFromFile_ : blksCouldRead;
	int blksReaded =0;
	bool b;
	
	int timesReaded = 0;
	const int rdTimesIfFailed = 5;

	t_debugMsg(t_rwDataBlksLvl2,"RWDataBlocks::readBlks start: fileName=%s blkOffInFile=%d blksToRdFromFile=%d block_size=%d buf=%p\n", m_fileName, blkOffInFile, blksToRdFromFile,m_blkSize,buf);
	//�����ݿ�
	
//	usleep(5);
	for(timesReaded=0; timesReaded < rdTimesIfFailed; timesReaded++){
		blksReaded=m_pRWDataBlocks_adapted->readBlks(blkOffInFile,buf, blksToRdFromFile);
		if(blksReaded<0){//�����ݿ����
			iRet = -1;
			t_errMsg("m_pRWDataBlocks_adapted->readBlks failed: fileName=%s blkOffInFile=%d blksToRdFromFile=%d\n",m_fileName,blkOffInFile,blksToRdFromFile);
			goto errOut;
		}
	
		t_debugMsg(t_rwDataBlksLvl2,"RWDataBlocks::readBlks checkBlock: fileName=%s blkOffInFile=%d blksToRdFromFile=%d block_size=%d buf=%p\n", m_fileName, blkOffInFile, blksToRdFromFile,m_blkSize,buf);
		//�Զ����������ݿ�ִ��checksum���
		b=doCheckBlocks(buf,blksReaded);
		if(!b){
			iRet = -1;
			t_errMsg("�Զ����������ݿ�ִ��checksum��� ʧ��: fileName=%s blkOffInFile=%d blksToRdFromFile=%d block_size=%d blksReaded=%d buf=%p\n", m_fileName, blkOffInFile, blksToRdFromFile,m_blkSize,blksReaded,buf);
			sleep(1);
			continue;
		}

		break; //success
	}

	t_debugMsg(t_rwDataBlksLvl2,"RWDataBlocks::readBlks end: fileName=%s blkOffInFile=%d blksToRdFromFile=%d block_size=%d buf=%p\n", m_fileName, blkOffInFile, blksToRdFromFile,m_blkSize,buf);

	iRet = blksReaded;
errOut:
	return iRet;
}

int RWDataBlocks::writeBlks(const int blkOffInFile, void* buf, const int blksToWriteToFile_)
{
	int iRet = -1;
	const int totalBlksInFile = m_totalBlks;
	const int blksCouldWrite = totalBlksInFile-blkOffInFile;
	const int blksToWriteToFile = blksCouldWrite>blksToWriteToFile_? blksToWriteToFile_ : blksCouldWrite;
	bool b = false;
	int blksWrited = 0;

	if(blkOffInFile + blksToWriteToFile_ == totalBlksInFile ){//д����Խ��
		t_wnMsg("RWDataBlocks::writeBlks����дԽ��: blkOffInFile=%d blksToWriteToFile_=%d totalBlksInFile=%d\n",blkOffInFile,blksToWriteToFile_, totalBlksInFile);
		iRet=-1;
		goto errOut;
	}

	if(blkOffInFile + blksToWriteToFile_ > totalBlksInFile ){//дԽ��
		t_errMsg("RWDataBlocks::writeBlksдԽ��: blkOffInFile=%d blksToWriteToFile_=%d totalBlksInFile=%d\n",blkOffInFile,blksToWriteToFile_, totalBlksInFile);
		iRet=-1;
		goto errOut;
	}

	//�Լ���д�������ļ������ݿ�ִ��checksum���
	b = doCheckBlocks(buf,blksToWriteToFile);
	if(!b){//���ʧ��
		iRet = -1;
		t_errMsg("�Լ���д�������ļ������ݿ�ִ��checksum��� ʧ��\n");
		goto errOut;
	}

	//��ͨ���������ݿ�д�������ļ���
	blksWrited=m_pRWDataBlocks_adapted->writeBlks(blkOffInFile, buf, blksToWriteToFile);
	if(blksWrited != blksToWriteToFile){//��ʧ��
		iRet = -1;
		t_errMsg("m_pRWDataBlocks_adapted->writeBlks failed: m_fileName=%s blkOffInFile=%d blksToWriteToFile=%d blksWrited=%d\n",m_fileName,blkOffInFile,blksToWriteToFile,blksWrited);
		goto errOut;
	}

	iRet = blksWrited;

errOut:
	return iRet;
	
}


//private
bool RWDataBlocks::doCheckBlocks(void* blksBuf, int blksToCheck)
{
	bool bRet = true;
	const int blkSize = m_blkSize;
	int i;
	int iTmp;
	BYTE *pBlk = NULL;

	pBlk = (BYTE*)blksBuf;
	for(i=0;i<blksToCheck;i++)
	{
		iTmp=do_checksum(blkSize, pBlk);
		if(iTmp){//checksum failed
			bRet = false;
			goto errOut;
		}
		
		pBlk += blkSize;
	}

	bRet=true;
errOut:
	return bRet;
}
bool RWDataBlocks::setFileInfo_(RWInfoOfFile* pRWInfoOfFile)
{
	bool bRet = true;
	int nameLen;

	if(m_fileName){
		delete[] m_fileName;
		m_fileName=NULL;
	}
	nameLen = strlen(pRWInfoOfFile->fileName);
	m_fileName = new char[nameLen+1];
	if(!m_fileName){
		exit(1);
	}
	memset(m_fileName,0,nameLen+1);
	strcpy(m_fileName,pRWInfoOfFile->fileName);

	m_totalBlks = pRWInfoOfFile->totalBlks;
	m_blkSize =   pRWInfoOfFile->blkSize;
	m_fileType =  pRWInfoOfFile->fileType;

errOut:
	return bRet;
}

void RWDataBlocks::updateBlocksOfFile(const int blocks)
{
	if( m_totalBlks > blocks){
		t_errMsg("blocks error:%d\n",blocks);
		return;
	}

	m_totalBlks = blocks;
	//m_pRWDataBlocks_adapted->updateBlocksOfFile(blocks);

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////










////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//��ʼ��RWDataBlocks_map�ľ�̬��Ա����

RWDataBlocks_map* RWDataBlocks_map::s_instance=NULL;
Mutex RWDataBlocks_map::s_mutex; 


RWDataBlocks_map::RWDataBlocks_map()
{
	return;
}

RWDataBlocks_map::~RWDataBlocks_map()
{
	mapIteratorType it;
	//free memory
	for(it=m_RWDataBlocks_baseMap.begin(); it != m_RWDataBlocks_baseMap.end(); it++)
	{
		if(it->second){
			delete it->second;
		}
		it->second=NULL;
	}

	return;
}

/*
��������:��map�л�ȡ��д�����ļ��Ķ���
����: fileNo--�����ļ���
���: ppRWDataBlocks--��д�����ļ��Ķ����ָ���ָ��
����: 
*/
bool RWDataBlocks_map::findRWDataBlocks(const int fileNo, RWDataBlocks** ppRWDataBlocks)
{
	bool bRet=true;
	mapIteratorType it;

	lock();
	it=m_RWDataBlocks_baseMap.find(fileNo);
	if(it == m_RWDataBlocks_baseMap.end()){//not find
		bRet=false;
		goto errOut;
	}
	//find
	
	if(!it->second){
		t_errMsg("RWDataBlocks_map: it->scond=NULL fileNo=%d\n",fileNo);
		exit(1);
	}

	*ppRWDataBlocks=it->second;
errOut:
	unlock();
	return bRet;
}

/*
*��������: ��map�в���һ����д�����ļ���RWDataBlocks����
*����: fileNo--�����ļ��ļ��� pRWDataBlocks--��д�����ļ��Ķ���
*���:
*����: true--�ɹ� false--ʧ��
*/
bool RWDataBlocks_map::insertRWDataBlocks(const int fileNo, RWDataBlocks* pRWDataBlocks)
{
	bool bRet=true;
	
	mapIteratorType it;
	mapSecondType mapSecond = NULL;
	mapInsertRetType mapInsertRet;

	bool isFind = true;

	lock();

	it=m_RWDataBlocks_baseMap.find(fileNo);
	if(it != m_RWDataBlocks_baseMap.end()){//find
		bRet=false;//find can't insert
		goto errOut;
	}
	//not find
	
	if(!pRWDataBlocks){
		bRet=false;
		t_errMsg("RWDataBlocks_map::insertRWDataBlocks: pRWDataBlocks=NULL\n");
		goto errOut;
	}
	
	mapSecond = pRWDataBlocks;
	mapInsertRet=m_RWDataBlocks_baseMap.insert(make_pair(fileNo, mapSecond));
	if(!mapInsertRet.second){//insert failed
		bRet=false;
		goto errOut;
	}

errOut:
	unlock();
	return bRet;
}

/*
*��������: ��ȡ��ʵ��ģʽ��Ψһʵ��
*����: ��ʵ��ģʽ��Ψһʵ��
*/
RWDataBlocks_map* RWDataBlocks_map::getInstance()
{
	if(!s_instance){
		
		lock();
		if(!s_instance){
			s_instance = new RWDataBlocks_map;
			if(!s_instance){
				t_errMsg("new failed\n");
				exit(1);
			}
		}
		unlock();
	}

	return s_instance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


