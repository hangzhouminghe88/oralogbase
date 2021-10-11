#include "tw_ASM.h"
#include "skip_ASM.h"
#include "tw_api.h"


vector< vector<DISK> > G_Disks;
vector<Group> G_Groups;


void ASMFile::Clear()
{
	GroupNum		= -1;
	ASMFileIndex	= 0;
	AU_Size			= 0;
	BlockSize		= 0;
	MetaBlockSize	= 0;
	TotelAUCount	= 0;
	DirectRedundancy	= 0;
	InDirectRedundancy	= 0;
	FileSize = 0;
	Blocks = 0;
	bLogfile = 0;
	reload = true;
//	AUIndexs.clear();
	
	return;
}

ASMFile::ASMFile()
{
	Clear();
	reload = true;	// true时，内容要重新载入
}

tw_ASM::tw_ASM()
{
	m_pASMFile=new ASMFile;
	if(!m_pASMFile){
		exit(1);
	}
	m_pOneBlock=NULL;
	m_pGroup=NULL;
	m_pDisks=NULL;
	m_pASMAdapted = new skip_ASM;
	if(!m_pASMAdapted){
		exit(1);
	}
	m_fileAuOffset=0;
	m_totalBlksOfFile = 0;

	m_oneAuBuf = NULL;
	m_isDataFile = false;
	m_fileType = FILE_UNKOWN_TYPE;
}

tw_ASM::~tw_ASM()
{
	if(m_pOneBlock){
		delete[] m_pOneBlock;
		m_pOneBlock=NULL;
	}

	if(m_pASMAdapted){
		delete m_pASMAdapted;
		m_pASMAdapted = NULL;
	}

	if(m_oneAuBuf){
		delete[] m_oneAuBuf;
		m_oneAuBuf = NULL;
	}
}

bool tw_ASM::setASMFileInfo(const char* fileName, const int blkSize, OciQuery* pASMQuery, int fileType)
{
	bool bRet=true;

	int groupNum;
	int grpAuSize;
	int grpBlkSize;

	int fileNoInASM;
	int blkSizeInFile;

	t_debugMsg(t_rwDataBlksLvl,"tw_ASM::setASMFileInfo: %s\n", fileName);

	m_isDataFile = false;
	m_fileType = fileType;

	bRet=getASMFileInfo(fileName,pASMQuery);
	if(!bRet){
		goto errOut;
	}
	
	m_remainBlksInFile = m_pASMFile->Blocks;
	m_totalBlksOfFile = m_pASMFile->Blocks;

	groupNum = m_pGroup->GroupNum;
	grpAuSize = m_pGroup->AU_Size;
	grpBlkSize = m_pGroup->Block_Size;
	// 设置ASM文件所属的磁盘组的相关信息
	bRet=m_pASMAdapted->setGroupInfo(groupNum,*m_pDisks, grpAuSize,grpBlkSize);
	if(!bRet){
		goto errOut;
	}

	fileNoInASM = m_pASMFile->ASMFileIndex;
	blkSizeInFile = m_pASMFile->BlockSize;
	//  设置ASM文件的相关信息
	m_pASMAdapted->setASMDataFileInfo(fileNoInASM,blkSizeInFile, m_isDataFile);
	if(!bRet){
		goto errOut;
	}

	//判断文件中的数据块大小是否正确
	if(blkSizeInFile != blkSize){
		bRet = false;
		goto errOut;
	}

	if(!m_oneAuBuf){
		m_oneAuBuf = new char[grpAuSize];
		if(!m_oneAuBuf){
			t_errMsg("new error\n");
			bRet=false;
			exit(1);
			goto errOut;
		}
		memset(m_oneAuBuf,0,grpAuSize);
	}


errOut:
	return bRet;

}

/*
bool tw_ASM::setASMFileInfo(const char* fileName, const int blkSize, OciQuery* pASMQuery, bool isDataFile)
{
	bool bRet=true;

	int groupNum;
	int grpAuSize;
	int grpBlkSize;

	int fileNoInASM;
	int blkSizeInFile;

	t_debugMsg(t_rwDataBlksLvl,"tw_ASM::setASMFileInfo: %s\n", fileName);

	m_isDataFile = isDataFile;

	bRet=getASMFileInfo(fileName,pASMQuery);
	if(!bRet){
		goto errOut;
	}
	
	m_remainBlksInFile = m_pASMFile->Blocks;
	m_totalBlksOfFile = m_pASMFile->Blocks;

	groupNum = m_pGroup->GroupNum;
	grpAuSize = m_pGroup->AU_Size;
	grpBlkSize = m_pGroup->Block_Size;
	// 设置ASM文件所属的磁盘组的相关信息
	bRet=m_pASMAdapted->setGroupInfo(groupNum,*m_pDisks, grpAuSize,grpBlkSize);
	if(!bRet){
		goto errOut;
	}

	fileNoInASM = m_pASMFile->ASMFileIndex;
	blkSizeInFile = m_pASMFile->BlockSize;
	//  设置ASM文件的相关信息
	m_pASMAdapted->setASMDataFileInfo(fileNoInASM,blkSizeInFile, isDataFile);
	if(!bRet){
		goto errOut;
	}

	//判断文件中的数据块大小是否正确
	if(blkSizeInFile != blkSize){
		bRet = false;
		goto errOut;
	}

	if(!m_oneAuBuf){
		m_oneAuBuf = new char[grpAuSize];
		if(!m_oneAuBuf){
			t_errMsg("new error\n");
			bRet=false;
			exit(1);
			goto errOut;
		}
		memset(m_oneAuBuf,0,grpAuSize);
	}


errOut:
	return bRet;
}
*/

