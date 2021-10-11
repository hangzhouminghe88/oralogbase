
#if 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <vector>

#include "tw_rwDataBlocks.h"
#include "tw_api.h"
#include "OciQuery.h"
#include "RedologINI.h"
#include "BlockPool.h"
#include "Defines.h"
#include "tw_ASM.h"



using namespace std;

extern Semaphore G_AnalyseSem;

extern Datafiles G_Datafiles;
extern BYTE8	G_CreateChange;

extern vector< vector<DISK> > G_Disks;
extern vector<Group> G_Groups;

bool tw_loadDataFilesInfo(OciQuery* pNormalQuery);
bool tw_loadGroupsInfo(OciQuery* pASMQuery, RedologINI* pConfigINI);
bool tw_readASMFile(const char* fileName, const int blkSize, const char* outFileName, OciQuery* pASMQuery);
bool tw_readASMBlks(const char* fileName, const int blkOffInFile, const int blksToRead, const int blkSize, char* blksBuf,OciQuery* pASMQuery);
bool tw_readCommonFile(const char* fileName, const int blkSize, const char* outFileName, OciQuery* pNormalQuery=NULL);
ReturnType tw_ConsumerThread(LPVOID *Context);
bool mn_checkBlocks(BYTE *buf,int startBlock,int blockNum,int  blockSize);

int main(int argc, char** argv)
{

	bool b;
	bool bASM;
	RedologINI configINI;
	OciQuery asmQuery;
	OciQuery normalQuery;
	HANDLE hThread;
	ThreadID dwThreadID;
	RedoLog logFile;
	Redologs archivedLogs;

	char asmUsr[128]={0};
	char asmPasswd[128]={0};
	char asmSID[128]={0};
	char usr[128]={0};
	char passwd[128]={0};
	char sid[128]={0};
	char arch[128] = {0};

	char* fileNameToRead = NULL;
	char* fileNameToWrite = NULL;
	char* archiveFileName = NULL;
	int sequence = 0;
	int i =0;
	t_debugMsg(1,"hello tianwei, today's date is:2014-02-13\n");

	configINI.LoadFile("/usr/NETPRO5.02.8/conf/Redolog.ini");

	////////////////////////////////////////////////
	//configINI.SavelongValuse("LOG","SCN_SENDED",14001533164501);
	//if(tw_isToDetectTime(configINI)){
	//	tw_detectResultWRTOConfig(configINI,true);
	//}
	///////////////////////////////////////////////

	configINI.LoadValuse("ASM", "USER", asmUsr);
	configINI.LoadValuse("ASM", "PASSWD", asmPasswd);
	configINI.LoadValuse("ASM", "ASM", asmSID);

	configINI.LoadOCIValuse(usr,passwd, sid);
	
	asmQuery.SetValses(asmSID,asmUsr,asmPasswd);
	normalQuery.SetValses(sid,usr,passwd);
	
	t_debugMsg(t_dbgLvl,"asmSID=%s asmUsr=%s asmPasswd=%s\n",asmSID,asmUsr,asmPasswd);
	t_debugMsg(t_dbgLvl,"sid=%s usr=%s passwd=%s \n",sid,usr,passwd);
	
	b=tw_loadDataFilesInfo(&normalQuery);
	if(!b){
		t_errMsg("tw_loadDataFilesInfo failed\n");
		return -1;
	}

	configINI.LoadValuse("LOG","ASM",arch);
	bASM = false;
	if(0 == strcmp(arch,"YES")){
		bASM = true;
	}
	
	if(bASM){
		b=tw_loadGroupsInfo(&asmQuery,&configINI);
		if(!b){
			return -1;
		}
	}

	
//	return 0;


/*

///////////////////////////////////////


	hThread = MyCreateThread(&dwThreadID,tw_ConsumerThread,NULL);

	configINI.LoadValuse("LOG","ASM",arch);
	bASM = false;
	if(0 == strcmp(arch,"YES")){
		bASM = true;
	}

	BYTE8 LastSCN = configINI.LoadSCN();

	if(CONNECT_ERROR == normalQuery.GetArchivedLogBySCN_DATE(archivedLogs,LastSCN,NULL,LastSCN)) {//OCI出问题了,线程退出
			logFile.RedoAnalyseEnd();
			t_errMsg("normalQuery.GetArchivedLogBySCN_DATE\n");
			return 0;
	}

	for(i=0; i<archivedLogs.size();i++){
//	for(i=0;i<1;i++){
//		archiveFileName = "+DATA/orcl/archivelog/2014_06_11/thread_1_seq_895.309.849977453";
//		sequence = 895;

		archiveFileName = archivedLogs[i].FileName;
		sequence = archivedLogs[i].Sequence;

		BYTE8 beginSCN = archivedLogs[i].BeginSCN;

		if(logFile.LoadNewFile(archiveFileName,_INACTIVE,sequence,bASM,&asmQuery) < 0){
			cout<<"LoadNewFile ERROR"<<endl;
			return -1;
		}
	
		logFile.LoadandSkipLogHead();
		cout<<"LogFile.BeginSCN:\t"<<logFile.BeginSCN<<endl;
		fprintf(stdout,"start archiveFileName=%s sequence#=%d\n",archiveFileName,sequence);
		if(logFile.BeginSCN != beginSCN){
			t_errMsg("LoadandSkipLogHead failed: logFile.BeginSCN=%d query.BeginsSCN=%d\n", logFile.BeginSCN, beginSCN );
			return -1;
		}

		b= logFile.Loadall();
		if(b){
			cout<<"OK .... SaveSCN:\t"<<logFile.BeginSCN<<endl;
		}
		fprintf(stdout,"end archiveFileName=%s sequence#=%d\n",archiveFileName,sequence);
	}
	


	return 0;


/////////////////////////////////

*/	
	
//	return 0;


/////////////////////////////////


//	fileNameToRead = "+DATA/orcl/datafile/example.264.818938391";
//	fileNameToWrite = "/home/tw/example.264.818938391";
//	b=tw_readASMFile(fileNameToRead,512,fileNameToWrite,&asmQuery);
//	if(!b){
//		return -1;
//	}

	fileNameToRead = "+EMRS_DATA/emrs/datafile/undotbs104.ora";
//	fileNameToRead = "/oradata/DG/datafile/undotbs01.dbf";
	fileNameToWrite = "/home/tw/tw_test.dbf_Write";

//	b=tw_readASMBlks(fileNameToRead,9999, 1,8192,NULL,&asmQuery);
//	if(!b){
//		return -1;
//	}
	
	b=tw_readASMFile(fileNameToRead,8192,fileNameToWrite,&asmQuery);
	if(!b){
		return -1;
	}

//	fileNameToRead = "+DATA/orcl/archivelog/2014_03_02/thread_1_seq_857.772.841125619";
//	fileNameToWrite = "/home/tw/thread_1_seq_857.772.841125619_WRITE";
//	b = tw_readCommonFile(fileNameToRead, 512, fileNameToWrite);
//	if(!b){
//		return -1;
//	}


///////////////////////////////////////////////////
	return 0;
}

