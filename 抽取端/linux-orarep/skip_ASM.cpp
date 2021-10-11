#include "skip_ASM.h"
#include "tw_api.h"
#include "tw_file.h"



Mutex skip_ASM::s_readDisk_mutex;

skip_ASM::skip_ASM()
{
	m_grp_num=-1;
	m_grp_blockSize=0;
	m_grp_auSize=0;
	m_grp_redundancy=0;
	m_grp_pDisks=NULL;

	m_grp_dbcompat = 0;

	m_1ASMFile_diskNum = 0;
	m_1ASMFile_auNum = 2;
	m_1ASMFile_blockNum = 1;

	m_1ASMFile_1BlockBuf = NULL;
	m_1ASMFile_1BlockLoaded = false;

	m_asmDataFile_num = 0;
	m_asmDataFile_blkSize=0;
	m_asmDataFile_isDataFile=false;
	m_asmDataFile_directBlkBuf = NULL;

	m_isDirectOpen = true;
	m_twoAuBuf_forDrctRW = NULL;
		
}

skip_ASM::~skip_ASM()
{
		
	if(m_1ASMFile_1BlockBuf){
		delete[] m_1ASMFile_1BlockBuf;
		m_1ASMFile_1BlockBuf = NULL;
	}
		
	if(m_asmDataFile_directBlkBuf){
		delete[] m_asmDataFile_directBlkBuf;
		m_asmDataFile_directBlkBuf= NULL;
	}

	if(m_twoAuBuf_forDrctRW){
		delete[] m_twoAuBuf_forDrctRW;
		m_twoAuBuf_forDrctRW = NULL;
	}

	closeAllDisks();
	if(m_grp_pDisks){
		delete m_grp_pDisks;
		m_grp_pDisks = NULL;
	}


	
//	m_grp_pDisks = NULL;//指向全局G_Disks的一个成员的指针
		

}

bool skip_ASM::setGroupInfo(int grp_num,const vector<DISK> &disks, int auSize,int blockSize,int oneDiskNum, int oneAuNum, int oneBlkNum)
{

	bool bRet = true;
	
	//判断磁盘组信息是否已近设过了
	if(grp_num == m_grp_num){
		bRet = true;
		t_debugMsg(t_rwDataBlksLvl,"GroupInfo have seted so don't set again: groupNum=%d\n", grp_num);
		goto errOut;
	}
	
	t_debugMsg(t_rwDataBlksLvl,"skip_ASM::setGroupInfo: groupNum=%d\n", grp_num);
	t_errMsg("skip_ASM::setGroupInfo: groupNum=%d\n", grp_num);

	//检验磁盘组里的磁盘是否合法
	if(disks.size()<=0 || disks[0].GroupNum != grp_num){//disks invalid
		bRet = false;
		t_errMsg("disks invalid:disks.size()=%d disks[0].GroupNum=%d grp_num=%d \n", disks.size(),disks[0].GroupNum,grp_num);
		goto errOut;
	}

	//设置磁盘组的入口au信息
	t_debugMsg(t_rwDataBlksLvl,"skip_ASM::setOneASMFileEntry: groupNum=%d oneDiskNum=%d oneAuNum=%d oneBlkNum=%d \n", grp_num, oneDiskNum,oneAuNum,oneBlkNum);
	bRet=setOneASMFileEntry(oneDiskNum,oneAuNum,oneBlkNum);
	if(!bRet){// 设置一号ASM文件的入口失败
		t_errMsg("setOneASMFileEntry failed\n");
		bRet = false;
		goto errOut;
	}

	//设置磁盘组相关信息
	m_grp_num = grp_num;
	m_grp_auSize = auSize;
	m_grp_blockSize = blockSize;
	//设置磁盘组里的磁盘信息
	bRet=setGroupDisks(disks);
	if(!bRet){
		bRet = false;
		goto errOut;
	}

	//清空一号文件的入口磁盘块
	if(m_1ASMFile_1BlockBuf){
		delete[] m_1ASMFile_1BlockBuf;
		m_1ASMFile_1BlockBuf = NULL;
	}
	m_1ASMFile_1BlockBuf = new char[m_grp_blockSize];
	if(!m_1ASMFile_1BlockBuf){
		t_errMsg("new error\n");
		bRet = false;
		exit(1);
		goto errOut;
	}
	memset(m_1ASMFile_1BlockBuf,0,m_grp_blockSize);

	//给数据文件的入口块分配内存
	if(m_asmDataFile_directBlkBuf){
		delete[] m_asmDataFile_directBlkBuf;
		m_asmDataFile_directBlkBuf = NULL;
	}
	m_asmDataFile_directBlkBuf = new char[m_grp_blockSize];
	if(!m_asmDataFile_directBlkBuf){
		t_errMsg("new error\n");
		bRet = false;
		exit(1);
		goto errOut;
	}
	memset(m_asmDataFile_directBlkBuf,0,m_grp_blockSize);
	
	//分配两个AU的缓存
	if(m_twoAuBuf_forDrctRW){
		delete[] m_twoAuBuf_forDrctRW;
		m_twoAuBuf_forDrctRW=NULL;
	}
	m_twoAuBuf_forDrctRW = new char[2*m_grp_auSize];
	if(!m_twoAuBuf_forDrctRW){
		bRet = false;
		exit(1);
		goto errOut;
	}
	memset(m_twoAuBuf_forDrctRW,0,2*m_grp_auSize);


	//如果磁盘以O_Direct方式打开 需要内存对齐 大小为two au
	if(m_isDirectOpen){
		
	}

	//磁盘组的一号ASM文件的入口磁盘块
	bRet= loadOneMetaFileEntryBlock();
	if(!bRet){
		t_errMsg("loadOneMetaFileEntryBlock failed\n");
		bRet = false;
		goto errOut;
	}
	
	bRet=loadDiskHeaderInfo();
	if(!bRet){
		bRet = false;
		t_errMsg("loadDiskHeaderInfo failed\n");
		goto errOut;
	}

	bRet=loadOneMetaFileHeaderInfo();
	if(!bRet){
		bRet = false;
		t_errMsg("loadOneMetaFileHeaderInfo failed\n");
		goto errOut;
	}


	//将磁盘组一号文件的入口磁盘块转化为一号文件的au分布信息
//	m_1ASMFile_ausVec.clear();//清空一号文件的ausVec
//	bRet=drctBlkToAusVec(m_1ASMFile_1BlockBuf,m_1ASMFile_ausVec);
//	if(!bRet){
//		t_errMsg("drctBlkToAusVec failed: fileIndexInASM=%d\n", 1);
//		goto errOut;
//	}
//	t_debugMsg(t_rwDataBlksLvl,"skip_ASM::drctBlkToAusVec: asmFileIndex=%d\n", 1);


errOut:
	if(!bRet){
		bRet=false;
	}
	return bRet;
}