int tw_ASM::readBlksFromFile_arch(const int blkOffInFile, void* blksBuf, const int blksToRead,OciQuery* pASMQuery)
{
	int iRet = -1;
	bool b;
	int blksReaded = 0;
	const int totalBlksInFile = m_totalBlksOfFile;
	int blksRealToRd = 0;

	if(!m_isDataFile){//是归档文件
		blksRealToRd = blkOffInFile+blksToRead<=totalBlksInFile?blksToRead:(totalBlksInFile-blkOffInFile);
	}else{//是数据文件
		blksRealToRd = blksToRead;
	}

	t_debugMsg(t_rwDataBlksLvl2,"tw_ASM::readBlksFromFile start: blkOffInFile=%d blksToRead=%d totalBlksInFile=%d blksRealToRd=%d\n", blkOffInFile,blksToRead,totalBlksInFile,blksRealToRd);
	b=readBlksFromFile_skip(blkOffInFile,blksBuf,blksRealToRd,pASMQuery);
	if(!b){
		iRet = -1;
		goto errOut;
	}
	
	blksReaded = blksRealToRd;
	m_remainBlksInFile -=blksReaded;
	t_debugMsg(t_rwDataBlksLvl2,"tw_ASM::readBlksFromFile end: blkOffInFile=%d blksToRead=%d totalBlksInFile=%d blksReaded=%d \n", blkOffInFile,blksToRead,totalBlksInFile, blksReaded);
	iRet=blksReaded;

errOut:
	return iRet;
}

int tw_ASM::readBlksFromFile(const int blkOffInFile_, void* blksBuf, const int blksToRead_,OciQuery* pASMQuery)
{
	switch(m_fileType){
	case FILE_ARCHIVE_TYPE:
		return this->readBlksFromFile_arch(blkOffInFile_, blksBuf, blksToRead_,pASMQuery);
		break;
	case FILE_ONLINE_CURRENT_TYPE:
	case FILE_ONLINE_ACTIVE_TYPE:
	case FILE_ONLINE_DEACTIVE_TYPE:
		return this->readBlksFromFile_online(blkOffInFile_,blksBuf, blksToRead_,pASMQuery);
		//return this->readBlksFromFile_arch(blkOffInFile_, blksBuf, blksToRead_,pASMQuery);
		break;
	default:
		t_errMsg("tw_ASM::readBlksFromFile fileType invalid \n");
		return -1;
	}

}

void tw_ASM::getOnlineExtentInfo(const int blkOffInFile, int* pExtentNo, int* pAuNoInExtent, int* pChunckNoInAu, int *pBlkNoInChunck)
{
	const int auSize = m_pGroup->AU_Size;
	const int blkSize = m_pASMFile->BlockSize;
	const int extentSize = 8*auSize;
	const int chunckSize = 256*blkSize;
	const int blksInChunck = 256;

	int blksInAu = auSize/blkSize;
	int blksInExtent = extentSize/blkSize;
	int extentNo = blkOffInFile/blksInExtent;
	int blksRemainInExtent = blkOffInFile%blksInExtent;
	int auNoInExtent = (blksRemainInExtent%blksInAu)/blksInChunck;
	int chunckNoInAu = (blksRemainInExtent/blksInAu);
	int blkNoInChunck = (blksRemainInExtent%blksInChunck);

	*pExtentNo = extentNo;
	*pAuNoInExtent = auNoInExtent;
	*pChunckNoInAu = chunckNoInAu;
	*pBlkNoInChunck = blkNoInChunck;

	return;
}