bool tw_loadDataFilesInfo(OciQuery* pNormalQuery)
{
	bool bRet=true;
	int i;
//	G_Datafiles.clear();
	t_debugMsg(t_dbgLvl,"tw_loadDataFilesInfo start: G_Datafils=%p G_CreateChange=%p\n", &G_Datafiles, &G_CreateChange);
	i=pNormalQuery->GetAllDatafileInfo(&G_Datafiles,G_CreateChange);
	if(i<0){
		bRet=false;
		goto errOut;
	}
	t_debugMsg(t_dbgLvl,"tw_loadDataFilesInfo end: G_Datafils=%p G_CreateChange=%p\n", &G_Datafiles, &G_CreateChange);
	t_debugMsg(t_dbgLvl,"tw_loadDataFilesInfo end: G_Datafils[%d].FileName=%s\n", 1, G_Datafiles[1].FileName);
	t_debugMsg(t_dbgLvl,"tw_loadDataFilesInfo end: G_Datafils[%d].FileName=%s\n", 2, G_Datafiles[2].FileName);
errOut:
	return bRet;
}



bool tw_loadGroupsInfo(OciQuery* pASMQuery, RedologINI* pConfigINI)
{
	bool bRet=true;
	int i;

	int groupSize;
	int size;

	int size1 = 0;
	int size2 = 0;
	char strFullPath[STR_LEN]={0};
	char strDiskName[STR_LEN]={0};
	int headlen = sizeof(ASMHEAD);
	
	int dbgLvl = 33;
	t_debugMsg(dbgLvl,"tw_loadGroupsInfo start:\n");
	//清理全局变量
	for(i=0;i<G_Disks.size();i++){
		G_Disks[i].clear();
	}
	G_Disks.clear();
	G_Groups.clear();

	i=pASMQuery->GetAllGroupInfo(&G_Groups);
//	if(i<0){总是返回错误，即使成功
//		bRet=false;
//		goto errOut;
//	}
	
	groupSize = G_Groups.size(); 
	size = groupSize;
	for(int i=0;i<size;i++){ // 可能存在不连续Group的情况
		if(groupSize < G_Groups[i].GroupNum){
			groupSize = G_Groups[i].GroupNum;
		}
	}
	
	i=pASMQuery->GetAllASMDisks(&G_Disks,groupSize);
//	if(i<0){
//		bRet=false;
//		goto errOut;
//	}
	
	size1 = G_Disks.size();
	size2 = 0;
//	headlen = sizeof(ASMHEAD);
	headlen = strlen(ASMHEAD);
	for(int index=0;index<size1;index++){ //这里处理磁盘路径为CRTL:DISK1 这种情况
		size2 = G_Disks[index].size();
		for(int j=0;j<size2;j++){
			t_debugMsg(dbgLvl,"tw_loadGroupsInfo start66:G_Disks[%d][%d].Path=%s ASMHEAD=%s headlen=%d\n",index, j,G_Disks[index][j].Path,ASMHEAD, headlen);
			t_debugMsg(dbgLvl,"tw_loadGroupsInfo start66:strncmp(ASMHEAD,G_Disks[index][j].Path,headlen)=%d G_Disks[index][j].Path[0]=%c\n",strncmp(ASMHEAD,G_Disks[index][j].Path,headlen),G_Disks[index][j].Path[0]);

			if(G_Disks[index][j].Path[0] != '/' && !strncmp(ASMHEAD,G_Disks[index][j].Path,headlen)){ //ORCL:
				t_debugMsg(dbgLvl,"tw_loadGroupsInfo start77:G_Disks[index][j].Path=%s \n",G_Disks[index][j].Path);
				memset(strFullPath,0,sizeof(strFullPath));
				strcpy(strDiskName,(G_Disks[index][j].Path+headlen));
				t_debugMsg(dbgLvl,"tw_loadGroupsInfo start777:G_Disks[index][j].Path=%s strDiskName=%s\n",G_Disks[index][j].Path, strDiskName);
				pConfigINI->LoadValuse("ASM", strDiskName, strFullPath);
				t_debugMsg(dbgLvl,"tw_loadGroupsInfo start777:G_Disks[index][j].Path=%s strDiskName=%s strFullPath=%s\n",G_Disks[index][j].Path, strDiskName,strFullPath);
				if(strFullPath[0] != 0){
					strncpy(G_Disks[index][j].Path,strFullPath,sizeof(G_Disks[index][j].Path));
					t_debugMsg(dbgLvl,"tw_loadGroupsInfo start88:G_Disks[index][j].Path=%s \n",G_Disks[index][j].Path);
				}else{
					cout<<"配置磁盘信息出错:\t"<<G_Disks[index][j].Path<<endl;
					t_errMsg("配置磁盘信息出错: %s\n",G_Disks[index][j].Path);
					bRet=false;
					goto errOut;
				}
			}

			t_debugMsg(dbgLvl,"tw_loadGroupsInfo start99:G_Disks[%d][%d].Path=%s \n",index, j,G_Disks[index][j].Path);
		}
	}

	t_debugMsg(dbgLvl,"tw_loadGroupsInfo end\n");

errOut:
	return bRet;
}