bool skip_ASM::setGroupDisks(const vector<DISK> &disks)
{
	bool bRet = true;
	int i;
	if(!m_grp_pDisks){//disks info not be seted
		m_grp_pDisks = new vector<DISK>(disks);
		if(!m_grp_pDisks){
			t_errMsg("new error\n");
			bRet = false;
			exit(1);
			goto errOut;
		}

		for(i=0;i<m_grp_pDisks->size(); i++){
			m_grp_pDisks->at(i).DiskFd = -1;
			m_grp_pDisks->at(i).Openflag = 0;
		}

	}

errOut:
	if(!bRet){
		bRet=false;
	}
	return bRet;
}

bool skip_ASM::loadOneMetaFileEntryBlock()
{
	bool bRet = true;
	//读数据
	bRet=readBlocksFromDisk_InSameAu(m_1ASMFile_diskNum,m_1ASMFile_auNum,m_1ASMFile_blockNum,1,m_1ASMFile_1BlockBuf);
	if(!bRet){
		t_errMsg("readBlocksFromDisk_InSameAu failed:m_1ASMFile_diskNum=%d m_1ASMFile_auNum=%d m_1ASMFile_blockNum=%d\n", m_1ASMFile_diskNum,m_1ASMFile_auNum,m_1ASMFile_blockNum);
		bRet = false;
		goto errOut;
	}

errOut:
	if(!bRet){
		bRet=false;
	}
	return bRet;
}

bool skip_ASM::setOneASMFileEntry(int diskNum, int auNum, int blockNum){
	bool bRet = true;
	m_1ASMFile_diskNum = diskNum;
	m_1ASMFile_auNum = auNum;
	m_1ASMFile_blockNum = blockNum;
errOut:
	if(!bRet){
		bRet=false;
	}
	return bRet;
}

bool skip_ASM::setASMDataFileInfo(int fileIndexInASM, int blockSizeInFile,bool isDataFile,bool bReset)
{
	bool bRet = true;
	int auOffIn1File;
	int blkOffInAu;
	int blkByteOff;
	int filesInAu;
	int diskNum;
	int auNumInDisk;
	char *auBuf = NULL;


	const BYTE8 grpAuSize = m_grp_auSize;
	const BYTE8 grpBlkSize = m_grp_blockSize;

	//判断asm文件信息是否已经设置过了
	bReset=true; //需要重新加载一号文件的au分布信息
	if(!bReset&&fileIndexInASM == m_asmDataFile_num){//已经设置过了
		t_debugMsg(t_rwDataBlksLvl,"file info have seted don't need set: asmFileIndex=%d bReset=%d\n", fileIndexInASM,bReset);
		bRet = true;
		goto errOut;
	}
	t_debugMsg(t_rwDataBlksLvl,"skip_ASM::setASMDataFileInfo: asmFileIndex=%d bReset=%d\n", fileIndexInASM,bReset);

	//如果需要重新加载磁盘组的一号文件的au分布信息
	if(bReset){
		bRet=loadOneMetaFileEntryBlock();
		if(!bRet){
			t_errMsg("loadOneMetaFileEntryBlock failed\n");
			bRet = false;
			goto errOut;
		}

//		m_1ASMFile_ausVec.clear();
//		bRet=drctBlkToAusVec(m_1ASMFile_1BlockBuf,m_1ASMFile_ausVec);
//		if(!bRet){
//			t_errMsg("drctBlkToAusVec failed: fileIndexInASM=%d\n",1);
//			goto errOut;
//		}
//		t_debugMsg(t_rwDataBlksLvl,"skip_ASM::drctBlkToAusVec: asmFileIndex=%d bReset=%d\n", fileIndexInASM,bReset);
	}


	//文件的asm索引号
	m_asmDataFile_num = fileIndexInASM;
	// 文件的中的逻辑数据块大小
	m_asmDataFile_blkSize = blockSizeInFile;
	//该ASM文件是数据文件还是归档文件
	m_asmDataFile_isDataFile=isDataFile;
	
	//给数据文件的入口块分配内存
	//m_asmDataFile_directBlkBuf 已经在setGroupInfo中分配
	
	//加载普通ASM文件的au分部信息
	bRet=loadDataFileAusVec(fileIndexInASM);
	if(!bRet){
		t_errMsg("loadDataFileAusVec failed: fileIndexInASM=%d\n",fileIndexInASM);
		goto errOut;
	}

	t_debugMsg(t_rwDataBlksLvl,"skip_ASM::loadDataFileAusVec: asmFileIndex=%d bReset=%d\n", fileIndexInASM,bReset);

errOut:
	if(auBuf){
		delete[] auBuf;
		auBuf=NULL;
	}
	if(!bRet){
		bRet=false;
	}
	return bRet;
}

//加载普通ASM文件的au分部信息 没有优化
bool skip_ASM::loadDataFileAusVec(int fileIndexInASM)
{
	return loadDataFileAusVec_optimize(fileIndexInASM);
}

bool skip_ASM::loadDataFileAusVec_optimize(int fileIndexInASM)
{
	bool bRet=true;
	const bool isDataFile=m_asmDataFile_isDataFile;

	if(isDataFile){//普通ASM文件时数据文件 这里需要优化

		bRet=DataFilesAuDistribute::getInstance()->findAuInfoOfFile(fileIndexInASM,&m_asmDataFile_ausVec);
		if(!bRet){//如果在缓存中没找到 则从磁盘中直接读
			
			t_debugMsg(t_rwDataBlksLvl,"如果在缓存中没找到 则从磁盘中直接读: asmFileIndex=%d isDataFile=%d\n", fileIndexInASM,isDataFile);
			//加载ASM文件的入口磁盘块
			bRet=loadDataFileEntryBlock(fileIndexInASM);
			if(!bRet){
				t_errMsg("loadDataFileEntryBlock failed: fileIndexInASM=%d\n",fileIndexInASM);
				goto errOut;
			}
			//将ASM文件的入口磁盘块转化为ASM文件的au分布信息
			m_asmDataFile_ausVec.clear();
			bRet = drctBlkToAusVec(m_asmDataFile_directBlkBuf,m_asmDataFile_ausVec);
			if(!bRet){
				t_errMsg("drctBlkToAusVec failed: fileIndexInASM=%d\n",fileIndexInASM);
				goto errOut;
			}
			t_debugMsg(t_rwDataBlksLvl,"skip_ASM::drctBlkToAusVec loadDataFileAusVec_optimize: asmFileIndex=%d isDataFile=%d\n", fileIndexInASM,isDataFile);

			//将从磁盘中读的数据加载到缓存中
			bRet=DataFilesAuDistribute::getInstance()->insertAuInfoOfFile(fileIndexInASM,&m_asmDataFile_ausVec);
			if(!bRet){
				t_errMsg("DataFilesAuDistribute::getInstance()->insertAuInfoOfFile failed: fileIndexInASM=%d\n",fileIndexInASM);
				goto errOut;
			}
		}
	}else{//普通ASM文件是归档文件 这里无需优化
		//加载ASM文件的入口磁盘块
		t_debugMsg(t_rwDataBlksLvl,"普通ASM文件是归档文件 这里无需优化: asmFileIndex=%d isDataFile=%d\n", fileIndexInASM,isDataFile);
		bRet=loadDataFileEntryBlock(fileIndexInASM);
		if(!bRet){
			t_errMsg("loadDataFileEntryBlock failed: asmFileIndex=%d isDataFile=%d\n", fileIndexInASM,isDataFile);
			goto errOut;
		}
		//将ASM文件的入口磁盘块转化为ASM文件的au分布信息
		m_asmDataFile_ausVec.clear();
		bRet = drctBlkToAusVec(m_asmDataFile_directBlkBuf,m_asmDataFile_ausVec);
		if(!bRet){
			t_errMsg("drctBlkToAusVec failed: asmFileIndex=%d isDataFile=%d\n", fileIndexInASM,isDataFile);
			goto errOut;
		}
		t_debugMsg(t_rwDataBlksLvl,"skip_ASM::drctBlkToAusVec loadDataFileAusVec_optimize: asmFileIndex=%d isDataFile=%d\n", fileIndexInASM,isDataFile);
	}

//	bRet=true;
errOut:
	return bRet;

}