int tw_ASM::readBlksInOnline_inSameChunck(char* buf, int extentNo, int auNoInExtent, int chunckNo, int blkOffInChunck, int blksToRead, OciQuery* pASMQuery)
{
	int iRet = blksToRead;	
	bool b = true;

	const int auSize = m_pGroup->AU_Size;
	const int blkSize = m_pASMFile->BlockSize;
	const int ausInExtent = 8;
	const int extentSize = 8*auSize;
	const int chunckSize = 256*blkSize;
	const int blksInChunck = 256;
	const int blksInAu = auSize/blkSize;
	const int blksInExtent = extentSize/blkSize;
	const int fileIndexInASM = m_pASMFile->ASMFileIndex;


	const int auOffInFile = extentNo*ausInExtent + auNoInExtent;

	if(chunckNo >= (auSize/chunckSize) || (blkOffInChunck>=blksInChunck) ||( (blkOffInChunck+blksToRead) > blksInChunck)){
		t_errMsg("tw_ASM::readBlksInOnline_inSameChunck argument invalid \n");
		return -1;
	}

	int blkOffInAu = chunckNo*blksInChunck+blkOffInChunck;
	int byteOffInAu = blkOffInAu*blkSize;
	int bytesToRead = blksToRead*blkSize;
	
	int diskNum = 0;
	int auNumInDisk = 0;
	b=getAuInfoInDisk(fileIndexInASM,auOffInFile,&diskNum,&auNumInDisk,pASMQuery);
	if(!b){
		iRet = -1;
		goto errOut;
	}
	
	b=m_pASMAdapted->readBytesFromDisk_inSameAu(diskNum,auNumInDisk,byteOffInAu, bytesToRead, buf);
	if(!b){
		iRet = -1;
		goto errOut;
	}
	iRet = blksToRead;

errOut:
	return iRet;
}

int tw_ASM::readBlksFromFile_online(const int _blkOffInFile, void* _blksBuf, const int blksToRead,OciQuery* pASMQuery)
{
	int iRet = blksToRead;
	int blkOffInFile = _blkOffInFile;
	char* blksBuf = (char*)_blksBuf;

	const int blkSize = m_pASMFile->BlockSize;


	int lastExtentNo,lastAuNoInExtent, lastChunckNoInAu, lastBlkNoInChunck;
	int extentNo,auNoInExtent, chunckNoInAu, blkNoInChunck;
	int startBlkNo, endBlkNo;
	int endBlkNoToRead = blkOffInFile+blksToRead;

	int i = 0;
	int blksReaded = 0;
	for(;blkOffInFile < endBlkNoToRead;){
		getOnlineExtentInfo(blkOffInFile,&lastExtentNo,&lastAuNoInExtent, &lastChunckNoInAu, &lastBlkNoInChunck);
		startBlkNo = blkOffInFile;
		for(;blkOffInFile < endBlkNoToRead;blkOffInFile++){
			getOnlineExtentInfo(blkOffInFile,&extentNo, &auNoInExtent, &chunckNoInAu,&blkNoInChunck);
			if( lastExtentNo != extentNo || lastAuNoInExtent != auNoInExtent || lastChunckNoInAu != chunckNoInAu){
				break;
			}
		}
		endBlkNo = blkOffInFile;

		if(endBlkNo <= startBlkNo){//invalid
			t_errMsg("endBlkNo <= startBlkNo \n");
			iRet = -1;
			goto errOut;
		}

		blksReaded = readBlksInOnline_inSameChunck(&blksBuf[(startBlkNo-_blkOffInFile)*blkSize], lastExtentNo, lastAuNoInExtent, lastChunckNoInAu, 
			lastBlkNoInChunck, endBlkNo-startBlkNo, pASMQuery);
		if(blksReaded != (endBlkNo-startBlkNo)){
			t_errMsg("readBlksInOnline_inSameChunck failed \n");
			iRet = -1;
			goto errOut;
		}
	}

errOut:
	return iRet;

}

bool tw_ASM::getASMFileInfo(const char* fileName, OciQuery* pASMQuery)
{
	bool bRet=true;
	string groupName;
	string fileBaseName;
	
	t_debugMsg(t_rwDataBlksLvl, "tw_ASM::getASMFileInfo start: fileName=%s pASMQuery=%p\n",fileName, pASMQuery);
	if(!fileName || fileName[0] != '+'){//数据文件不是ASM文件 非法
		cout<<"数据文件不是ASM文件"<<endl;
		t_errMsg("数据文件不是ASM文件\n");
		bRet=false;
		goto errOut;
	}
	
	clearASMFileInfo();
	//从数据文件的全路径中获取磁盘组名
	bRet=getGroupNameFromPath(fileName,groupName);
	if(!bRet){
		goto errOut;
	}
	
	//从数据文件的全路径中获取数据文件名
	bRet=getFileNameFromPath(fileName, fileBaseName);
	if(!bRet){
		goto errOut;
	}
	
	//根据磁盘组名设置磁盘组信息
	t_debugMsg(t_rwDataBlksLvl, "tw_ASM::setGroupInfoByGroupName start: groupName=%s pASMQuery=%p\n",groupName.c_str(), pASMQuery);
	bRet=setGroupInfoByGroupName(groupName.c_str());
	if(!bRet){
		goto errOut;
	}
	t_debugMsg(t_rwDataBlksLvl, "tw_ASM::setGroupInfoByGroupName end: groupName=%s pASMQuery=%p\n",groupName.c_str(), pASMQuery);
	
	//根据文件名设置文件的ASM信息
	t_debugMsg(t_rwDataBlksLvl, "tw_ASM::setASMInfoOfFileByName start: fileBaseName=%s pASMQuery=%p\n",fileBaseName.c_str(), pASMQuery);
	bRet=setASMInfoOfFileByName(fileBaseName.c_str(), pASMQuery);
	if(!bRet){
		goto errOut;
	}
	t_debugMsg(t_rwDataBlksLvl, "tw_ASM::setASMInfoOfFileByName end: fileBaseName=%s pASMQuery=%p\n",fileBaseName.c_str(), pASMQuery);
	
errOut:
	return bRet;
}