bool tw_readASMFile(const char* fileName, const int blkSize, const char* outFileName,OciQuery* pASMQuery)
{
	bool bRet = true;
	RWDataBlocks* pRWDataBlks = NULL;
	char* oneBlkBuf = NULL;

	int blksTotalReaded = 0;
	int blksReaded;
	int totalBlks;
	int i;
	int openFlag;
	int outFd;
	
	RWInfoOfFile rwInfoOfFile;

	
	oneBlkBuf = new char[blkSize];
	if(!oneBlkBuf){
		bRet=false;
		goto errOut;
	}
	
	t_debugMsg(t_dbgLvl,"tw_readASMFile start: fileName=%s blkSize=%d outFileName=%s\n",fileName, blkSize, outFileName);

	t_debugMsg(t_dbgLvl,"tw_readASMFile start11: fileName=%s blkSize=%d outFileName=%s\n",fileName, blkSize, outFileName);
	pRWDataBlks = new RWDataBlocks();
	if(!pRWDataBlks){
		bRet=false;
		t_errMsg("new error\n");
		goto errOut;
	}
	t_debugMsg(t_dbgLvl,"tw_readASMFile start22: fileName=%s blkSize=%d outFileName=%s\n",fileName, blkSize, outFileName);

	

	t_debugMsg(t_dbgLvl,"tw_readASMFile start33: fileName=%s blkSize=%d outFileName=%s\n",fileName, blkSize, outFileName);
	rwInfoOfFile.fileName = (char*)fileName;
	rwInfoOfFile.fileType = FILE_DATA_TYPE;
	rwInfoOfFile.blkSize = blkSize;
	rwInfoOfFile.pQuery = pASMQuery;
	bRet=pRWDataBlks->setFileInfo(&rwInfoOfFile);
	if(!bRet){
		t_errMsg("pRWDataBlks->setFileInfo error\n");
		goto errOut;
	}
	t_debugMsg(t_dbgLvl,"tw_readASMFile start44: fileName=%s blkSize=%d outFileName=%s\n",fileName, blkSize, outFileName);

//	return 0;

    /*
	
	openFlag = TW_OPEN_WRITE|TW_OPEN_CREAT;
	outFd = tw_open(outFileName,openFlag);
	if(outFd<0){
		bRet=false;
		t_errMsg("new error\n");
		goto errOut;
	}
	*/

	//totalBlks = pASM->totalBlksOfFile();
	blksTotalReaded = 0;
	while(true){
		
		//printf("fileName=%s blkOff=%d blksToRead=%d\n", fileName, blksTotalReaded,1);
		blksReaded=pRWDataBlks->readBlks(blksTotalReaded,oneBlkBuf,1);
		if(blksReaded<0){
			bRet = false;
			goto errOut;
		}else if(blksReaded == 0){//读完了
			bRet = true;
			break;
		}

		blksTotalReaded +=blksReaded;

		/*
		i=tw_write(outFd,oneBlkBuf,blkSize);
		if(i != blkSize){
			bRet = false;
			goto errOut;
		}
		*/

	}

errOut:
	if(pRWDataBlks){
		delete pRWDataBlks;
		pRWDataBlks = NULL;
	}
	if(oneBlkBuf){
		delete[] oneBlkBuf;
		oneBlkBuf = NULL;
	}
	return bRet;
}