bool skip_ASM::loadDataFileEntryBlock(const int dataFileIndexInASM)
{
	bool bRet = true;
	const BYTE8 fileIndexInASM = dataFileIndexInASM;
	const BYTE8 grpAuSize = m_grp_auSize;
	const BYTE8 grpBlkSize = m_grp_blockSize;
	const BYTE8 filesInAu = grpAuSize/grpBlkSize;
	const BYTE8 auOffIn1File = fileIndexInASM/filesInAu;
	const BYTE8 blkOffInAu =fileIndexInASM%filesInAu;

	int diskNum;
	int auNumInDisk;

	//获取AU的物理位置
	bRet=getAuInfoInDisk(1,auOffIn1File,&diskNum,&auNumInDisk);
	if(!bRet){
		t_errMsg("getAuInfoInDisk failed: fileIndexInASM=%d auOffInFile=%d diskNum=%d auNumInDisk=%d\n",1,auOffIn1File,diskNum,auNumInDisk);
		bRet = false;
		goto errOut;
	}

	//读相应的块
	bRet = readBlocksFromDisk_InSameAu(diskNum,auNumInDisk,blkOffInAu,1,m_asmDataFile_directBlkBuf);
	if(!bRet){
		t_errMsg("readBlocksFromDisk_InSameAu failed: \n");
		goto errOut;
	}

errOut:
	if(!bRet){
		bRet=false;
	}
	return bRet;
}

bool skip_ASM::getAuInfoInDisk(int fileIndex, int auOffInFile,int* pDiskNum,int* pAuOffInDisk)
{
	bool bRet = true;


	bRet= getAuInfoInDisk_optimize(fileIndex,auOffInFile,pDiskNum,pAuOffInDisk);
	if(!bRet){
		t_errMsg("getAuInfoInDisk_optimize: fileIndex%d auOffInFile=%d\n", fileIndex,auOffInFile);
		goto errOut;
	}

errOut:
	return bRet;
}

bool skip_ASM::getAuInfoInDisk_optimize(int fileIndex, int auOffInFile,int* pDiskNum,int* pAuOffInDisk)
{
	bool bRet = true;
	int errNo;
	const int getAuInfoTimes = 100;
	int i;
	const bool isCommonFile=fileIndex==1?false:true;
	const bool isCommonDataFile= isCommonFile? (m_asmDataFile_isDataFile?true:false):false;
	bool isFirstFind = true;


//	t_debugMsg(t_rwDataBlksLvl,"skip_ASM::getAuInfoInDisk_optimize start: fileNoInASM=%d auOffInFile=%d\n", fileIndex, auOffInFile);
	if(1 == fileIndex){//磁盘组的1号ASM文件
		for(i=0;i<getAuInfoTimes;i++){

			bRet = getAuInfoInDisk_byDrctBlk(m_1ASMFile_1BlockBuf, fileIndex, auOffInFile, pDiskNum, pAuOffInDisk, &errNo);
	
	//		if(!bRet && errNo==OUT_OF_AusVec){
			if(!bRet){
				t_debugMsg(t_rwDataBlksLvl,"内存中不包含该ASM文件的信息 重新载入磁盘组的一号文件的au分布信息: fileNoInASM=%d groupNo=%d\n", fileIndex,m_grp_num);
				isFirstFind=false;

				bRet=loadOneMetaFileEntryBlock();
				if(!bRet){
					t_errMsg("loadOneMetaFileEntryBlock: fileNoInASM=%d groupNo=%d\n", fileIndex,m_grp_num);
					goto errOut;
				}
				continue;
			}
			break;
		}
	}else{//磁盘里的普通ASM文件
		for(i=0;i<getAuInfoTimes;i++){

			if(m_asmDataFile_num != fileIndex){
				t_errMsg("fileIndex invalid: m_asmDataFile_num=%d fileIndex=%d\n", m_asmDataFile_num, fileIndex);
				bRet = false;
				goto errOut;
			}

			bRet=getAuInfoInDisk_(m_asmDataFile_ausVec, auOffInFile,pDiskNum,pAuOffInDisk,&errNo);

		//	if(!bRet&&errNo==OUT_OF_AusVec){
			if(!bRet){	
				t_debugMsg(t_rwDataBlksLvl,"普通ASM文件%d的第%d号AU的位置信息不在内存中 重新加载该ASM文件的AU分布信息: fileNoInASM=%d groupNo=%d\n", m_asmDataFile_num,auOffInFile);
				isFirstFind=false;
				
				bRet=loadOneMetaFileEntryBlock();
				if(!bRet){
					t_errMsg("loadOneMetaFileEntryBlock: fileNoInASM=%d groupNo=%d\n", 1,m_grp_num);
					goto errOut;
				}

				bRet=loadDataFileEntryBlock(m_asmDataFile_num);
				if(!bRet){
					t_errMsg("loadDataFileEntryBlock error: m_asmDataFile_num=%d auOffInFile=%d\n",m_asmDataFile_num, auOffInFile);
					goto errOut;
				}
				m_asmDataFile_ausVec.clear();
				bRet=drctBlkToAusVec(m_asmDataFile_directBlkBuf,m_asmDataFile_ausVec);
				if(!bRet){
					t_errMsg("drctBlkToAusVec error: m_asmDataFile_num=%d auOffInFile=%d\n",m_asmDataFile_num, auOffInFile);
					goto errOut;
				}

				sleep(1);
				continue;
			}
			break;
			
		}
			
	}

	if(bRet && !isFirstFind && isCommonDataFile){
		//更新缓存里的ASM文件的au分布信息
		t_debugMsg(t_rwDataBlksLvl,"更新缓存里的ASM文件的au分布信息: fileNoInASM=%d groupNo=%d\n", m_asmDataFile_num,auOffInFile);
		bRet=DataFilesAuDistribute::getInstance()->insertAuInfoOfFile(fileIndex,&m_asmDataFile_ausVec);
		if(!bRet){
			goto errOut;
		}
	}

//	t_debugMsg(t_rwDataBlksLvl,"skip_ASM::getAuInfoInDisk_optimize end: fileNoInASM=%d auOffInFile=%d isFirstFind=%d isCommonDataFile=%d diskNum=%d auOffInDisk=%d\n", fileIndex, auOffInFile, isFirstFind, isCommonDataFile,*pDiskNum,*pAuOffInDisk);
	
//		t_DebugMsg(t_skipASMLvl,"fileIndexInASM=%d\t auOffInFile=%d\t diskNum=%d\t auOffInDisk=%d\n",fileIndex,auOffInFile,*pDiskNum,*pAuOffInDisk);
errOut:
	if(!bRet){
		t_errMsg("获取au的物理位置信息失败: fileNoInASM=%d AuOffInFile=%d\n", fileIndex,auOffInFile);
	}
	return bRet;
}