void tw_ASM::clearASMFileInfo()
{
	m_pASMFile->Clear();
	return;
}

bool tw_ASM::getGroupNameFromPath(const char* filePath, string& groupName)
{
	bool bRet = true;
	const char separator = '/';
	char* p;
	char* q;

	char* name = NULL;
	int nameLen;

	p=(char*)filePath;
	if(*p != '+'){
		bRet=false;
		goto errOut;
	}
	p++;

	q=strchr(p,separator);
	if(!q){
		bRet=false;
		goto errOut;
	}

	nameLen = q-p;
	name = new char[nameLen+1];
	if(!name){
		bRet=false;
		goto errOut;
	}
	memset(name,0,nameLen+1);
	strncpy(name,p,nameLen);

	groupName.clear();
	groupName=name;

errOut:
	if(name){
		delete[] name;
		name=NULL;
	}
	return bRet;
}

bool tw_ASM::getFileNameFromPath(const char* filePath, string& fileBaseName)
{
	bool bRet= true;

	const char separator = '/';
	char* p;

	p=strrchr(filePath,separator);
	if(!p){
		bRet=false;
		goto errOut;
	}
	++p;

	fileBaseName.clear();
	fileBaseName = p;


errOut:
	return bRet;
}

bool tw_ASM::setGroupInfoByGroupName(const char* groupName)
{
	bool bRet = true;
	int i;
	int groupsSize;
	int groupNum;
	
	groupsSize=G_Groups.size();
	for(i=0; i<groupsSize;i++){
		if(!strcmp(G_Groups[i].Name,groupName)){
			m_pGroup = &G_Groups[i];
			break;
		}
	}
	if(i>=groupsSize){// not found info of the disk group
		bRet=false;
		goto errOut;
	}
	
	//设置磁盘组里的磁盘信息
	groupNum = m_pGroup->GroupNum;
	m_pDisks = &G_Disks[groupNum];

errOut:
	return bRet;

}


bool tw_ASM::setASMInfoOfFileByName(const char* fileBaseName, OciQuery *pASMQuery)
{
	bool bRet=true;
	int i;
	int groupNum = 0;

	groupNum = m_pGroup->GroupNum;
	if(groupNum <= 0){
		t_errMsg("groupNum invalied:%d\n",groupNum);
		bRet = false;
		goto errOut;
	}
	
	t_debugMsg(t_rwDataBlksLvl, "tw_ASM::setASMInfoOfFileByName start: fileBaseName=%s groupNum=%d pASMQuery=%p\n",
		fileBaseName, groupNum, pASMQuery);

	i=pASMQuery->GetASMFileInfo2(m_pASMFile,(char*)fileBaseName,groupNum);
	if(i<0){
		bRet=false;
		goto errOut;
	}
	t_debugMsg(t_rwDataBlksLvl, "tw_ASM::setASMInfoOfFileByName end: fileBaseName=%s groupNum=%d pASMQuery=%p\n",
		fileBaseName, groupNum, pASMQuery);
errOut:
	return bRet;
}

bool tw_ASM::getGrpFirstFileEntry(const int groupNum, int* pDiskNum, int* pAuOffInDisk, int* pBlkOffInAu, OciQuery *pASMQuery)
{
	bool bRet = true;

errOut:
	return bRet;
}