bool tw_readASMBlks(const char* fileName, const int blkOffInFile_, const int blksToRead, const int blkSize, char* blksBuf,OciQuery* pASMQuery)
{
	bool bRet = true;
	RWDataBlocks* pRWDataBlks = NULL;
	char* oneBlkBuf = NULL;

	int blksTotalReaded = 0;
	int blksReaded;
	int totalBlks;
	int i;
	int openFlag;
	int outFd;
	char *writeFileName="/home/tw/tmp.txt";
	const int grpAuSize = 1024*1024;

	int blkOffInFile= blkOffInFile_;
	
	RWInfoOfFile rwInfoOfFile;

	
	oneBlkBuf = new char[1024*1024];
	if(!oneBlkBuf){
		bRet=false;
		goto errOut;
	}
	
	pRWDataBlks = new RWDataBlocks();
	if(!pRWDataBlks){
		bRet=false;
		t_errMsg("new error\n");
		goto errOut;
	}
	
	rwInfoOfFile.fileName = (char*)fileName;
	rwInfoOfFile.fileType = FILE_DATA_TYPE;
	rwInfoOfFile.blkSize = blkSize;
	rwInfoOfFile.pQuery = pASMQuery;
	bRet=pRWDataBlks->setFileInfo(&rwInfoOfFile);
	if(!bRet){
		t_errMsg("pRWDataBlks->setFileInfo error\n");
		goto errOut;
	}
	
	//totalBlks = pASM->totalBlksOfFile();
	blksTotalReaded = 0;		
	printf("fileName=%s blkOff=%d blksToRead=%d\n", fileName, blksTotalReaded,1);
	blkOffInFile = 0;
//	blksToRead = 1;
	blkOffInFile = 9998;
	while(true){
		t_debugMsg(222,"readBlks:blkOffInFile=%d blksToRead=%d\n", blkOffInFile,blksToRead);
		blkOffInFile= 9984;
		blksReaded=pRWDataBlks->readBlks(blkOffInFile,oneBlkBuf,128);
		t_debugMsg(222,"readBlks:blkOffInFile=%d blksToRead=%d blksReaded=%d\n", blkOffInFile,blksToRead, blksReaded);
		if(blksReaded==0){
			break;
		}

		bRet=mn_checkBlocks((BYTE*)oneBlkBuf,blkOffInFile,128,8192);
		if(!bRet){
			t_errMsg("mn_CheckBlocks:blkOffInFile=%d blksToRead=%d blksReaded=%d\n", blkOffInFile,blksToRead, blksReaded);
		}
		
		break;
		blkOffInFile++;


	/*
	if(blksReaded<0){
		t_errMsg("read error\n");
		bRet = false;
		goto errOut;
	}
	*/
	}

	outFd=open(writeFileName,O_WRONLY);
	if(outFd<0){
		t_errMsg("open failed: filename=%d\n", writeFileName);
		exit(1);
	}
	
	i = write(outFd,oneBlkBuf,grpAuSize);
	if(i != grpAuSize){
		t_errMsg("write error: bytesToWrite=%d bytesWrited=%d\n",grpAuSize, i);
		exit(1);
	}

	
errOut:
	if(pRWDataBlks){
		delete pRWDataBlks;
		pRWDataBlks = NULL;
	}
	if(oneBlkBuf){
		delete[] oneBlkBuf;
		oneBlkBuf = NULL;
	}
	return bRet;
}

bool mn_checkBlocks(BYTE *buf,int startBlock,int blockNum,int  blockSize)
{
	bool bRet = true;
	
	DWORD blockIndex = -1;
	int blockCMP = -1;

	for(int i=0;i<blockNum;i++){
		memcpy(&blockIndex,buf+i*blockSize+_BLOCK_NUM_OFFSET,sizeof(blockIndex));
		blockIndex = blockIndex & _BLOCK_BIT ;
		int blockCMP = (startBlock+i) & _BLOCK_BIT ; //只比较最后_BLOCK_BIT位，大文件表空间使用32位的
		if(blockIndex != blockCMP){
			t_errMsg("CheckBlocks error:blockIndex=%d blockCMP=%d startBlock=%d blockNum=%d blockSize=%d\n", blockIndex,blockCMP,startBlock, blockNum, blockSize);
			bRet=false;
			goto errOut;
		}
	}
errOut:
	return bRet;
}