bool skip_ASM::getAuInfoInDisk_(const vector<XPTR> &ausVec,const int auOffInFile, int *pDiskNum,int *pAuNumInDisk,int *pErrNo)
{
	bool bRet = true;
	
	if(auOffInFile<0 || auOffInFile>=ausVec.size()){//auOffInFile invalid
		t_errMsg("getAuInfoInDisk_ error: m_asmDataFile_num=%d auOffInFile=%d auOffInFile=%d ausVec.size()=%d \n", m_asmDataFile_num,auOffInFile, auOffInFile,ausVec.size());
		bRet = false;
		if(auOffInFile >= ausVec.size()){
			*pErrNo = OUT_OF_AusVec;
		}
		goto errOut;
	}
		
	
	*pDiskNum= ausVec.at(auOffInFile).disk;
	*pAuNumInDisk = ausVec.at(auOffInFile).au;

errOut:
	return bRet;
}

bool skip_ASM::getAuInfoInDisk_byDrctBlk(const char* drctBlk, int fileIndexInASM, int auOffInFile, int* pDiskNum, int* pAuOffInDisk, int* errNo)
{
	bool bRet = true;
	const BYTE8 grpAuSize = m_grp_auSize;
	const BYTE8 grpBlkSize = m_grp_blockSize;
	const BYTE8 drctBlkHeadLen = KFBH_L + POS_AU;
	const BYTE8 idrctBlkHeadLen = KFBH_L + KFFIXB_L;
		
	BYTE8 drctRddc;
	BYTE8 idrctRddc;
		
	BYTE8 maxDrctAusInDrctBlk;
	BYTE8 totalAusInDrctBlk;
	BYTE8 drctAusInDrctBlk;
	BYTE8 idrctAusInDrctBlk;
		
	BYTE8 i;

	BYTE8 byteOff;
		
	bool bHaveIdrctAus=false;

	KFBH kfbh;
	KFFFDB kfffdb;
	XPTR xptr;

	char *auBuf = NULL;

	memcpy(&kfbh,drctBlk+0,sizeof(kfbh));
	memcpy(&kfffdb,drctBlk+sizeof(KFBH),sizeof(KFFFDB));

	drctRddc=kfffdb.dXrs&0xF;
	idrctRddc=kfffdb.iXrs&0xF;

		
	bHaveIdrctAus = (kfffdb.xtntblk>kfffdb.asm_break)?true:false;
	maxDrctAusInDrctBlk = kfffdb.asm_break/drctRddc;
	drctAusInDrctBlk = bHaveIdrctAus?maxDrctAusInDrctBlk:(kfffdb.xtntblk/drctRddc);
	totalAusInDrctBlk = bHaveIdrctAus?maxDrctAusInDrctBlk+(kfffdb.xtntblk-kfffdb.asm_break)/idrctRddc:drctAusInDrctBlk;
	idrctAusInDrctBlk = totalAusInDrctBlk-drctAusInDrctBlk;
	
	if(drctAusInDrctBlk<=0){//调试用
		t_errMsg("drctBlkToAusVec: drctBlk=%p drctAusInDrctBlk=%d idrctAusInDrctBlk=%d\n", drctBlk,  drctAusInDrctBlk, idrctAusInDrctBlk);
	}
	
	byteOff=drctBlkHeadLen;
	if(auOffInFile < maxDrctAusInDrctBlk){//au分布在入口direct blk中
		byteOff += auOffInFile*drctRddc*sizeof(XPTR);
		memcpy(&xptr,drctBlk+byteOff,sizeof(XPTR));
	}else{//au分布信息不在入口direct blk中

		BYTE8 maxAusInIdrctBlk = AUinMetaB/drctRddc;
		BYTE8 maxAusInIdrctAu = (grpAuSize/grpBlkSize)*maxAusInIdrctBlk;
		BYTE8 idrctAuOffInDrctBlk = (auOffInFile - maxDrctAusInDrctBlk)/maxAusInIdrctAu;
		BYTE8 blkOffInIdrctAu = ( (auOffInFile - maxDrctAusInDrctBlk)%maxAusInIdrctAu )/maxAusInIdrctBlk;
		BYTE8 auOffInIdrctBlk = (auOffInFile - maxDrctAusInDrctBlk)%maxAusInIdrctBlk;

		auBuf = m_twoAuBuf_forDrctRW;

		byteOff += kfffdb.asm_break*sizeof(XPTR) + idrctAuOffInDrctBlk*idrctRddc*sizeof(XPTR);
		memcpy(&xptr,drctBlk+byteOff,sizeof(XPTR));
		bRet=readOneAUFromDisk(xptr.disk,xptr.au,auBuf);
		if(!bRet){
			goto errOut;
		}

		byteOff = blkOffInIdrctAu*grpBlkSize + idrctBlkHeadLen + auOffInIdrctBlk*drctRddc*sizeof(XPTR);
		memcpy(&xptr,auBuf+byteOff,sizeof(XPTR));
	}
	
	*pDiskNum = xptr.disk;
	*pAuOffInDisk = xptr.au;

	if( (drctAusInDrctBlk <= auOffInFile && auOffInFile < maxDrctAusInDrctBlk) || (auOffInFile >= maxDrctAusInDrctBlk && (auOffInFile - maxDrctAusInDrctBlk) >= idrctAusInDrctBlk)){
		//越界
		bRet = false;
		*errNo = OUT_OF_AusVec;
		goto errOut;
	}

	if(xptr.au == 0xffffffff){//越界标志
		bRet = false;
		*errNo = OUT_OF_AusVec;
		goto errOut;
	}



errOut:
	if(!bRet){
		bRet=false;
	}

	return bRet;
}