bool tw_ASM::getAuInfoInDisk_byQuery(const int fileIndexInASM, const int auOffInFile, int* pDiskNum, int* pAuOffInDisk, OciQuery* pASMQuery)
{
	bool bRet = true;
	BindDatas bindDatas;
	int i;

	
	const int grpNum=m_pGroup->GroupNum;
	const int parNum = 3;

	bindDatas.clear();
	bindDatas.resize(parNum);
	bindDatas[0].DataInt = grpNum;
	bindDatas[0].Type = SQLT_INT;
	bindDatas[1].DataInt = fileIndexInASM;
	bindDatas[1].Type = SQLT_INT;
	bindDatas[2].DataInt = auOffInFile;
	bindDatas[2].Type = SQLT_INT;	

	if(pASMQuery->GetAUInfo2(m_pASMFile,bindDatas,parNum) < 0){ // 查询失败
		bRet = false;
		goto errOut;
	}

	*pDiskNum=m_pASMFile->curDiskNum;
	*pAuOffInDisk = m_pASMFile->curAuOffInDisk;
errOut:
	return bRet;
}


bool tw_ASM::getAuInfoInDisk_bySkip(const int fileIndexInASM, const int auOffInFile,  int* pDiskNum, int* pAuOffInDisk,OciQuery* )
{
	bool bRet = true;
	
	bRet=m_pASMAdapted->getAuInfoInDisk(fileIndexInASM,auOffInFile,pDiskNum,pAuOffInDisk);
	if(!bRet){
		goto errOut;
	}
	
errOut:
	return bRet;
}

/*
bool tw_ASM::readBlksFromFile_skip(const int blkOffInFile_, void* blksBuf, const int blksToRead_,OciQuery* pASMQuery)
{
	bool bRet = true;

	const BYTE8 blkOffInFile = blkOffInFile_;
	const BYTE8 blksToRead = blksToRead_;

	const BYTE8 fileIndexInASM = m_pASMFile->ASMFileIndex;
	const BYTE8 blkSizeInFile =  m_pASMFile->BlockSize;
	const BYTE8 grpAuSize = m_pGroup->AU_Size;
	const BYTE8 grpBlkSize = m_pGroup->Block_Size;
	const BYTE8 grpBlksInAu = grpAuSize/grpBlkSize;

	const BYTE8 readOffInFile = blkOffInFile*blkSizeInFile;
	const BYTE8 bytesToRead = blksToRead*blkSizeInFile;
	      BYTE8 auOffInFile = readOffInFile/grpAuSize;
	const BYTE8 readOffInAu = (readOffInFile%grpAuSize);
	const BYTE8 blkOffInAu = readOffInAu/grpBlkSize;
	const BYTE8 byteOffInGrpBlk = readOffInAu%grpBlkSize;
	
	const BYTE8 realReadOffInAu = readOffInAu-byteOffInGrpBlk;
	const BYTE8 realBytesToRead = (byteOffInGrpBlk+bytesToRead)%grpBlkSize?(byteOffInGrpBlk+bytesToRead)+(grpBlkSize-(byteOffInGrpBlk+bytesToRead)%grpBlkSize):(byteOffInGrpBlk+bytesToRead);

	const bool bNeedGrpBlksBuf = !byteOffInGrpBlk?(!((byteOffInGrpBlk+bytesToRead)%grpBlkSize)?false:true):true;

	const BYTE8 totalGrpBlksToRead = realBytesToRead/grpBlkSize;

	const BYTE8 maxGrpBlksToRdInFirstAu = grpBlksInAu-realReadOffInAu/grpBlkSize;
	const bool bBlksInSameAu = ((readOffInAu+bytesToRead)>grpAuSize)?false:true;
	const BYTE8 grpBlksToRdInFirstAu = !bBlksInSameAu? maxGrpBlksToRdInFirstAu :totalGrpBlksToRead;

	const BYTE8 grpBlksToRdInLastAu = !bBlksInSameAu?(totalGrpBlksToRead-grpBlksToRdInFirstAu)%grpBlksInAu:0;

	const BYTE8 mediaAusToRead = (totalGrpBlksToRead-grpBlksToRdInFirstAu-grpBlksToRdInLastAu)/grpBlksInAu;

	char* grpBlksBuf = NULL;
	int byteOffInGrpBlksBuf;

	int diskNum;
	int auNumInDisk;
	int diskNum2;
	int auNumInDisk2;
	
	
	int i;
	
	
	if( (totalGrpBlksToRead-grpBlksToRdInFirstAu-grpBlksToRdInLastAu)%grpBlksInAu || realBytesToRead%grpBlkSize || realReadOffInAu%grpBlkSize){
		bRet=false;
		goto errOut;

	}

//	t_debugMsg(t_rdDtFileLvl,"read ASM File: fileIndexInASM=%d\t blockOff=%d\t blocks=%d\t auOff=%d\t blkOffInAu=%d\t bytesToRead=%d\n",
//										m_pASMFile->ASMFileIndex,blkOffInFile,blksToRead,auOffInFile,blkOffInAu,realBytesToRead);
	if(bNeedGrpBlksBuf){
		grpBlksBuf = new char[realBytesToRead];
		if(!grpBlksBuf){
			bRet = false;
			exit(1);
			goto errOut;
		}
		memset(grpBlksBuf,0,realBytesToRead);
	}else{
		grpBlksBuf = (char*)blksBuf;
	}

	//读第一个au中的磁盘块
	byteOffInGrpBlksBuf = 0;

	bRet=getAuInfoInDisk(fileIndexInASM,auOffInFile,&diskNum,&auNumInDisk,pASMQuery);
	if(!bRet){
		goto errOut;
	}

	bRet=m_pASMAdapted->readBlocksFromDisk_InSameAu(diskNum,auNumInDisk,blkOffInAu,grpBlksToRdInFirstAu,grpBlksBuf+byteOffInGrpBlksBuf);
	if(!bRet){
		goto errOut;
	}
	byteOffInGrpBlksBuf += grpBlksToRdInFirstAu*grpBlkSize;
	
	//读中间的几个AU
	for(i=0;i<mediaAusToRead;i++){
		bRet=getAuInfoInDisk(fileIndexInASM,++auOffInFile,&diskNum,&auNumInDisk,pASMQuery);
		if(!bRet){
			goto errOut;
		}
		bRet=m_pASMAdapted->readOneAUFromDisk(diskNum,auNumInDisk,grpBlksBuf+byteOffInGrpBlksBuf);
		if(!bRet){
			goto errOut;
		}
		byteOffInGrpBlksBuf += grpAuSize;
	}

	//读最后一个au中的磁盘块
	if(grpBlksToRdInLastAu>0){
		bRet=getAuInfoInDisk(fileIndexInASM,++auOffInFile,&diskNum,&auNumInDisk,pASMQuery);
		if(!bRet){
			goto errOut;
		}
		bRet=m_pASMAdapted->readBlocksFromDisk_InSameAu(diskNum,auNumInDisk,0,grpBlksToRdInLastAu,grpBlksBuf+byteOffInGrpBlksBuf);
		if(!bRet){
			goto errOut;
		}
		byteOffInGrpBlksBuf += grpBlksToRdInLastAu*grpBlkSize;
	}

	if(bNeedGrpBlksBuf){
		memcpy(blksBuf,grpBlksBuf+byteOffInGrpBlk,bytesToRead);
	}

errOut:
	if(!bRet){
		t_errMsg("readBlocksFromFile 失败\n");
		bRet=false;
	}
	if(grpBlksBuf&&bNeedGrpBlksBuf){
		delete[] grpBlksBuf;
		grpBlksBuf=NULL;
	}
	return bRet;
}
*/