bool tw_readCommonFile(const char* fileName, const int blkSize, const char* outFileName, OciQuery* pNormalQuery)
{
	bool bRet = true;
	int blkOffInFile= 0;
	const int blksToRead=1;
	const int bytesToRead = blksToRead*blkSize;
	int blksReaded;
	int blksWrited;
	RWDataBlocks *pR_rwDataBlocks = NULL;
	RWDataBlocks *pW_rwDataBlocks = NULL;
	char* blksBuf = NULL;
	
	RWInfoOfFile r_rwInfoOfFile;
	RWInfoOfFile w_rwInfoOfFile;
	
	pR_rwDataBlocks = new RWDataBlocks();
	pW_rwDataBlocks = new RWDataBlocks();
	if(!pR_rwDataBlocks || !pW_rwDataBlocks){
		bRet = false;
		exit(1);
		goto errOut;
	}

	blksBuf = new char[bytesToRead];
	if(!blksBuf){
		bRet=false;
		exit(1);
		goto errOut;
	}
	memset(blksBuf,0,bytesToRead);

	r_rwInfoOfFile.fileName=(char*)fileName;
	r_rwInfoOfFile.blkSize = blkSize;
	bRet=pR_rwDataBlocks->setFileInfo(&r_rwInfoOfFile);
	if(!bRet){
		cout<<"setFileInfo error"<<endl;
		goto errOut;
	}
	
	w_rwInfoOfFile.fileName = (char*)outFileName;
	w_rwInfoOfFile.blkSize = blkSize;
	bRet = pW_rwDataBlocks->setFileInfo(&w_rwInfoOfFile);
	if(!bRet){
		cout<<"setFileInfo Error"<<endl;
		goto errOut;
	}
	
	blkOffInFile = 0;
	while(true){
		printf("fileName=%s blkOff=%d blksToRead=%d\n", fileName, blkOffInFile,blksToRead);
		blksReaded = pR_rwDataBlocks->readBlks(blkOffInFile,blksBuf,blksToRead);
		if(blksReaded<0){
			bRet=false;
			cout<<"读数据文件出错"<<endl;
			goto errOut;
		}else if(blksReaded==0){
			bRet = true;
			cout<<"数据文件读完了"<<endl;
			break;
		}

		blksWrited = pW_rwDataBlocks->writeBlks(blkOffInFile,blksBuf,blksReaded);
		if(blksWrited != blksReaded){
			bRet = false;
			cout<<"写数据文件出错"<<endl;
			goto errOut;
		}

		blkOffInFile += blksWrited;
	}


	
errOut:
	if(pR_rwDataBlocks){
		delete pR_rwDataBlocks;
		pR_rwDataBlocks = NULL;
	}
	if(pW_rwDataBlocks){
		delete pW_rwDataBlocks;
		pW_rwDataBlocks = NULL;
	}
	if(blksBuf){
		delete[] blksBuf;
		blksBuf = NULL;
	}
	return bRet;
}	