bool skip_ASM::drctBlkToAusVec(const char *drctBlk, vector<XPTR> &ausVec)
{
	bool bRet = true;
	const BYTE8 grpAuSize = m_grp_auSize;
	const BYTE8 grpBlkSize = m_grp_blockSize;
	const BYTE8 drctBlkHeadLen = KFBH_L + POS_AU;
		
	BYTE8 drctRddc;
	BYTE8 idrctRddc;
		
	BYTE8 maxDrctAusInDrctBlk;
	BYTE8 totalAusInDrctBlk;
	BYTE8 drctAusInDrctBlk;
	BYTE8 idrctAusInDrctBlk;
		
	BYTE8 i;

	BYTE8 byteOff;
		
	bool bHaveIdrctAus=false;

	KFBH kfbh;
	KFFFDB kfffdb;
	XPTR xptr;

	char *auBuf = NULL;

	memcpy(&kfbh,drctBlk+0,sizeof(kfbh));
	memcpy(&kfffdb,drctBlk+sizeof(KFBH),sizeof(KFFFDB));

	drctRddc=kfffdb.dXrs&0xF;
	idrctRddc=kfffdb.iXrs&0xF;

		
	bHaveIdrctAus = (kfffdb.xtntblk>kfffdb.asm_break)?true:false;
	maxDrctAusInDrctBlk = kfffdb.asm_break/drctRddc;
	drctAusInDrctBlk = bHaveIdrctAus?maxDrctAusInDrctBlk:(kfffdb.xtntblk/drctRddc);
	totalAusInDrctBlk = bHaveIdrctAus?maxDrctAusInDrctBlk+(kfffdb.xtntblk-kfffdb.asm_break)/idrctRddc:drctAusInDrctBlk;
	idrctAusInDrctBlk = totalAusInDrctBlk-drctAusInDrctBlk;

	BYTE8 maxAusInIdrctBlk = AUinMetaB/drctRddc;
	BYTE8 maxAusInIdrctAu = (grpAuSize/grpBlkSize)*maxAusInIdrctBlk;
	BYTE8 maxCouldAusInFile = drctAusInDrctBlk + idrctAusInDrctBlk*maxAusInIdrctAu;

	if(ausVec.capacity() < maxCouldAusInFile){//容量不够
		ausVec.reserve(maxCouldAusInFile);
	}

	auBuf = m_twoAuBuf_forDrctRW;

	if(drctAusInDrctBlk<=0){//调试用
		t_errMsg("drctBlkToAusVec: drctBlk=%p &ausVec=%p drctAusInDrctBlk=%d idrctAusInDrctBlk=%d\n", drctBlk, &ausVec, drctAusInDrctBlk, idrctAusInDrctBlk);
	}


	byteOff=drctBlkHeadLen;
	for(i=0;i<drctAusInDrctBlk;i++){
		memcpy(&xptr,drctBlk+byteOff,sizeof(XPTR));
		ausVec.push_back(xptr);
		byteOff += sizeof(XPTR)*drctRddc;
	}

	for(i=0;i<idrctAusInDrctBlk;i++){
		memcpy(&xptr,drctBlk+byteOff,sizeof(XPTR));
		bRet=readOneAUFromDisk(xptr.disk,xptr.au,auBuf);
		if(!bRet){
			goto errOut;
		}
		bRet=ausInIdrctAuPushBackToAusVec(auBuf,ausVec);
		if(!bRet){
			goto errOut;
		}
		byteOff += sizeof(XPTR);
	}		

errOut:
	if(!bRet){
		bRet=false;
	}

	return bRet;
}


bool skip_ASM::ausInIdrctAuPushBackToAusVec(const char* idrctAuBuf,vector<XPTR> &ausVec)
{
	bool bRet = true;

	const BYTE8 grpAuSize = m_grp_auSize;
	const BYTE8 grpBlkSize = m_grp_blockSize;
	const BYTE8 blksInAu = grpAuSize/grpBlkSize;
	const BYTE8 idrctBlkHeadLen = KFBH_L + KFFIXB_L;

	BYTE8 drctRddc;
	BYTE8 maxAusInIdrctBlk;

	BYTE8 byteOffInIdrctAu = 0;
		

	BYTE8 i,j;

	bool bHaveNoAu = false;
	
	KFBH kfbh;
	KFFIXB kffixb;
		
	XPTR xptr;
		
	memcpy(&kfbh,  idrctAuBuf+0,            sizeof(KFBH));
	memcpy(&kffixb,idrctAuBuf+sizeof(KFBH), sizeof(KFFIXB));
		
	drctRddc = kffixb.dXrs&0xF;
	maxAusInIdrctBlk = AUinMetaB/drctRddc;
	kfbh.block_blk;

	byteOffInIdrctAu = 0;
	bHaveNoAu = false;
	for(i=0;i<blksInAu;i++){
		byteOffInIdrctAu = i*grpBlkSize;
		memcpy(&kfbh,  idrctAuBuf+byteOffInIdrctAu,            sizeof(KFBH));
		memcpy(&kffixb,idrctAuBuf+byteOffInIdrctAu+sizeof(KFBH), sizeof(KFFIXB));
		kfbh.block_blk;
		byteOffInIdrctAu += idrctBlkHeadLen;
		for(j=0;j<maxAusInIdrctBlk;j++){
			memcpy(&xptr,idrctAuBuf+byteOffInIdrctAu,sizeof(XPTR));
			if(xptr.au == 0xffffffff){
				bHaveNoAu = true;
				bRet = true;
				goto errOut;
			}
			ausVec.push_back(xptr);
			byteOffInIdrctAu += sizeof(XPTR)*drctRddc;
			
		}		

	}

errOut:
	return bRet;
}