bool tw_ASM::readBlksFromFile_skip(const int blkOffInFile, void* blksBuf_, const int blksToRead,OciQuery* pASMQuery)
{
	bool bRet = true;

	const int fileIndexInASM = m_pASMFile->ASMFileIndex;
	const int blkSizeInFile =  m_pASMFile->BlockSize;
	const int grpAuSize = m_pGroup->AU_Size;

	const BYTE8 readOffInFile = ((BYTE8)blkOffInFile)*((BYTE8)blkSizeInFile);
	const int bytesToRead = blksToRead*blkSizeInFile;
	      int auOffInFile = readOffInFile/grpAuSize;
	const int readOffInAu = (readOffInFile%grpAuSize);

	const int ausNeedRead = (bytesToRead+readOffInAu)/grpAuSize;
	int bytesReaded = 0;
	int bytesToCpy = 0;

	char * blksBuf = (char*)blksBuf_;
	char * oneAuBuf = NULL;
	int mediaAusToRead = ausNeedRead > 0? (ausNeedRead-1):0;

	int diskNum;
	int auNumInDisk;
	
	int i;
	
	static int readedTimes=0;// should remove 
	static int newAuTimes=0; // shoud remove
	readedTimes++;

	//读第一个au中的磁盘块
	bytesReaded = 0;

	bRet=getAuInfoInDisk(fileIndexInASM,auOffInFile,&diskNum,&auNumInDisk,pASMQuery);
	if(!bRet){
		goto errOut;
	}

	bytesToCpy = ausNeedRead>0? (grpAuSize-readOffInAu):bytesToRead;
	bRet=m_pASMAdapted->readBytesFromDisk_inSameAu(diskNum,auNumInDisk,readOffInAu, bytesToCpy, blksBuf+bytesReaded);
	if(!bRet){
		goto errOut;
	}
	bytesReaded += bytesToCpy;
	
	if(ausNeedRead > 0){
		oneAuBuf = m_oneAuBuf;
		newAuTimes++;
	//	t_wnMsg("readedTimes=%d newAuTimes=%d\n", readedTimes, newAuTimes);

	}else{//don't need to continue,because have finished
		goto errOut;
	}

/*
	
	if(ausNeedRead > 0){
		oneAuBuf = new char[grpAuSize];
		if(!oneAuBuf){
			t_errMsg("new error");
			exit(1);
		}
		memset(oneAuBuf,0,grpAuSize);

		newAuTimes++;
		t_errMsg(" new oneAuBuf:fileIndexInASM=%d  blkOffInFile=%d auOffInFile=%d  blkSizeInFile=%d grpAuSize=%d readOffInFile=%lld bytesToRead=%d auOffInFile=%d readOffInAu=%d ausNeedRead=%d\n",
			fileIndexInASM,blkOffInFile, auOffInFile, blkSizeInFile, grpAuSize, readOffInFile, bytesToRead, auOffInFile, readOffInAu, ausNeedRead);
		t_errMsg("readedTimes=%d newAuTimes=%d\n", readedTimes, newAuTimes);

	}else{//don't need to continue,because have finished
		goto errOut;
	}
*/

	
	//读中间的几个AU
	for(i=0;i<mediaAusToRead;i++){

		bRet=getAuInfoInDisk(fileIndexInASM,++auOffInFile,&diskNum,&auNumInDisk,pASMQuery);
		if(!bRet){
			goto errOut;
		}
		bRet=m_pASMAdapted->readOneAUFromDisk(diskNum,auNumInDisk,oneAuBuf);
		if(!bRet){
			goto errOut;
		}
		
		bytesToCpy = grpAuSize;
		memcpy(blksBuf+bytesReaded, oneAuBuf, bytesToCpy);
		bytesReaded += bytesToCpy;
	}

	//读最后一个au中的磁盘块
	if(ausNeedRead>0){

		bRet=getAuInfoInDisk(fileIndexInASM,++auOffInFile,&diskNum,&auNumInDisk,pASMQuery);
		if(!bRet){
			goto errOut;
		}
		bRet=m_pASMAdapted->readOneAUFromDisk(diskNum,auNumInDisk,oneAuBuf);
		if(!bRet){
			goto errOut;
		}
	
		bytesToCpy = (bytesToRead+readOffInAu) - (ausNeedRead*grpAuSize);
		memcpy(blksBuf+bytesReaded, oneAuBuf, bytesToCpy);
		bytesReaded += bytesToCpy;

	}

errOut:
	if(!bRet){
		t_errMsg("readBlocksFromFile 失败\n");
		bRet=false;
	}

	return bRet;
}