ReturnType tw_ConsumerThread(LPVOID *Context)
{
	Consumer myConsumer;
	int fileIndex = 0;
	bool b;
	//static RWDataBlocks* pRWDataBlocks = NULL;
	RWDataBlocks* pRWDataBlocks_local = NULL;
	RWInfoOfFile rwInfoOfFile;

	int blksReaded = 0;
	static char *readBuf = NULL;

	RedologINI configINI;
	OciQuery normalQuery;
	OciQuery asmQuery;

	char usr[128]={0};
	char passwd[128]={0};
	char sid[128]={0};
	char asmUsr[128]={0};
	char asmPasswd[128]={0};
	char asmSID[128]={0};

	configINI.LoadFile("/usr/NETPRO5.02.8/conf/Redolog.ini");

	configINI.LoadValuse("ASM", "USER", asmUsr);
	configINI.LoadValuse("ASM", "PASSWD", asmPasswd);
	configINI.LoadValuse("ASM", "ASM", asmSID);
	asmQuery.SetValses(asmSID,asmUsr,asmPasswd);

	configINI.LoadOCIValuse(usr,passwd, sid);
	normalQuery.SetValses(sid,usr,passwd);

	
	if(!readBuf){
		readBuf = new char[8192];
		if(!readBuf){
			exit(1);
		}
		memset(readBuf, 0, 8192);
	}

	BlockNode CurrentNode;
	t_debugMsg(3,"start ConsumerThread\n");
	while(true){
		while (!myConsumer.IsHaveData()) { //没有数据时，进入死循环
			myConsumer.GiveBack2Pool();
			myConsumer.GetFromProduce();
		}

		fileIndex=myConsumer.GetOneNode(CurrentNode);
		t_debugMsg(3,"GetOneNode: fileIndex=%d blockIndex=%d blocks=%d \n", CurrentNode.FileIndex,CurrentNode.BlockIndex,CurrentNode.Num);
//		continue;

		switch (fileIndex)
		{
		case _ANALYSE_END:		//一般会是OCI出问题，或主备库状态不正确
			break;
		case _BLOCK_EOF:		//一个归档分析结束
			break;
		case _NODE_RECOVER:		//一个归档分析结束
			break;
		case _ANALYSE_EXPAND:
		case _ANALYSE_ADDFILE:
			normalQuery.GetAllDatafileInfo(&G_Datafiles,G_CreateChange);
			G_AnalyseSem.V();
			break;
		default:
			if (fileIndex < 0 ){ //应该大于0的
			}else{
			}
			break;
		}

		continue;

		if(fileIndex<=0){
			continue;
		}

		if(fileIndex<=0 || fileIndex >= G_Datafiles.size() || G_Datafiles[fileIndex].FileName[0] == '\0' || G_Datafiles[fileIndex].FileNum != fileIndex || G_Datafiles[fileIndex].FileSize <= 0)
		{//文件信息有误
			t_errMsg("文件信息不对: fileIndex=%d\n", fileIndex);
			exit(1);
		}

		if(CurrentNode.BlockIndex+CurrentNode.Num > G_Datafiles[fileIndex].Blocks){
			t_errMsg("读越界: fileIndex=%d CurrentNode.BlockIndex=%d CurrentNode.Num=%d G_Datafiles[fileIndex].Blocks=%d\n",fileIndex,CurrentNode.BlockIndex,CurrentNode.Num,G_Datafiles[fileIndex].Blocks);
		}

		t_debugMsg(3,"read datafile: fileIndex=%d blockIndex=%d blocks=%d \n", fileIndex, CurrentNode.BlockIndex, CurrentNode.Num);

		b=RWDataBlocks_map::getInstance()->findRWDataBlocks(fileIndex,&pRWDataBlocks_local);
		if(!b){//not find
			pRWDataBlocks_local=new RWDataBlocks();
			if(!pRWDataBlocks_local){
				exit(1);
			}

			rwInfoOfFile.fileName = G_Datafiles[fileIndex].FileName;
			rwInfoOfFile.fileType = FILE_DATA_TYPE;
			rwInfoOfFile.blkSize  = G_Datafiles[fileIndex].BlockSize;
			rwInfoOfFile.totalBlks = G_Datafiles[fileIndex].Blocks;
			rwInfoOfFile.pQuery   = &asmQuery;//读ASM文件时需要查询相关表 
			
			b=pRWDataBlocks_local->setFileInfo(&rwInfoOfFile);
			if(!b){
				t_errMsg("pRWDataBlocks->setFileInfo: fileInde=%d\n",fileIndex);
				return (ReturnType)-1;
			}		

			b=RWDataBlocks_map::getInstance()->insertRWDataBlocks(fileIndex,pRWDataBlocks_local);
			if(!b){//insert failed
				t_errMsg("insertRWDataBlocks failed\n");
				exit(1);
			}
		}else{//find
			
		}
		
		
		
		t_debugMsg(3,"read datafile: fileIndex=%d blockIndex=%d blocks=%d \n", fileIndex, CurrentNode.BlockIndex, CurrentNode.Num);
		blksReaded = pRWDataBlocks_local->readBlks(CurrentNode.BlockIndex,readBuf,CurrentNode.Num);
		if(blksReaded < CurrentNode.Num){
			t_errMsg("read datafile error: fileIndex=%d blockIndex=%d blocks=%d\n", fileIndex, CurrentNode.BlockIndex, CurrentNode.Num);
			return (ReturnType)-1;
		}

	}
	return (ReturnType)0;
}