/*
bool skip_ASM::readBlocksFromDisk_InSameAu(int diskNum, int auNumInDisk, int blockNumInAu,int blocksToRead, char *blocksBuf_)
{
	int bRet= true;

	int diskFd;
	int iCount;
	int iTmp;
	BYTE8 llCount;

	const BYTE8 grpAuSize=m_grp_auSize;
	const BYTE8 grpBlkSize = m_grp_blockSize;
	const BYTE8 readOffInDisk = auNumInDisk*grpAuSize + blockNumInAu*grpBlkSize;
	const BYTE8 readLen = blocksToRead*grpBlkSize;
	const int rdTimesIfRdFaild = 3;
	
	int openMode = TW_OPEN_READ;
	const bool isOpenWithDirect = true;//是否以直接读的方式打开

	char * twoAuBuf = NULL;
	char * blocksBuf = NULL;
		
	int readedTimes;

	if(diskNum <0 || diskNum >=m_grp_pDisks->size()){
		t_errMsg("diskNum invalied: diskNum=%d m_grp_pDisks->size()=%d\n", diskNum, m_grp_pDisks->size());
		bRet = false;
		goto errOut;
	}
		
	if( (blockNumInAu*grpBlkSize+readLen) > grpAuSize){// 读取的数据不在同一个AU中 非法
		t_errMsg("read too big\n");
		bRet = false;
		goto errOut;
	}

	if(isOpenWithDirect){//以直接读的方式打开
		twoAuBuf = new char[grpAuSize*2];
		if(!twoAuBuf){
			t_errMsg("new error\n");
			bRet = false;
			exit(1);
			goto errOut;
		}
		memset(twoAuBuf,0,grpAuSize*2);

		openMode = TW_OPEN_READ | O_DIRECT;
		blocksBuf = (char*)ptr_align (twoAuBuf, grpAuSize);
	}else{//以非直接读的方式打开
		openMode = TW_OPEN_READ;
		blocksBuf = blocksBuf_;
	}
		
	//打开磁盘
	bRet = openDisk(diskNum,openMode);
	if(!bRet){
		bRet =false;
		exit(1);
		goto errOut;
	}


	for(readedTimes=0;readedTimes<rdTimesIfRdFaild;readedTimes++){
		// 定位
		diskFd=m_grp_pDisks->at(diskNum).DiskFd;
		llCount = tw_lseek(diskFd,readOffInDisk,SEEK_SET);
		if(llCount != readOffInDisk){//定位失败
			t_errMsg("lseek error \n");
			bRet = false;
			tw_sleep(1);
			continue;
		}

		//读磁盘
		iCount = tw_read(diskFd,blocksBuf, readLen);
		if(iCount < 0 || iCount != readLen){//读取失败 或非法
			t_errMsg("read error: diskFd=%d readLen=%d readedLen=%d errMsg=%s\n",diskFd, readLen,iCount,strerror(errno));
			bRet = false;
			tw_sleep(1);
			continue;
		}
		t_debugMsg(t_rwDataBlksLvl,"read error: diskNum=%d auNumInDisk=%d blocksToRead=%d blocksBuf_=%p diskFd=%d readOffInDisk=%lld readLen=%d readedLen=%d \n",
			diskNum, auNumInDisk, blocksToRead, blocksBuf_, diskFd, readOffInDisk, readLen,iCount);
	    bRet = true;
		break;
	}

	if(readedTimes == rdTimesIfRdFaild){//read failed
		bRet = false;
		goto errOut;
	}
	
	if(isOpenWithDirect){
		memcpy(blocksBuf_,blocksBuf,readLen);
	}

errOut:
	if(!bRet){
		bRet=false;
	}

	if(twoAuBuf){
		delete[] twoAuBuf;
		twoAuBuf = NULL;
	}

	return bRet;
}


bool skip_ASM::readOneAUFromDisk(int diskNum,int auNumInDisk,char *auBuf)
{
	int bRet= true;
	
	const BYTE8 grpAuSize=m_grp_auSize;
	const BYTE8 readOffInDisk = auNumInDisk*grpAuSize;
	const BYTE8 readLen = grpAuSize;
	BYTE8 llCount = 0;
	int iCount = 0;
	
	int readedTimes = 0;
	const int rdTimesIfRdFaild = 3;
	int diskFd = 0;

	int openMode = TW_OPEN_READ;
	
	char* twoAuBuf = NULL;
	char* oneAuRdPtr = NULL;//指向内存对其的位置 然后读到该位置
	
	const bool isOpenWithDirect = true; //是否以直接方式打开磁盘
	
	if(isOpenWithDirect){//以直接读的方式打开
		twoAuBuf=new char[2*grpAuSize];
		if(!twoAuBuf){
			t_errMsg("new error\n");
			bRet = false;
			exit(1);
			goto errOut;
		}
		memset(twoAuBuf,0,2*grpAuSize);
		oneAuRdPtr=(char*)ptr_align (twoAuBuf, grpAuSize); //需要内存对齐
		openMode =TW_OPEN_READ | O_DIRECT; //直接读的方式打开
	}else{//以非直接读的方式打开
		oneAuRdPtr = auBuf; //无需内存对齐
		openMode = TW_OPEN_READ; //非直接读的方式打开
	}
	
	//打开磁盘
	bRet = openDisk(diskNum,openMode);
	if(!bRet){
		t_errMsg("openDisk error: diskNum=%d openMode=%d\n",diskNum,openMode);
		bRet =false;
		exit(1);
		goto errOut;
	}

	//读数据
	for(readedTimes=0;readedTimes<rdTimesIfRdFaild;readedTimes++){
		// 定位
		diskFd=m_grp_pDisks->at(diskNum).DiskFd;
		llCount = tw_lseek(diskFd,readOffInDisk,SEEK_SET);
		if(llCount != readOffInDisk){//定位失败
			t_errMsg("lseek error\n");
			bRet = false;
			tw_sleep(1);
			continue;
		}

		//读磁盘
		iCount = tw_read(diskFd,oneAuRdPtr, readLen);
		if(iCount < 0 || iCount != readLen){//读取失败 或非法
			t_errMsg("read error: diskFd=%d readLen=%d readedLen=%d errMsg=%s\n",diskFd, readLen,iCount,strerror(errno));
			bRet = false;
			tw_sleep(1);
			continue;
		}
		t_debugMsg(t_rwDataBlksLvl,"read error: diskNum=%d auNumInDisk=%d auBuf=%p diskFd=%d readOffInDisk=%lld readLen=%d \n",
			diskNum,auNumInDisk,auBuf,diskFd,readOffInDisk,readLen);
	    bRet = true;
		break;
	}

	if(readedTimes == rdTimesIfRdFaild){//read failed
		bRet = false;
		goto errOut;
	}

	if(isOpenWithDirect){
		memcpy(auBuf,oneAuRdPtr,readLen);
	}

errOut:

	if(twoAuBuf){
		delete[] twoAuBuf;
		twoAuBuf = NULL;
	}

	return bRet;

}
*/

bool skip_ASM::readBlocksFromDisk_InSameAu(int diskNum, int auNumInDisk, int blockNumInAu,int blocksToRead, char *blocksBuf_)
{
	bool bRet = true;
	const int bytesOffInAu = blockNumInAu*m_grp_blockSize;
	const int bytesToRead = blocksToRead*m_grp_blockSize;
	
	bRet = readBytesFromDisk_inSameAu(diskNum, auNumInDisk, bytesOffInAu, bytesToRead, blocksBuf_);
	if(!bRet){
		goto errOut;
	}


errOut:
	return bRet;
}

