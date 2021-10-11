#include "lgc_Defines.h"
#include "lgc_Structure.h"

#include "OciQuery.h"
#include "RedologINI.h"

#include "lgc_RedoFile.h"
#include "lgc_RedoRecord.h"
#include "lgc_Change.h"
#include "lgc_Transaction.h"

#include "lgc_RedoFileInput.h"
#include "lgc_RedoRecordInput.h"
#include "lgc_ChangeInput.h"

#include "lgc_RecordHandler.h"
#include "lgc_ChangeHandler.h"

#include "lgc_TransactionMgr.h"

#include "lgc_param.h"
#include "lgc_tableMeta.h"
#include "lgc_readRedoFile.h"

#include "lgc_ExtractThread.h"
#include "lgc_MergeThread.h"
#include "lgc_ConsumeThread.h"

int redoFileInput_main(int argc, char* argv[]);


extern vector< vector<DISK> > G_Disks;
extern vector<Group> G_Groups;

bool tw_loadGroupsInfo(OciQuery* pASMQuery, RedologINI* pConfigINI);
void lgc_extract_init(const char* paramFileName);


int main(int argc, char* argv[])
{	
	int iFuncRet = 0;
	
	const char*	paramFileName = "/home/tw/lgc/conf/extract.ini";
	LGC_Param*	pParam		  = LGC_Param::getInstance();

	if(pParam->loadParamFile(paramFileName) == false){
		lgc_errMsg("loadParamFile failed \n");
		return -1;
	}

	const char*				tnsname		= pParam->getOciSid();
	const char*				password	= pParam->getOciPasswd();
	const char*				username	= pParam->getOciUser();
	const char*				mediaDir	= pParam->getMediaFileDir();
	const BYTE8				startSCN	= pParam->getStartSCNOfExtract();
	const unsigned short	threads		= pParam->getThreadCount();

	pthread_t				extractTidArray[threads+1];
	LGC_ExtractThreadArg	extractThreadArgArray[threads+1];
	
	//初始化程序
	lgc_extract_init(paramFileName);

	//创建抽取线程
	for(int threadId = 1; threadId <= threads; threadId++){
		extractThreadArgArray[threadId].threadId = threadId;
		extractThreadArgArray[threadId].startSCN = startSCN;
		extractThreadArgArray[threadId].threads  = threads;
		extractThreadArgArray[threadId].tnsname  = tnsname;
		extractThreadArgArray[threadId].password = password;
		extractThreadArgArray[threadId].username = username;

		iFuncRet = pthread_create(&extractTidArray[threadId],
			                      NULL, 
			                      extractThreadFunc, 
			                      &extractThreadArgArray[threadId]);
		if(iFuncRet != 0 ){
			lgc_errMsg("create extract thread failed:%u \n", threadId);
			return -1;
		}
		fprintf(stdout, "extractThread created:threadId=%u tid=%u \n", threadId, extractTidArray[threadId]);
	}
	
	//创建合并线程
	pthread_t			mergeTid;
	LGC_MergeThreadArg	mergeThreadArg;

	mergeThreadArg.threads = threads;

	iFuncRet = pthread_create(&mergeTid, 
		                      NULL, 
		                      mergeThreadFunc, 
		                      &mergeThreadArg);
	if(iFuncRet != 0){
		lgc_errMsg("create merge thread failed:\n");
		return -1;
	}
	fprintf(stdout, "merge thread created: tid=%u \n", mergeTid);
	
	//创建消费线程
	pthread_t				consumeTid;
	LGC_ConsumeThreadArg	consumeThreadArg;

	mergeThreadArg.threads		 = threads;
	mergeThreadArg.mediaFileDir  = mediaDir;

	iFuncRet = pthread_create(&consumeTid,
		                      NULL, 
		                      consumeThreadFunc, 
		                      &consumeThreadArg);
	if(iFuncRet != 0){
		lgc_errMsg("create consume thread failed: \n");
		return -1;
	}
	fprintf(stdout, "consume thread created: tid=%u \n", consumeTid);

	//主线程等待
	void* tret;
	for(int threadId = 1; threadId <= threads; threadId++){
		iFuncRet = pthread_join(extractTidArray[threadId],&tret);
		if(iFuncRet != 0){
			lgc_errMsg("pthread_join failed \n");
			return -1;
		}
	}

	iFuncRet = pthread_join(mergeTid,&tret);
	if(iFuncRet != 0){
		lgc_errMsg("pthread_join failed \n");
		return -1;
	}

	iFuncRet = pthread_join(consumeTid, &tret);
	if(iFuncRet != 0){
		lgc_errMsg("pthread_join failed \n");
		return -1;
	}

	fprintf(stdout, "all sub thread exited: \n");
	
	return 0;
}


void lgc_extract_init(const char* paramFileName)
{
	bool b = true;

	RedologINI configINI;
	
	char asmUsr[125]    = {0};
	char asmPasswd[125] = {0};
	char asmSID[125]    = {0};
	char usr[125]       = {0};
	char passwd[125]    = {125};
	char sid[128]       = {0};
	char arch[128]      = {0};

	OciQuery asmQuery;
	OciQuery normalQuery;
	
	//加载参数文件
	configINI.LoadFile((char*)paramFileName);

	configINI.LoadValuse(		"ASM","USER",  asmUsr);
	configINI.LoadValuse(		"ASM","PASSWD",asmPasswd);
	configINI.LoadValuse(		"ASM","ASM",   asmSID);
	configINI.LoadOCIValuse(	usr,  passwd,  sid);
	configINI.LoadValuse(		"LOG","ASM",   arch);

	asmQuery.SetValses(    asmSID,asmUsr,asmPasswd);
	normalQuery.SetValses( sid,   usr,   passwd);
	
	//如果是rac, 则加载磁盘组信息
	bool bASM = false;
	if(strcmp(arch,"YES") == 0){
		bASM = true;
	}
	if(bASM){
		b=tw_loadGroupsInfo(&asmQuery,&configINI);
		if(!b){
			lgc_errMsg("loadGroupInfo failed");
			exit(1);
		}
	}
	
	//加载需要分析的标的meta info
	b = LGC_TblsMeta_Mgr::getInstance()->loadTablesMetaInfo();
	if(!b){
		lgc_errMsg("loadTablesMetaInfo failed\n");
		exit(1);
	}

	return;
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