/*
ReturnType tw_ConsumerThread(LPVOID *Context)
{
	Consumer myConsumer;
	int fileIndex = 0;
	bool b;
	static RWDataBlocks* pRWDataBlocks = NULL;
	RWInfoOfFile rwInfoOfFile;

	int blksReaded = 0;
	static char *readBuf = NULL;

	RedologINI configINI;
	OciQuery normalQuery;
	OciQuery asmQuery;

	char usr[128]={0};
	char passwd[128]={0};
	char sid[128]={0};
	char asmUsr[128]={0};
	char asmPasswd[128]={0};
	char asmSID[128]={0};

	configINI.LoadFile("/usr/NETPRO5.02.8/conf/Redolog.ini");

	configINI.LoadValuse("ASM", "USER", asmUsr);
	configINI.LoadValuse("ASM", "PASSWD", asmPasswd);
	configINI.LoadValuse("ASM", "ASM", asmSID);
	asmQuery.SetValses(asmSID,asmUsr,asmPasswd);

	configINI.LoadOCIValuse(usr,passwd, sid);
	normalQuery.SetValses(sid,usr,passwd);

	if(!pRWDataBlocks){
		pRWDataBlocks=new RWDataBlocks();
		if(!pRWDataBlocks){
			exit(1);
		}
	}

	if(!readBuf){
		readBuf = new char[8192];
		if(!readBuf){
			exit(1);
		}
		memset(readBuf, 0, 8192);
	}

	BlockNode CurrentNode;
	t_debugMsg(3,"start ConsumerThread\n");
	while(true){
		while (!myConsumer.IsHaveData()) { //没有数据时，进入死循环
			myConsumer.GiveBack2Pool();
			myConsumer.GetFromProduce();
		}

		fileIndex=myConsumer.GetOneNode(CurrentNode);
		t_debugMsg(3,"GetOneNode: fileIndex=%d blockIndex=%d blocks=%d \n", CurrentNode.FileIndex,CurrentNode.BlockIndex,CurrentNode.Num);
		//continue;

		switch (fileIndex)
		{
		case _ANALYSE_END:		//一般会是OCI出问题，或主备库状态不正确
			break;
		case _BLOCK_EOF:		//一个归档分析结束
			break;
		case _NODE_RECOVER:		//一个归档分析结束
			break;
		case _ANALYSE_EXPAND:
		case _ANALYSE_ADDFILE:
			normalQuery.GetAllDatafileInfo(&G_Datafiles,G_CreateChange);
			G_AnalyseSem.V();
			break;
		default:
			if (fileIndex < 0 ){ //应该大于0的
			}else{
			}
			break;
		}
		
		if(fileIndex<=0){
			continue;
		}

		if(fileIndex<=0 || fileIndex >= G_Datafiles.size() || G_Datafiles[fileIndex].FileName[0] == '\0' || G_Datafiles[fileIndex].FileNum != fileIndex || G_Datafiles[fileIndex].FileSize <= 0)
		{//文件信息有误
			t_errMsg("文件信息不对: fileIndex=%d\n", fileIndex);
			exit(1);
		}

		if(CurrentNode.BlockIndex+CurrentNode.Num > G_Datafiles[fileIndex].Blocks){
			t_errMsg("读越界: fileIndex=%d CurrentNode.BlockIndex=%d CurrentNode.Num=%d G_Datafiles[fileIndex].Blocks=%d\n",fileIndex,CurrentNode.BlockIndex,CurrentNode.Num,G_Datafiles[fileIndex].Blocks);
		}

		t_debugMsg(3,"read datafile: fileIndex=%d blockIndex=%d blocks=%d \n", fileIndex, CurrentNode.BlockIndex, CurrentNode.Num);


		rwInfoOfFile.fileName = G_Datafiles[fileIndex].FileName;
		rwInfoOfFile.fileType = FILE_DATA_TYPE;
		rwInfoOfFile.blkSize  = G_Datafiles[fileIndex].BlockSize;
		rwInfoOfFile.totalBlks = G_Datafiles[fileIndex].Blocks;
		rwInfoOfFile.pQuery   = &asmQuery;//读ASM文件时需要查询相关表
		b=pRWDataBlocks->setFileInfo(&rwInfoOfFile);
		if(!b){
			t_errMsg("pRWDataBlocks->setFileInfo: fileInde=%d\n",fileIndex);
			return (ReturnType)-1;
		}
		
		t_debugMsg(3,"read datafile: fileIndex=%d blockIndex=%d blocks=%d \n", fileIndex, CurrentNode.BlockIndex, CurrentNode.Num);
		blksReaded = pRWDataBlocks->readBlks(CurrentNode.BlockIndex,readBuf,CurrentNode.Num);
		if(blksReaded < CurrentNode.Num){
			t_errMsg("read datafile error: fileIndex=%d blockIndex=%d blocks=%d\n", fileIndex, CurrentNode.BlockIndex, CurrentNode.Num);
			return (ReturnType)-1;
		}

	}
	return (ReturnType)0;
}
*/
#endif




#if 0
/////////////////////////////////////////////////////////////////wqy测试时候的main//////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <list>

#include "Defines.h"
//tw changed
//#include "BlockPool.h"
// is not tw changed
//#include "MyLog.h"
#include "ChuckSum.h"
#include "RedologINI.h"
#include "OciQuery.h"
#include "tw_api.h"

#define WITHASM

// tw changed
//extern ResourceList Tranlist;
//extern ResourceList ResourcePool;
extern Semaphore G_AnalyseSem;

extern Datafiles G_Datafiles;
extern BYTE8	G_CreateChange;
extern vector<vector<DISK> > G_Disks;
extern vector<Group> G_Groups;