bool skip_ASM::readOneAUFromDisk(int diskNum,int auNumInDisk,char *auBuf)
{
	bool bRet = true;

	const int bytesOffInAu = 0;
	const int bytesToRead = m_grp_auSize;

	bRet = readBytesFromDisk_inSameAu(diskNum, auNumInDisk, bytesOffInAu, bytesToRead, auBuf);
	if(!bRet){
		goto errOut;
	}


errOut:
	return bRet;
}
bool skip_ASM::readBytesFromDisk_inSameAu(int diskNum, int auNumInDisk, int bytesOffInAu, int bytesToRead, char* bytesBuf)
{
	bool bRet = true;

	const BYTE8 grpAuSize=m_grp_auSize;
	const BYTE8 readOffInDisk = auNumInDisk*grpAuSize + bytesOffInAu;
	const BYTE8 readLen = bytesToRead;
	BYTE8 llCount = 0;
	int iCount = 0;
	
	int readedTimes = 0;
	const int rdTimesIfRdFaild = 3;
	int diskFd = 0;

	int openMode = TW_OPEN_READ;
	char* twoAuBuf = NULL;
	char* readBuf = NULL;//指向内存对其的位置 然后读到该位置
	const bool isOpenWithDirect = m_isDirectOpen; //是否以直接方式打开磁盘

	if(bytesOffInAu+bytesToRead > grpAuSize){
		return false;
		
	}

	if(isOpenWithDirect){//以直接读的方式打开
		twoAuBuf=m_twoAuBuf_forDrctRW;
		readBuf =(char*)ptr_align (twoAuBuf, grpAuSize); //需要内存对齐
		openMode =TW_OPEN_READ | O_DIRECT; //直接读的方式打开
	}else{//以非直接读的方式打开
		readBuf = bytesBuf; //无需内存对齐
		openMode = TW_OPEN_READ; //非直接读的方式打开
	}
	
	s_readDisk_mutex.Lock();
	//打开磁盘
	bRet = openDisk(diskNum,openMode);
	if(!bRet){
		t_errMsg("openDisk error: diskNum=%d openMode=%d\n",diskNum,openMode);
		bRet =false;
		exit(1);
		goto errOut;
	}
	
	//读数据
	for(readedTimes=0;readedTimes<rdTimesIfRdFaild;readedTimes++){
		// 定位
		diskFd=m_grp_pDisks->at(diskNum).DiskFd;
		llCount = tw_lseek(diskFd,readOffInDisk,SEEK_SET);
		if(llCount != readOffInDisk){//定位失败
			t_errMsg("lseek error\n");
			bRet = false;
			tw_sleep(1);
			continue;
		}

		//读磁盘
		iCount = tw_read(diskFd,readBuf, readLen);
		if(iCount < 0 || iCount != readLen){//读取失败 或非法
			t_errMsg("read error: diskFd=%d readLen=%d readedLen=%d errMsg=%s\n",diskFd, readLen,iCount,strerror(errno));
			bRet = false;
			tw_sleep(1);
			continue;
		}
		t_debugMsg(t_rwDataBlksLvl,"read success: diskNum=%d auNumInDisk=%d readBuf=%p diskFd=%d readOffInDisk=%lld readLen=%d \n",
			diskNum,auNumInDisk,readBuf,diskFd,readOffInDisk,readLen);
	    bRet = true;
		break;
	}

	if(readedTimes == rdTimesIfRdFaild){//read failed
		bRet = false;
		goto errOut;
	}
	
	if(isOpenWithDirect){
		memcpy(bytesBuf,readBuf,readLen);
	}

errOut:
	s_readDisk_mutex.Unlock();
	return bRet;
}

bool skip_ASM::openDisk(int diskNum, int flag)
{
	bool bRet = true;
	int diskFd = -1;
	char diskPath[256] = {0};
	int dbgLvl = 111;

	if(diskNum<0 || diskNum >= m_grp_pDisks->size()){//diskNum invalid
		bRet= false;
		t_errMsg("diskNum invalied: diskNum=%d\n",diskNum);
		goto errOut;
	}

	if(m_grp_pDisks->at(diskNum).DiskFd < 0 || m_grp_pDisks->at(diskNum).Openflag != flag){//磁盘没有打开或者打开的方式不对
		if(m_grp_pDisks->at(diskNum).DiskFd >=0){//打开方式不对
			myclose(m_grp_pDisks->at(diskNum).DiskFd);
		}

		m_grp_pDisks->at(diskNum).DiskFd = -1;
		m_grp_pDisks->at(diskNum).Openflag = 0;
		strncpy(diskPath,m_grp_pDisks->at(diskNum).Path,256-1);	
		t_debugMsg(dbgLvl,"open disk: grpNum=%d diskNum=%d diskPath=%s \n", m_grp_num,diskNum, diskPath);
		diskFd = tw_open(diskPath,flag);
		if(diskFd<0){
			bRet = false;
			t_errMsg("open disk failed\n");
			goto errOut;
		}
		m_grp_pDisks->at(diskNum).DiskFd = diskFd;
		m_grp_pDisks->at(diskNum).Openflag = flag;
	}

errOut:
	if(!bRet){
		bRet=false;
	}
	return bRet;
}

bool skip_ASM::closeAllDisks()
{
	bool bRet = true;
	int i;
	int diskFd;

	if(!m_grp_pDisks){
		goto errOut;
	}
		
	for(i=0; i< m_grp_pDisks->size(); i++)
	{
		diskFd = m_grp_pDisks->at(i).DiskFd;
		if(diskFd>=0){
			myclose(diskFd);
		}
		m_grp_pDisks->at(i).DiskFd = -1;
		m_grp_pDisks->at(i).Openflag = 0;
	}

	bRet = true;

errOut:
	if(!bRet){
		bRet=false;
	}
	return bRet;
}

bool skip_ASM::loadDiskHeaderInfo()
{
	bool bRet = true;
	
	int groupNum = m_grp_num;
	int grpBlockSize = m_grp_blockSize;

	int diskNumInGrp = 0;
	int auNumInDisk = 0;
	int blockNumInAu = 0;
	int blocksToRead = 1;

	KFDHDB	kfdhdb;
	
	char* diskHeaderBlock = new char[grpBlockSize];
	
	if(!diskHeaderBlock){
		t_errMsg("new failsed\n");
		bRet = false;
		goto errOut;
	}
	
	//读数据
	bRet=readBlocksFromDisk_InSameAu(diskNumInGrp,auNumInDisk,blockNumInAu,blocksToRead,diskHeaderBlock);
	if(!bRet){
		bRet = false;
		t_errMsg("readBlocksFromDisk_InSameAu failed\n");
		goto errOut;
	}

	memcpy(&kfdhdb,diskHeaderBlock+KFBH_L,KFDHDB_L);
	
	m_grp_dbcompat = kfdhdb.dbcompat;


errOut:
	if(diskHeaderBlock){
		delete[] diskHeaderBlock;
		diskHeaderBlock = NULL;
	}
	return bRet;
}

