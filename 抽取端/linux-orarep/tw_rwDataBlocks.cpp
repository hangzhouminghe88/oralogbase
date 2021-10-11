#include "tw_rwDataBlocks.h"
#include "tw_ASM.h"
#include "tw_file.h"

///////////////////////////////class RWDataBlocks_base的实现部分////////////////////////////////////////
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
	
	//设数据文件名
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
	
	//设数据文件的数据块大小
	m_blkSize=blkSize;
	//设置数据文件的总数据块数
	m_totalBlks=totalBlks;
	//设置数据文件的类型
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




/////////////////////////////////////////////////class RWDataBlocks_commonFile的实现部分///////////////////////////////////////////
RWDataBlocks_commonFile::RWDataBlocks_commonFile()
{
	m_fd=-1;
	m_openFlag=0;
	return;
}

RWDataBlocks_commonFile::~RWDataBlocks_commonFile()
{
	if(m_fd>0){//关闭打开的数据文件
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

	if(m_fd>0){//关闭打开的数据文件
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
	

	//以读的方式打开数据文件
	if(m_fd>0 ){//文件已经打开
		if(m_openFlag != readOpenFlag){ //文件打开的模式不对
			tw_close(m_fd);
			m_fd=tw_open(fileName,readOpenFlag);
			if(m_fd<0){
				iRet=-1;
				goto errOut;
			}
		}
	}else{// 文件还没打开
		m_fd=tw_open(fileName,readOpenFlag);
		if(m_fd<0){
			iRet=-1;
			goto errOut;
		}
	}
	m_openFlag=readOpenFlag;

	//定读写位置
	ull=tw_lseek(m_fd,byteOffInFile,SEEK_SET);
	if(ull != byteOffInFile){
		iRet=-1;
		goto errOut;
	}

	//读数据
	bytesReaded=tw_read_times(m_fd,buf,bytesToRd);
	if(bytesReaded<0){
		iRet=-1;
		goto errOut;
	}else if(bytesReaded==0){
		iRet = 0;
		goto errOut;
	}

	blksReaded=bytesReaded/blkSize;
//	if(bytesReaded%blkSize){//读取的数据不是数据块大小的整数倍 非法
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
	
	//以写的方式打开数据文件
	if(m_fd>0 ){//文件已经打开
		if(m_openFlag != writeOpenFlag){ //文件打开的模式不对
			tw_close(m_fd);
	
			m_fd=tw_open(fileName,writeOpenFlag);
			if(m_fd<0){
				t_errMsg("以写的方式打开数据文件失败: fileName=%s\n", fileName);
				iRet=-1;
				goto errOut;
			}
		}
	}else{// 文件还没打开
		m_fd=tw_open(fileName,writeOpenFlag);
		if(m_fd<0){
			t_errMsg("以写的方式打开数据文件失败: fileName=%s\n", fileName);
			iRet=-1;
			goto errOut;
		}
	}
	m_openFlag=writeOpenFlag;
	
	//定读写位置
	ull=tw_lseek(m_fd,byteOffInFile,SEEK_SET);
	if(ull != byteOffInFile){
		t_errMsg("定位写的位置失败: fileName=%s bytesOff=%lld\n", fileName,ull);
		iRet=-1;
		goto errOut;
	}
	//写数据
	bytesWrited=tw_write_times(m_fd,buf,bytesToWrite);
	if(bytesWrited != bytesToWrite){
		t_errMsg("写出错: fileName=%s, bytesToWrite=%d bytesWrited=%d blksToWriteToFile=%d blkOffInFile=%s \n",fileName,bytesToWrite, bytesWrited,blksToWriteToFile,blkOffInFile);
		iRet=-1;
		goto errOut;
	}
	
	blksWrited=bytesWrited/blkSize;
	if(bytesWrited%blkSize){//写的数据不是数据块大小的整数倍 非法
		t_errMsg("写的数据不是数据块大小的整数倍: bytesWrited=%d blkSize=%d\n",bytesWrited, blkSize);
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


//////////////////////////////////////RWDataBlocks_asmFile的实现部分//////////////////////////////////////////////////////////
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
	//设置基本的数据文件信息
	bRet=RWDataBlocks_base::setFileInfo(pRWInfoOfFile);
	if(!bRet){
		goto errOut;
	}

	isDataFile = m_fileType==FILE_DATA_TYPE?true:false;

	//设置ASM文件特有的信息
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


///////////////////////////////////////////RWDataBlocks类的实现////////////////////////////////////////////////////////////////
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
	if(m_fileName != NULL && m_fileName[0] == pRWInfoOfFile->fileName[0]){//和以前的那个文件的文件类型相同 都是common文件或者asm文件
		isSameFileType = true;
		t_debugMsg(t_rwDataBlksLvl,"RWDataBlocks::setFileInfo: is same file type with old\n");
	}else{//和以前的那个文件的文件类型不同 一个是common文件或者asm文件
		isSameFileType = false;
		t_debugMsg(t_rwDataBlksLvl,"RWDataBlocks::setFileInfo: is not same file type with old\n");
	}
	


	//设置基本的文件信息
	bRet=setFileInfo_(pRWInfoOfFile);
	if(!bRet){
		bRet=false;
		t_errMsg("setFileInfo error\n");
		goto errOut;
	}
	

	if(!isSameFileType){//和以前的文件类型不同 edit by tw 20140611
//	if(true){//和以前的文件类型不同
		if(m_pRWDataBlocks_adapted){//删除以前的
			delete m_pRWDataBlocks_adapted;
			m_pRWDataBlocks_adapted=NULL;
		}
		
		//t_errMsg("isSameFileType:%d\n",isSameFileType);
		if(fileName[0]=='+'){//数据文件是asm文件
			if(!pASMQuery){
				bRet = false;
				t_errMsg("pASMQuery is NULL\n");
				goto errOut;
			}
			m_pRWDataBlocks_adapted = new RWDataBlocks_asmFile(pASMQuery);
		}else{//数据文件是普通数据文件
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
	//读数据块
	
//	usleep(5);
	for(timesReaded=0; timesReaded < rdTimesIfFailed; timesReaded++){
		blksReaded=m_pRWDataBlocks_adapted->readBlks(blkOffInFile,buf, blksToRdFromFile);
		if(blksReaded<0){//读数据块出错
			iRet = -1;
			t_errMsg("m_pRWDataBlocks_adapted->readBlks failed: fileName=%s blkOffInFile=%d blksToRdFromFile=%d\n",m_fileName,blkOffInFile,blksToRdFromFile);
			goto errOut;
		}
	
		t_debugMsg(t_rwDataBlksLvl2,"RWDataBlocks::readBlks checkBlock: fileName=%s blkOffInFile=%d blksToRdFromFile=%d block_size=%d buf=%p\n", m_fileName, blkOffInFile, blksToRdFromFile,m_blkSize,buf);
		//对读出来的数据块执行checksum检查
		b=doCheckBlocks(buf,blksReaded);
		if(!b){
			iRet = -1;
			t_errMsg("对读出来的数据块执行checksum检查 失败: fileName=%s blkOffInFile=%d blksToRdFromFile=%d block_size=%d blksReaded=%d buf=%p\n", m_fileName, blkOffInFile, blksToRdFromFile,m_blkSize,blksReaded,buf);
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

	if(blkOffInFile + blksToWriteToFile_ == totalBlksInFile ){//写可能越界
		t_wnMsg("RWDataBlocks::writeBlks可能写越界: blkOffInFile=%d blksToWriteToFile_=%d totalBlksInFile=%d\n",blkOffInFile,blksToWriteToFile_, totalBlksInFile);
		iRet=-1;
		goto errOut;
	}

	if(blkOffInFile + blksToWriteToFile_ > totalBlksInFile ){//写越界
		t_errMsg("RWDataBlocks::writeBlks写越界: blkOffInFile=%d blksToWriteToFile_=%d totalBlksInFile=%d\n",blkOffInFile,blksToWriteToFile_, totalBlksInFile);
		iRet=-1;
		goto errOut;
	}

	//对即将写书数据文件的数据块执行checksum检查
	b = doCheckBlocks(buf,blksToWriteToFile);
	if(!b){//检查失败
		iRet = -1;
		t_errMsg("对即将写书数据文件的数据块执行checksum检查 失败\n");
		goto errOut;
	}

	//将通过检查的数据块写到数据文件中
	blksWrited=m_pRWDataBlocks_adapted->writeBlks(blkOffInFile, buf, blksToWriteToFile);
	if(blksWrited != blksToWriteToFile){//是失败
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


//初始化RWDataBlocks_map的静态成员变量

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
函数功能:从map中获取读写数据文件的对象
输入: fileNo--数据文件号
输出: ppRWDataBlocks--读写数据文件的对象的指针的指针
返回: 
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
*函数功能: 向map中插入一个读写数据文件的RWDataBlocks对象
*输入: fileNo--数据文件文件号 pRWDataBlocks--读写数据文件的对象
*输出:
*返回: true--成功 false--失败
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
*函数功能: 获取单实例模式的唯一实例
*返回: 单实例模式的唯一实例
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