ReturnType ConsumerThread(LPVOID *Context)
{
	Consumer myConsumer;
	BlockNode CurrentNode;
	FILE *fh = fopen("Consumer.txt", "a");

	while (true)
	{		
		if (myConsumer.GetFromProduce() == false){
			std::cout<<"..........LOCKED...........\n";
			fdsleep(500);
			continue;
		}

		if(myConsumer.IsHaveData() == false){
			cout<<"myConsumer Have no Data\n";
			continue;
		}else
			cout<<"myConsumer Have Data\n";
		
//		fprintf(fh,"Before--------%d------\n",ResourcePool.size());
		while(myConsumer.GetOneNode(CurrentNode)){
			fprintf(fh,"%d\t%d\t%d\n",CurrentNode.FileIndex,CurrentNode.BlockIndex,CurrentNode.Num);
			if(CurrentNode.FileIndex == _ANALYSE_EXPAND){
				cout<<"-----CurrentNode.FileIndex == _ANALYSE_EXPAND"<<CurrentNode.BlockIndex<<'\t'<<CurrentNode.Num<<endl;
				G_AnalyseSem.V();
			}
		}
		myConsumer.GiveBack2Pool();
		fprintf(fh,"After--------%d------\n",ResourcePool.size());
		fflush(fh);
	}
	fclose(fh);
	return (ReturnType)0;
}

extern vector<vector<DISK> > G_Disks;
extern vector<Group> G_Groups;

int wqy_main(int argc, char** argv)
{
	if(argc < 2){
		cout<<"参数个数不对"<<endl;
		return 0;
	}

	ThreadID dwThreadID;
	HANDLE hThread = MyCreateThread(&dwThreadID,ConsumerThread,NULL);
	
	RedologINI ConfigINI;
	ConfigINI.LoadFile("./Redolog.ini");

	char strUSER_ASM[MAX_NAME_LENGTH]= {0};
	char strPASSWD_ASM[MAX_NAME_LENGTH] = {0};
	char strSID_ASM[MAX_NAME_LENGTH] = {0};
	ConfigINI.LoadValuse("ASM", "USER", strUSER_ASM);
	ConfigINI.LoadValuse("ASM", "PASSWD", strPASSWD_ASM);
	ConfigINI.LoadValuse("ASM", "ASM", strSID_ASM);

	OciQuery ASMQuery;
	ASMQuery.SetValses(strSID_ASM,strUSER_ASM,strPASSWD_ASM);
	
	cout<<strUSER_ASM<<endl<<strPASSWD_ASM<<endl<<strSID_ASM<<endl;

	
	OciQuery myQuery;
	char strUSER[MAX_NAME_LENGTH]= {0};
	char strPASSWD[MAX_NAME_LENGTH] = {0};
	char strSID[MAX_NAME_LENGTH] = {0};
	if(false == ConfigINI.LoadOCIValuse(strUSER,strPASSWD, strSID)){
		return -1;
	}
	myQuery.SetValses(strSID,strUSER,strPASSWD);
	myQuery.GetAllDatafileInfo(&G_Datafiles,G_CreateChange);

	LoadAllDisksOCI();

	string strName = argv[1];//文件名
	string strSeq = argv[2];

	int	nseq = atoi(strSeq.c_str());

	RedoLog LogFile;

	char Arch[128] = {0};
	ConfigINI.LoadValuse("LOG","ASM",Arch);

//  ASM相关
	bool bASM = false;
	if(0 == strcmp(Arch,"YES")){
		bASM = true;
	}

	if(LogFile.LoadNewFile((char *)strName.c_str(),_INACTIVE,nseq,bASM,&ASMQuery) < 0){
		cout<<"LoadNewFile ERROR"<<endl;
		return -1;
	}
	LogFile.LoadandSkipLogHead();
	cout<<"LoadandSkipLogHead --- OK\n";
	cout<<"LogFile.BeginSCN:\t"<<LogFile.BeginSCN<<endl;
	if(true == LogFile.Loadall()){
		cout<<"OK .... SaveSCN:\t"<<LogFile.BeginSCN<<endl;
	}
	else{
		cout<<"Loadall() ERROR"<<endl;
		int BlockNum,SCN,Offset;
		char strMes[256] = {0};
		LogFile.GetErrorMsg(BlockNum,SCN,Offset,strMes);
		cout<<BlockNum<<endl
			<<SCN<<endl
			<<Offset<<endl
			<<strMes<<endl;
	}
	
	int num=5;
	while(num--){
		sleep(1);
	}

	return 0;
}

int tw_main(int argc,char** argv)
{
	int iRet=0;
	char nowTimeStr[256]={0};
	test_print();
	tw_time_localNowTimeStr_s(nowTimeStr,256);
	t_debugMsg(1,"hello tianwei,now local time is:%s\n",nowTimeStr);
	return iRet;
}

int main(int argc,char** argv)
{
	return tw_main(argc,argv);
}

#endif