bool skip_ASM::loadOneMetaFileHeaderInfo()
{
	bool bRet = true;
	int grpAuSize = m_grp_auSize;

	KFFFDB	kfffdb;//文件头信息
	
	BYTE stripeSize = 0;
	int stripeDepth = 1;
	
	int i;
	

	char* asmFile_1BlockBuf = m_1ASMFile_1BlockBuf;

	memset(&kfffdb,0,KFFFDB_L);
	memcpy(&kfffdb,asmFile_1BlockBuf+KFBH_L,KFFFDB_L);

	m_1ASMFile_stripeWidth = kfffdb.strpwdth;
	stripeSize = kfffdb.strpsz;

	stripeDepth = 1;
	for(i=0; i< stripeSize; i++){
		stripeDepth *= 2;
	}
	if(m_1ASMFile_stripeWidth > 1 && stripeDepth != grpAuSize){//有条带且条带深度不是一个AU 暂时不支持
		t_errMsg("have stripe bu stripeDepth is not one au size\n");
		bRet = false;
		goto errOut;
		
	}
	m_1ASMFile_stripeDepth =stripeDepth;

	

errOut:
	return bRet;

}

//初始化DataFileAuDistribute的静态成员变量
DataFilesAuDistribute* DataFilesAuDistribute::s_instance = NULL;
Mutex DataFilesAuDistribute::s_mutex;


DataFilesAuDistribute::DataFilesAuDistribute()
{
	return;
}

DataFilesAuDistribute::~DataFilesAuDistribute()
{
	mapIteratorType it;
	//释放map里的所有内存
	for(it=m_auDistributeOfFiles.begin(); it != m_auDistributeOfFiles.end(); it++){
		if(it->second){
			delete it->second;
		}
		it->second = NULL;
	}
	m_auDistributeOfFiles.clear();
	return;
}

void DataFilesAuDistribute::clear()
{
	mapIteratorType it;
	
	lock();

	//释放map里的所有内存
	for(it=m_auDistributeOfFiles.begin(); it != m_auDistributeOfFiles.end(); it++){
		if(it->second){
			delete it->second;
		}
		it->second = NULL;
	}
	m_auDistributeOfFiles.clear();

errOut:
	unlock();
	return;
}
/*
函数功能:从map中获取ASM文件fileNo的au分布信息
输入: fileNo--ASM文件号
输出: pAuInfoOfFile--获取的au分布信息
返回: 
*/
bool DataFilesAuDistribute::findAuInfoOfFile(const int fileNo, vector<XPTR> *pAuInfoOfFile)
{
	bool bRet=true;
	mapIteratorType it;
	
//	t_debugMsg(888,"DataFilesAuDistribute::findAuInfoOfFile: fileNo=%d\n",fileNo);
	lock();
    //查看数据文件fileNo的au分布信息 在map中
	it=m_auDistributeOfFiles.find(fileNo);
	if(it == m_auDistributeOfFiles.end()){//not find
		bRet=false;
		goto errOut;
	}
	
	//将fileNo的au分布信息拷贝到另一块内存中
	bRet = copyXPTRVec(*it->second, *pAuInfoOfFile);
	if(!bRet){
		goto errOut;
	}

errOut:
	unlock();
	return bRet;
}

/*
*函数功能: 向map中插入ASM文件fileNo的au分布信息
*输入: fileNo--ASM文件号 pAuInfoOfFile--au分布信息
*输出:
*返回: true--成功 false--失败
*/
bool DataFilesAuDistribute::insertAuInfoOfFile(const int fileNo, const vector<XPTR> *pAuInfoOfFile)
{
	bool bRet=true;
	
	mapIteratorType it;
	mapSecondType mapSecond = NULL;
	mapInsertRetType mapInsertRet;
	
	bool isFind = true;

//    t_debugMsg(888,"DataFilesAuDistribute::insertAuInfoOfFile: fileNo=%d\n",fileNo);
	lock();
	//查看数据文件fileNo的au分布信息 在map中
	it=m_auDistributeOfFiles.find(fileNo);
	if(it == m_auDistributeOfFiles.end()){//not find
		bRet=false;
		isFind=false;
		//插入一个空的节点
		mapSecond = new vector<XPTR>;
		if(!mapSecond){
			bRet=false;
			exit(1);
			goto errOut;
		}
		mapInsertRet=m_auDistributeOfFiles.insert(make_pair(fileNo, mapSecond));
		if(!mapInsertRet.second){//insert failed
			bRet=false;
			goto errOut;
		}

	}

	//将fileNo的新的au分布信息 拷贝到map相应的节点中
	bRet=copyXPTRVec(*pAuInfoOfFile,*mapSecond);
	if(!bRet){
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
DataFilesAuDistribute* DataFilesAuDistribute::getInstance()
{
	
	if(!s_instance){

		lock();
		if(!s_instance){
			s_instance=new DataFilesAuDistribute;
			if(!s_instance){
				exit(1);
			}
		}
		unlock();

	}
	
errOut:
	return s_instance;
}


/*
*函数功能: 拷贝vector<XPTR>
*输入: srcVec
*输出: dstVec
*返回: true--success false--failed
*/
/*
bool DataFilesAuDistribute::copyXPTRVec(const vector<XPTR> &srcVec, vector<XPTR> &dstVec)
{
	bool bRet=true;
	const int srcVecSize = srcVec.size();

	
	//确保目的vec有足够的空间用于拷贝
	dstVec.resize(srcVecSize);
	//清空目的vect以前的数据
	dstVec.clear();
	//执行拷贝
	dstVec.insert(dstVec.begin(), srcVec.begin(), srcVec.end());

errOut:
	return bRet;
}
*/

bool DataFilesAuDistribute::copyXPTRVec(const vector<XPTR> &srcVec, vector<XPTR> &dstVec)
{
	bool bRet=true;
	const int srcVecSize = srcVec.size();
	const int dstVecCapacity = dstVec.capacity();

	
	//确保目的vec有足够的空间用于拷贝
	//dstVec.resize(srcVecSize);
	if(dstVecCapacity < srcVecSize ){
		dstVec.reserve(srcVecSize);
	}
	//清空目的vect以前的数据
	dstVec.clear();
	//执行拷贝
	//dstVec.insert(dstVec.begin(), srcVec.begin(), srcVec.end());
	dstVec.assign(srcVec.begin(), srcVec.end());

errOut:
	return bRet;
}