bool tw_ASM::getAuInfoInDisk_check(const int fileIndexInASM, const int auOffInFile, int* pDiskNum, int* pAuOffInDisk, OciQuery* pASMQuery)
{
	bool bRet=true;
	
	int diskNum, auNumInDisk;
	int diskNum2,auNumInDisk2;

//	t_debugMsg(t_rwDataBlksLvl,"tw_ASM::getAuInfoInDisk_check start: fileIndexInASM=%d auOffInFile=%d \n",fileIndexInASM,auOffInFile);
	bRet=getAuInfoInDisk_bySkip(fileIndexInASM,auOffInFile,&diskNum,&auNumInDisk,pASMQuery);
	if(!bRet){
		t_errMsg("tgetAuInfoInDisk_bySkip failed\n");
		goto errOut;
	}

	bRet=getAuInfoInDisk_byQuery(fileIndexInASM,auOffInFile,&diskNum2,&auNumInDisk2,pASMQuery);
	if(!bRet){
		t_errMsg("getAuInfoInDisk_byQuery failed\n");
		goto errOut;
	}
	
//	t_debugMsg(t_rwDataBlksLvl,"tw_ASM::getAuInfoInDisk_check end: fileIndexInASM=%d auOffInFile=%d diskNum=%d diskNum2=%d auNumInDisk=%d auNumInDisk2=%d\n",fileIndexInASM,auOffInFile,diskNum,diskNum2,auNumInDisk,auNumInDisk2);

	if(diskNum!=diskNum2 || auNumInDisk != auNumInDisk2){
		bRet=false;
		t_errMsg("getAuInfoInDisk error: fileIndexInASM=%d auOffInFile=%d\n",fileIndexInASM,auOffInFile);
		t_errMsg("getAuInfoInDisk error: diskNum=%d diskNum2=%d auNumInDisk=%d auNumInDisk2=%d\n",diskNum,diskNum2,auNumInDisk,auNumInDisk2);
		tw_exit(1);
		goto errOut;
	}
	
	*pDiskNum = diskNum;
	*pAuOffInDisk = auNumInDisk;

errOut:
	return bRet;
}

bool tw_ASM::getAuInfoInDisk_noCheck(const int fileIndexInASM, const int auOffInFile, int* pDiskNum, int* pAuOffInDisk, OciQuery* pASMQuery)
{
	bool bRet=true;

	bRet=m_pASMAdapted->getAuInfoInDisk(fileIndexInASM,auOffInFile,pDiskNum,pAuOffInDisk);
	if(!bRet){
		goto errOut;
	}

errOut:
	return bRet;
}


bool tw_ASM::getAuInfoInDisk(const int fileIndexInASM, const int auOffInFile, int* pDiskNum, int* pAuOffInDisk, OciQuery* pASMQuery)
{
	bool bRet=true;

	DWORD dbCompat = m_pASMAdapted->getDbcompat();
	BYTE stripeWidth = m_pASMAdapted->getStripeWidth();
	int  stripeDepth = m_pASMAdapted->getStripeDepth();

	DWORD dXsiz[3];	// 
	DWORD dasz[4];	// 2^dasz  幂函数

	int extentOffInFile = 0;
	int auOffInExtent = 0;
	int externSize = ExtentSIZE;

	int ausInExtent ;
	int extentsInStripe ;
	int ausInStripe ;
	int ausInExtend1;
	int ausInExtend2;

//	int extentOffInFile = 0;

	if(dbCompat == DISK_COMPAT_10_1){
			extentOffInFile = auOffInFile;
			auOffInExtent = 0;

	}else if(dbCompat == DISK_COMPAT_11_1){
			t_errMsg("暂时不支持11.1版本\n");
			bRet = false;
			goto errOut;
   }else if(dbCompat == DISK_COMPAT_11_2){
			
			if(auOffInFile < externSize){	//小于20000 一个extent对应一个AU 条带深度假定为一个AU
				extentOffInFile = auOffInFile;
				auOffInExtent = 0;

			}else if( auOffInFile < (externSize + externSize*KFDASZ_4X) ){ // [20000,20000+20000*4) 一个extent对应4个AU 条带深度假定为一个AU
				
				ausInExtent = KFDASZ_4X;
				extentsInStripe = stripeWidth;
				ausInStripe = ausInExtent * stripeWidth;
				
				ausInExtend1 = (auOffInFile - externSize);
				ausInExtend1 = 0;

				extentOffInFile = externSize + ( ( ausInExtend1/ausInStripe ) * extentsInStripe ) + ausInExtend1%extentsInStripe;
				auOffInExtent = ausInExtend1%ausInStripe/extentsInStripe;

			}else{//auOffInFile >= 20000+20000*4
			
				ausInExtent = KFDASZ_16X;
				extentsInStripe = stripeWidth;
				ausInStripe = ausInExtent * stripeWidth;

				ausInExtend1 = externSize*KFDASZ_4X;
				ausInExtend2 = auOffInFile - (externSize + externSize*KFDASZ_4X);

				extentOffInFile = (externSize + externSize*KFDASZ_4X) + ( ( ausInExtend2/ausInStripe ) * extentsInStripe ) + ausInExtend2%extentsInStripe;
				auOffInExtent = ausInExtend2%ausInStripe/extentsInStripe;

			}
	}


	bRet=getAuInfoInDisk_noCheck(fileIndexInASM,extentOffInFile,pDiskNum,pAuOffInDisk,pASMQuery);
	if(!bRet){
		t_errMsg("getAuInfoInDisk_noCheck failed\n");
		goto errOut;
	}
	
	*pAuOffInDisk = *pAuOffInDisk + auOffInExtent;

errOut:
	return bRet;
}



//bool tw_ASM::getAuInfoInDisk(const int fileIndexInASM, const int auOffInFile, int* pDiskNum, int* pAuOffInDisk, OciQuery* pASMQuery)
//{
//	bool bRet=true;
	
	
//	t_debugMsg(t_rwDataBlksLvl,"tw_ASM::getAuInfoInDisk start: fileIndexInASM=%d auOffInFile=%d \n",fileIndexInASM,auOffInFile);
	
/*
	bRet=getAuInfoInDisk_check(fileIndexInASM,auOffInFile,pDiskNum,pAuOffInDisk,pASMQuery);
	if(!bRet){
		t_errMsg("getAuInfoInDisk_check failed\n");
		goto errOut;
	}
*/

//	bRet=getAuInfoInDisk_noCheck(fileIndexInASM,auOffInFile,pDiskNum,pAuOffInDisk,pASMQuery);
//	if(!bRet){
//		t_errMsg("getAuInfoInDisk_noCheck failed\n");
//		goto errOut;
//	}

//	t_debugMsg(t_rwDataBlksLvl,"tw_ASM::getAuInfoInDisk end: fileIndexInASM=%d auOffInFile=%d disNum=%d auOffInDisk=%d\n",fileIndexInASM,auOffInFile, *pDiskNum, *pAuOffInDisk);
	

/*
	bRet=getAuInfoInDisk_byQuery(fileIndexInASM,auOffInFile,pDiskNum,pAuOffInDisk,pASMQuery);
	if(!bRet){
		goto errOut;
	}
*/


//errOut:
//	return bRet;
//}
