//**************************************************************
//*
//* 文件名: OciQuery.cpp
//* 设 计 者：wqy 日期：2012-5-22
//* 修 改 者：    日期：2012-5-22
//*
//****************************************************************

#include "OciQuery.h"
#include "RedologINI.h"
#include "tw_api.h"
#include "lgc_tableMeta.h"
#include "lgc_api.h"

#include <string>
#include <iostream> 
#include <sstream> 
#include <vector>
#include <stdlib.h>

#include <unistd.h>

#include "Unit.h"

Datafiles G_Datafiles;
BYTE8	G_CreateChange;

using namespace std;

/*
Datafile::Datafile(const Datafile& Node)
{
	this->FileNum = Node.FileNum;
	this->CreationChange = Node.CreationChange;
	strncpy(this->FileName,Node.FileName,sizeof(this->FileName));
	this->FileSize = Node.FileSize;
	this->bExpand = Node.bExpand;
	this->ASMFileIndex = Node.ASMFileIndex;
	this->GroupNum = Node.GroupNum;
	this->BlockSize = Node.BlockSize;
	this->Blocks = Node.Blocks;
	if(this->pASMFile){
		if(Node.pASMFile){
			delete this->pASMFile;
			this->pASMFile = Node.pASMFile;
		}else{
			this->pASMFile->Clear();
		}
	}else{
		this->pASMFile = Node.pASMFile;
	}
}

Datafile& Datafile::operator=( const Datafile& b)
{
	this->FileNum = b.FileNum;
	this->CreationChange = b.CreationChange;
	strncpy(this->FileName,b.FileName,sizeof(this->FileName));
	this->FileSize = b.FileSize;
	this->bExpand = b.bExpand;
	this->ASMFileIndex = b.ASMFileIndex;
	this->GroupNum = b.GroupNum;
	this->BlockSize = b.BlockSize;
	this->Blocks = b.Blocks;
	//this->pASMFile = b.pASMFile;
	if(this->pASMFile){
		if(b.pASMFile){
			delete this->pASMFile;
			this->pASMFile = b.pASMFile;
		}else{
			this->pASMFile->Clear();
		}
	}else{
		this->pASMFile = b.pASMFile;
	}
	return *this;
}
*/

Datafile::Datafile(const Datafile& Node)
{
	this->FileNum = Node.FileNum;
	this->CreationChange = Node.CreationChange;
	strncpy(this->FileName,Node.FileName,sizeof(this->FileName));
	this->FileSize = Node.FileSize;
	this->bExpand = Node.bExpand;
	this->ASMFileIndex = Node.ASMFileIndex;
	this->GroupNum = Node.GroupNum;
	this->BlockSize = Node.BlockSize;
	this->Blocks = Node.Blocks;
	this->pASMFile = Node.pASMFile;
}

Datafile& Datafile::operator=( const Datafile& b)
{
	this->FileNum = b.FileNum;
	this->CreationChange = b.CreationChange;
	strncpy(this->FileName,b.FileName,sizeof(this->FileName));
	this->FileSize = b.FileSize;
	this->bExpand = b.bExpand;
	this->ASMFileIndex = b.ASMFileIndex;
	this->GroupNum = b.GroupNum;
	this->BlockSize = b.BlockSize;
	this->Blocks = b.Blocks;
	//this->pASMFile = b.pASMFile;
	if(this->pASMFile){
		if(b.pASMFile){
			delete this->pASMFile;
			this->pASMFile = b.pASMFile;
		}else{
			this->pASMFile->Clear();
		}
	}else{
		this->pASMFile = b.pASMFile;
	}
	return *this;
}

int GetStateDate(void *pList,const char *pVal,char* FileNum)
{
	string strTemp;
	stringstream sstr(pVal);
	int Stat= 0;

	getline(sstr,strTemp,'\n');
	if(strTemp == "PRIMARY"){
		Stat |= ORACLE_PRIMARY;
	}else if(strTemp == "PHYSICAL STANDBY"){
		Stat |= ORACLE_STANDBY;
	}else{
		Stat = STATE_UNKNOW;
		return QEURY_END_DATA;
	}
#ifdef _DEBUG_QUERY_ON_2
	printf("%s\t0x%x\t",strTemp.c_str(),Stat);
#endif
	getline(sstr,strTemp,'\n');
	if(strTemp == "MOUNTED"){
		Stat |= ORACLE_MOUNT;
	}else if(strTemp == "READ ONLY WITH APPLY"){
		Stat |= ORACLE_OPEN_APPLY;
	}else{
		Stat |= ORACLE_OPEN;
	}
#ifdef _DEBUG_QUERY_ON_2
	printf("%s\t0x%x\t",strTemp.c_str(),Stat);
#endif
	*((int *)pList) = Stat;
	return QEURY_END_DATA;
}

int GetSyncStateDate(void *pList,const char *pVal,char* FileNum)
{
	string strTemp;
	long LSeq = 0;
	stringstream sstr(pVal);

	sstr>>LSeq;
	if(0 == LSeq){
		*((int *)pList) = 0;
		return QEURY_END_DATA;
	}

	sstr>>strTemp;
	if(strTemp != "YES"){
		LSeq = LSeq*(-1);
	}

	*((long *)pList) = LSeq;
	return QEURY_END_DATA;
}
//tw changed
// "select file#,name,creation_change#,bytes,block_size from v$datafile "
int QueryDatafileInfo(void *pList,const char *pVal,char* FileNum)
{
	string strName;
	if (pVal)
	{
		Datafiles *pdatafiles = (Datafiles *)pList;
		stringstream sstr(pVal);
		Datafile NewDatafile;
		sstr>>NewDatafile.FileNum;
		sstr>>strName;
		sstr>>NewDatafile.CreationChange;
		sstr>>NewDatafile.FileSize;
		sstr>>NewDatafile.Blocks;
		sstr>>NewDatafile.BlockSize;

		int VecSize = pdatafiles->size();
		if (NewDatafile.FileNum >= VecSize){
			pdatafiles->resize(NewDatafile.FileNum+10);
		}
		strncpy(NewDatafile.FileName,strName.c_str(),sizeof(NewDatafile.FileName));
		pdatafiles->at(NewDatafile.FileNum) = NewDatafile;
	}
	return QEURY_MORE_DATA;
}

int GetDataFile(void *pList,const char *pVal,char* FileNum)
{
	*(string *)pList = pVal;
	return QEURY_END_DATA;
}

int GetDataFileALL(void *pList,const char *pVal,char* FileNum)
{
	*(string *)pList += pVal;
	*(string *)pList += ";";
	return QEURY_MORE_DATA;
}

int GetDataChars(void *pList,const char *pVal,char* FileNum)
{
	if(pVal)
		strcpy((char *)pList,pVal);
	return QEURY_END_DATA;
}

int GetNumDate(void *pList,const char *pVal,char* FileNum)
{
	if (pVal)
	{
		ASMFile *pASMFile = (ASMFile *)pList;
		stringstream sstr(pVal);
		sstr>>pASMFile->ASMFileIndex;
		sstr>>pASMFile->BlockSize;
		sstr>>pASMFile->FileSize;
		sstr>>pASMFile->Blocks;
	}
	return QEURY_END_DATA;
}

int GetAllGroups(void *pList,const char *pVal,char* FileNum)
{
	if (pVal)
	{
		VGroups *pGroups = (VGroups *)pList;
		Group oneGroup;
		stringstream sstr(pVal);
		sstr>>oneGroup.GroupNum;
		sstr>>oneGroup.Block_Size;
		sstr>>oneGroup.AU_Size;
		sstr>>oneGroup.Name;
		pGroups->push_back(oneGroup);
	}
	return QEURY_MORE_DATA;
}

int GetGroup(void *pList,const char *pVal,char* FileNum)
{
	if (pVal)
	{
		Group *pGroup = (Group *)pList;
		stringstream sstr(pVal);
		sstr>>pGroup->GroupNum;
		sstr>>pGroup->Block_Size;
		sstr>>pGroup->AU_Size;
	}
	return QEURY_END_DATA;
}

int GetSource(void *pList,const char *pVal,char* FileNum)
{
	if (pVal)
	{
		ASMFile *pASMFile = (ASMFile *)pList;
		stringstream sstr(pVal);
		sstr>>pASMFile->curDiskNum;
		sstr>>pASMFile->curAuOffInDisk;
	}
	return QEURY_END_DATA;
}

int GetAllDisks(void *pList,const char *pVal,char* FileNum)
{
	if (pVal)
	{
		DISK OneDisk;
		VVDisks *pVDisks = (VVDisks *)pList;
		stringstream sstr(pVal);

		sstr>>OneDisk.GroupNum;
		sstr>>OneDisk.DiskNum;
		sstr>>OneDisk.Path;

		if(OneDisk.GroupNum>=0 && OneDisk.GroupNum<pVDisks->size()){
			VDisks *pDisks = &(pVDisks->at(OneDisk.GroupNum));
			pDisks->push_back(OneDisk);
		}		
	}
	return QEURY_MORE_DATA;
}

int GetDiskInfo(void *pList,const char *pVal,char* FileNum)
{
	if (pVal)
	{
		DISK OneDisk;
		VDisks *pDisks = (VDisks *)pList;
		stringstream sstr(pVal);

		sstr>>OneDisk.GroupNum;
		sstr>>OneDisk.DiskNum;
		sstr>>OneDisk.Path;
		pDisks->push_back(OneDisk);

	}
	return QEURY_MORE_DATA;
}


int GetArchivedLog(void *pList,const char *pVal,char* FileNum)
{
	if (pVal)
	{
		stringstream sstr(pVal);
		RedologFile newFile;

		sstr>>newFile.Thread;
		sstr>>newFile.Sequence;

#ifdef _WIN32
		char strSCN[128] = {0};
		sstr>>strSCN;
		newFile.BeginSCN = Str2BYTE8(strSCN);	
#else
		sstr>>newFile.BeginSCN;
#endif
	
#ifdef _WIN32
		sstr>>strSCN;
		newFile.NEXTSCN = Str2BYTE8(strSCN);	
#else
		sstr>>newFile.NEXTSCN;
#endif

		sstr>>newFile.FileName;
	
		if (newFile.FileName[0] == 0){
			return QEURY_MORE_DATA;
		}

		Redologs *pVec = (Redologs *)pList;
		pVec->push_back(newFile);
	}
	return QEURY_MORE_DATA;
}

int GetRedolog(void *pList,const char *pVal,char* FieldNum)
{
	if (pVal)
	{
		int* num = (int *)FieldNum;
		if (*num != -0xFF)	//无限
		{		
			if(*num < 1){
				return QEURY_END_DATA;
			}else{
				--(*num);
			}
		}
		string strVal;
		stringstream sstr(pVal);

		RedologFile newFile;

		sstr>>newFile.Thread;
		sstr>>newFile.Sequence;
		sstr>>strVal;
		sstr>>newFile.BeginSCN; //这里的变量都用指针,就可以用循环还做了
		sstr>>newFile.FileName;

		if (!strVal.compare("INACTIVE")){ //相等
			newFile.nStatus = _INACTIVE;
		}else if(!strVal.compare("CURRENT")){
			newFile.nStatus = _CURRENT;
		}else if(!strVal.compare("ACTIVE")){
			newFile.nStatus = _ACTIVE;
		}
		
		Redologs *pVec = (Redologs *)pList;
		pVec->push_back(newFile);
	}
	return QEURY_MORE_DATA;
}

OciQuery::OciQuery()
{
	memset(SID,0,sizeof(SID));
	memset(USER,0,sizeof(USER));
	memset(PASSWD,0,sizeof(PASSWD));
	memset(QuerySql,0,sizeof(QuerySql));
	bConnected = false;
}

OciQuery::OciQuery(const char *sid,const char *user, const char *passwd)
{
	memset(SID,0,sizeof(SID));
	memset(USER,0,sizeof(USER));
	memset(PASSWD,0,sizeof(PASSWD));
	memset(QuerySql,0,sizeof(QuerySql));
	bConnected = false;
	SetValses(sid,user,passwd);
}

OciQuery::~OciQuery()
{
	bConnected = false;
	close();
}

// OCI_CONNECT_RETRYTIME次重连数据库,都连不上才算连接错误
bool OciQuery::ConnectOCI()
{
	bool RetVal = false;
	if(TestConnect() == true){
		RetVal = true;
		return RetVal;
	}
    else{
		cout <<"CONNECT ERROR "<<m_OciApi.ErrorMsg<<endl;
		close();
		cout <<"close----- OK "<<endl;
	}

	for(int i=0;i<OCI_CONNECT_RETRYTIME;i++){
		if(0 == m_OciApi.Connect(USER,PASSWD,SID)){
			RetVal = true;
			break;
		}
		cout<<"ConnectOCI "<<i<<" Error Info:"<<m_OciApi.ErrorMsg<<endl;
		cout<<"sleep 1 second, retry..\t"<<USER<<" "<<PASSWD<<" "<<SID<<endl;
		t_debugMsg(t_dbgLvl,"ConnectOCI Error: %s\n",m_OciApi.ErrorMsg);
		t_debugMsg(t_dbgLvl,"sleep %d seconds, then retry: USER=%s PASSWD=%s SID=%s\n",1,USER,PASSWD,SID);
		sleep(1);
	}

	// 上面连5次都连不上后，每1分钟连10次
	for(int i=0;(RetVal == false) && i<OCI_CONNECT_RETRYTIME*2;i++){
		if(0 == m_OciApi.Connect(USER,PASSWD,SID)){
			RetVal = true;
			break;
		}
		cout<<"ConnectOCI "<<i<<" Error Info:"<<m_OciApi.ErrorMsg<<endl;
		cout<<"sleep 60 second, retry.."<<endl;
		t_errMsg("ConnectOCI Error: %s\n",m_OciApi.ErrorMsg);
		t_errMsg("sleep %d seconds, then retry: USER=%s PASSWD=%s SID=%s\n",60,USER, PASSWD, SID);
		sleep(60);
	}

	// 再连不上，就每5分钟无限连
	while(RetVal == false)
	{
		if(0 == m_OciApi.Connect(USER,PASSWD,SID)){
			RetVal = true;
			break;
		}
		cout<<"ConnectOCI Error Info:"<<m_OciApi.ErrorMsg<<endl;
		cout<<"sleep 300 second, retry.."<<endl;
		t_errMsg("ConnectOCI Error: %s\n",m_OciApi.ErrorMsg);
		t_errMsg("sleep %d seconds, then retry: USER=%s PASSWD=%s SID=%s\n",300,USER,PASSWD,SID);
		sleep(300);
	}
	return RetVal;
}

void OciQuery::SetValses(const char *sid,const char *user, const char *passwd)
{
	if (sid && user && passwd)
	{
		strncpy(SID,sid,sizeof(SID));
		strncpy(USER,user,sizeof(USER));
		strncpy(PASSWD,passwd,sizeof(PASSWD));
		SID[sizeof(SID)-1] = 0;
		USER[sizeof(USER)-1] = 0;
		PASSWD[sizeof(PASSWD)-1] = 0;
	}
}

//"select sequence#,first_change#,name from v$archived_log where first_change#>%d %s and first_time > to_date('%s','yyyy-MM-dd')"
bool OciQuery::GetArchivedLogBySCN_DATE(Redologs &Logs,BYTE8 SCN,const char* DATE,BYTE8 Rac2SCN)
{
	bool RetVal = true;
#ifdef _DEBUG_QUERY_ON
		cout<<USER<<'\t'<<PASSWD<<'\t'<<SID<<endl;
#endif
	if(ConnectOCI()){
		memset(QuerySql,0,sizeof(QuerySql));
		if(NULL == DATE){
#ifdef	ORACLE_RAC
		BYTE8 SCN2 = 0;
		if(Rac2SCN == 0){
			SCN2 = SCN;
		}else{
			SCN2 = Rac2SCN;
		}
		sprintf(QuerySql,SELECT_LOG_SCN,SCN,SCN2);
#else
		sprintf(QuerySql,SELECT_LOG_SCN,SCN);
#endif			
		}else{
#ifdef _WIN32
			string strQuery = SELECT_LOG_SCN_DATA;
			int PosDATA = strQuery.find("%s");
			strQuery.replace(PosDATA,2,DATE);
			sprintf(QuerySql,strQuery.c_str(),SCN);
#else
			sprintf(QuerySql,SELECT_LOG_SCN_DATA,SCN,DATE);
#endif
		}
#ifdef _DEBUG_QUERY_ON
		cout<<"Query:\t"<<QuerySql<<endl;
#endif
		if(m_OciApi.ExecSQL(QuerySql,&GetArchivedLog,(void *)&Logs,5)){
			RetVal = false;
		}	
	}
#ifdef _DEBUG_QUERY_ON
	else{
		cout<<"Connect Error:\t"<<m_OciApi.ErrorMsg<<endl;
		RetVal = false;
	}
#endif
	return RetVal;
}

bool OciQuery::GetArchivedLogBySCN_DATE2(Redologs &Logs,BYTE8 SCN,const char* DATE,BYTE8 Rac2SCN)
{
	bool RetVal = true;
	if(ConnectOCI()){
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,SELECT_LOG_SCN2);

		BindDatas myDatas(2);
		myDatas[0].DataLong = SCN;
		myDatas[0].Type = SQLT_LNG;
		int DataNum = 1;

#ifdef	ORACLE_RAC
		BYTE8 SCN2 = 0;
		if(Rac2SCN == 0){
			SCN2 = SCN;
		}else{
			SCN2 = Rac2SCN;
		}
		myDatas[1].DataLong = SCN2;
		myDatas[1].Type = SQLT_LNG;
		DataNum = 2;
#endif	
	
#ifdef _DEBUG_QUERY_ON
		cout<<"Query:\t"<<QuerySql<<endl;
#endif
		if(m_OciApi.ExecSQL(QuerySql,&GetArchivedLog,(void *)&Logs,5,myDatas,DataNum)){
			RetVal = false;
		}	
	}
#ifdef _DEBUG_QUERY_ON
	else{
		cout<<"Connect Error:\t"<<m_OciApi.ErrorMsg<<endl;
		RetVal = false;
	}
#endif
	return RetVal;
}
bool OciQuery::GetArchivedLogBySCN(char *Logfile,long SCN)
{
	bool RetVal = true;
	if(ConnectOCI()){	
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,SELECT_ARGLOG,SCN);
#ifdef _DEBUG_QUERY_ON
		printf("%s\n", QuerySql);
#endif
		if(m_OciApi.ExecSQL1(QuerySql,&GetDataChars,(void *)Logfile)){
			RetVal = false;
		}
	}
	else{
		RetVal=false;
	}
	return RetVal;
}

bool OciQuery::GetArchivedLogBySEQ(char *Logfile,BYTE8 *SCN, long SEQ)
{
	bool RetVal = true;
	if(ConnectOCI()){	
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,SELECT_ARGLOG_SEQ,SEQ);
#ifdef _DEBUG_QUERY_ON
		printf("%s\n", QuerySql);
#endif
		char strTemp[512] = {0};
		if(m_OciApi.ExecSQL(QuerySql,&GetDataChars,(void *)strTemp,2)){
			RetVal = false;
			return RetVal;
		}
		stringstream sstr(strTemp);
		sstr>>(*SCN);
		sstr>>Logfile;
	}
	else{
		RetVal=false;
	}
	return RetVal;
}

bool OciQuery::GetArchivedLogName(Redologs &Logs)
{
	bool RetVal = true;
	if(ConnectOCI()){		
		if(m_OciApi.ExecSQL2(SELECT_LOG_SCN,&GetArchivedLog,(void *)&Logs)){
			RetVal = false;
		}		
	}
	else{
		RetVal=false;
	}
	return RetVal;
}

bool OciQuery::GetRedologNames(Redologs &Logs)
{
	bool RetVal = true;
	if(ConnectOCI()){
		int num = 4;
		if(m_OciApi.ExecSQL(SELECT_REDOLOG,&GetRedolog,(void *)&Logs,num)){
			RetVal = false;
		}
	}
	else{
		RetVal=false;
	}
	return RetVal;
}

bool OciQuery::GetCurrentRedolog(Redologs &Logs)
{
	bool RetVal = true;
	if(ConnectOCI()){
		int num = 4;
		if(m_OciApi.ExecSQL(SELECT_REDOLOG_CURRENT,&GetRedolog,(void *)&Logs,num)){
			RetVal = false;
		}
	}
	else{
		RetVal=false;
	}
	return RetVal;
}

bool OciQuery::GetRedologInactive(Redologs &Logs,int linenum,int SCN)
{
	bool RetVal = true;
	if(ConnectOCI()){
		int num = (linenum<<16)|4;
//		"select b.sequence#,b.status,b.first_change#,a.member from v$logfile a ,v$log b where a.group#=b.group# and b.status='%s' and b.first_change#>%d  order by b.sequence#"
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,SELECT_REDOLOG_BY,"INACTIVE",SCN);
		if(m_OciApi.ExecSQL(QuerySql,&GetRedolog,(void *)&Logs,num)){
			RetVal = false;
		}
	}
	else{
		RetVal=false;
	}
	return RetVal;
}

int OciQuery::GetASMFileInfo(ASMFile *pASMFile,char *mystr)
{
	int RetVal = QUERY_OK;
	if(mystr == NULL)
		return ERROR_NULL;

	if(ConnectOCI()){
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,SELECT_FILEINFO,mystr);
#ifdef _DEBUG_QUERY_ON
		cout<<QuerySql<<endl;
#endif
		if(m_OciApi.ExecSQL(QuerySql,&GetNumDate,(void *)pASMFile,4)){
			RetVal = QUERY_ERROR;
		}
	}else{
		RetVal = CONNECT_ERROR;
	}
	return RetVal;
}

int OciQuery::GetASMFileInfo2(ASMFile *pASMFile,char *mystr,int Groupnum)
{
	int RetVal = QUERY_OK;
	if(mystr == NULL)
		return ERROR_NULL;

	if(ConnectOCI()){
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,SELECT_FILEINFO2);
#ifdef _DEBUG_QUERY_ON
		cout<<QuerySql<<endl;
#endif
		int DataNum = 2; // modify 141204 
		BindDatas myDatas(DataNum);
		myDatas[0].DataStr = mystr;
		myDatas[0].Type = SQLT_STR;
		myDatas[1].DataInt = Groupnum;
		myDatas[1].Type = SQLT_INT;
		if(m_OciApi.ExecSQL(QuerySql,&GetNumDate,(void *)pASMFile,4,myDatas,DataNum)){
			RetVal = QUERY_ERROR;
		}
	}else{
		RetVal = CONNECT_ERROR;
	}
	return RetVal;
}

int OciQuery::GetAllGroupInfo(VGroups *pGroup)
{
	int RetVal = QUERY_OK;
	if(ConnectOCI()){
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,SELECT_ALL_GROUP);
#ifdef _DEBUG_QUERY_ON
		cout<<QuerySql<<endl;
#endif
		if(m_OciApi.ExecSQL(QuerySql,&GetAllGroups,(void *)pGroup,4)){
			RetVal = QUERY_ERROR;
		}
	}else{
		RetVal = CONNECT_ERROR;
	}
	return RetVal;
}

int OciQuery::GetGroupInfo(Group *pGroup,char *mystr)
{
	int RetVal = QUERY_OK;
	if(mystr == NULL)
		return ERROR_NULL;
	if(ConnectOCI()){
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,SELECT_GROUP_NUM,mystr);
#ifdef _DEBUG_QUERY_ON
		cout<<QuerySql<<endl;
#endif
		if(m_OciApi.ExecSQL(QuerySql,&GetGroup,(void *)pGroup,3)){
			RetVal = QUERY_ERROR;
		}
	}else{
		RetVal = CONNECT_ERROR;
	}
	return RetVal;
}

int OciQuery::GetAllASMDisks(VVDisks *pDisks,int GroupSize)
{
	int RetVal = QUERY_OK;
	if(pDisks){
		pDisks->resize(GroupSize+1);// 比最大GroupSize大1，第一个容器保留,这样方便索引
	}else{
		return ERROR_NULL;
	}

	if(ConnectOCI()){
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,SELECT_ALL_DISKS);
#ifdef _DEBUG_QUERY_ON
		cout<<QuerySql<<endl;
#endif
		if(m_OciApi.ExecSQL(QuerySql,&GetAllDisks,(void *)pDisks,3)){
			RetVal = QUERY_ERROR;
		}
	}else{
		RetVal = CONNECT_ERROR;
	}
	return RetVal;
}


int OciQuery::GetAUInfo2(ASMFile *pASMFile,BindDatas &Datas,int DataNum)
{
	int RetVal = QUERY_OK;
	if(ConnectOCI()){
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,SELECT_AU_INFO2);
#ifdef _DEBUG_QUERY_ON
		cout<<QuerySql<<endl;
#endif		
		if(m_OciApi.ExecSQL(QuerySql,&GetSource,(void *)pASMFile,2,Datas,DataNum)){
			RetVal = QUERY_ERROR;
		}
	}else{
		RetVal = CONNECT_ERROR;
	}
	return QUERY_OK;
}
int OciQuery::GetASMDisks(VDisks *pDisks,int GroupNum)
{
	bool RetVal = QUERY_OK;
	if(ConnectOCI()){
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,SELECT_DISK_INFO,GroupNum);
#ifdef _DEBUG_QUERY_ON
	cout<<QuerySql<<endl;
#endif	
		if(m_OciApi.ExecSQL(QuerySql,&GetDiskInfo,(void *)pDisks,3)){
			RetVal = QUERY_ERROR;
		}
		RetVal = QUERY_ERROR;
	}else{
		RetVal = CONNECT_ERROR;
	}
	return RetVal;
}

int OciQuery::GetDatafilePath(char *mystr,int Index)
{
	string tempstr;
	int ret = GetDatafilePath(tempstr,Index);
	if(ret == QUERY_OK)
		strcpy(mystr,tempstr.c_str());
	return ret;
}

int OciQuery::GetDatafilePath(string &mystr,int Index)
{
	if(Index < 0)
		return ERROR_NON_FILEINDEX;
	char ComStr[25];
	stringstream sstr;
	sstr<<Index;
	sstr>>ComStr;

	if(ConnectOCI() == false){
		return CONNECT_ERROR;
	}

	memset(QuerySql,0,sizeof(QuerySql));
	sprintf(QuerySql,SELECT_DATAFILE,Index);
#ifdef _DEBUG_QUERY_ON
	cout<<QuerySql<<endl;
#endif	
	if(m_OciApi.ExecSQL1(QuerySql,&GetDataFile,(void *)&mystr)){
		return QUERY_ERROR;
	}
	return QUERY_OK;
}

bool OciQuery::GetAllDatafileInfo(Datafiles *datafiles,BYTE8 &MaxCreationChange)
{
	bool RetVal = QUERY_ERROR;
	if(ConnectOCI()){
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,SELECT_DATAFILE_INFO);

	

#ifdef _DEBUG_QUERY_ON
	cout<<QuerySql<<endl;
#endif	
		if(m_OciApi.ExecSQL(QuerySql,&QueryDatafileInfo,(void *)datafiles,6)){
			RetVal = QUERY_ERROR;
		}else{
			RetVal = QUERY_OK;
		}		
	}else{
		RetVal = false;
	}

	int Num = datafiles->size();
	for (int i=0;i<Num;i++){
		if (MaxCreationChange < datafiles->at(i).CreationChange){
			MaxCreationChange =datafiles->at(i).CreationChange;
		}		
	}
	return RetVal;
}

// 备库执行
// 每次处理一个文件 
// 返回值：文件序号 如果是0，则没有找到该文件，添加操作继续
WORD OciQuery::GetNewDatafileInfo(int FileIndex,char *filename)
{
	WORD RetVal = 0;
	if(ConnectOCI()){
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,SELECT_NEW_DATAFILE,FileIndex);
#ifdef _DEBUG_QUERY_ON
	cout<<QuerySql<<endl;
#endif
		if(0 == m_OciApi.ExecSQL1(QuerySql,&GetDataChars,(void *)filename)){
			RetVal = FileIndex;
		}
	}
	else{
		RetVal = CONNECT_ERROR;
	}
	return RetVal;
}

BYTE8 OciQuery::GetExpandFileSize(int FileIndex)
{
	BYTE8 RetVal = 0;
	if(ConnectOCI()){
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,GETFILESIZE,FileIndex);
		char NewDatafile[64] = {0};
		if(0 == m_OciApi.ExecSQL1(QuerySql,&GetDataChars,(void *)NewDatafile)){
			RetVal = Str2BYTE8(NewDatafile);
		}
	}
	else{
		RetVal = 0;
	}
	return RetVal;
}

void OciQuery::GetErrorMsg(char *msg)
{
	strcpy(msg,m_OciApi.ErrorMsg);
}

bool OciQuery::InsertExeSQL(char *mystr)
{
	if(bConnected == false){
		if(ConnectOCI()){
			bConnected = true;
		}else{
			return false;
		}
	}
#ifdef _DEBUG_QUERY_ON
	cout<<mystr<<endl;
#endif
	if(0 == m_OciApi.InsertExecSQL(mystr))
		return true;
	return false;
}
// 这个用来测试连接是否正常
bool OciQuery::TestConnect()
{
	bool ret = false;
	if(0 == m_OciApi.SelectNull(SLELCT_DAUL))
		ret = true;
#ifdef _DEBUG_QUERY_ON
	cout<<"TestConnect:\t"<<ret<<endl;
	printf("m_OciApi:\t%d\n",&m_OciApi);
#endif
	return ret;
}

bool OciQuery::FlashOracleCache()
{
	bool RetVal = false;
	if(ConnectOCI()){
		if((0 == m_OciApi.InsertExecSQL(FLASH_CACHE)) && (0 == m_OciApi.InsertExecSQL(FLASH_SHARED_POOL)))
			RetVal = true;
	}else{
		RetVal = CONNECT_ERROR;
	}
	return RetVal;
}

void OciQuery::close()
{
	m_OciApi.Close();
	bConnected = false;
}

char *OciQuery::GetLastQuery()
{
	return QuerySql;
}

int OciQuery::GetAllDataFileName(string *names,char *path)
{
	if(ConnectOCI() == false){
		return CONNECT_ERROR;
	}
	sprintf(QuerySql,SELECT_DATAFILE_ALL);
#ifdef _DEBUG_QUERY_ON
	cout<<QuerySql<<endl;
#endif
	m_OciApi.ExecSQL1(QuerySql,&GetDataFileALL,(void *)names);
	return QUERY_OK;
}

int OciQuery::GetAllDataFileName(char *names)
{
	return GetAllDataFileName((string*)names,NULL);
}

bool OciQuery::GetNewControlFile(char *ControlFile)
{
	bool RetVal  = false;
	if(ConnectOCI()){
		if((0 == m_OciApi.InsertExecSQL(GET_CONTROL_FILE))){
			RetVal = true;
			char *p=getenv("ORACLE_HOME");
			if (p)
			{
#ifdef WIN32
				sprintf(ControlFile, "%s\\database\\control01.ctl", p);
#else
				sprintf(ControlFile, "%s/dbs/control01.ctl", p);
#endif
			}

		}
	}else{
		RetVal = CONNECT_ERROR;
	}
	return RetVal;
}

int OciQuery::GetOracleState()
{
	int RetVal = STATE_UNKNOW;
	int ConRet = 0;
	if( ConnectOCI() ){
		memset(QuerySql,0,sizeof(QuerySql));
		strcpy(QuerySql,SELECT_GETSTAT);
#ifdef _DEBUG_QUERY_ON
	cout<<QuerySql<<endl;
#endif
		if(m_OciApi.ExecSQL(QuerySql,&GetStateDate,(void *)&RetVal,2)){
			RetVal = STATE_UNKNOW;
		}
	} else {
#ifdef _DEBUG_QUERY_ON
	cout<<"ConRet"<<ConRet<<endl;
#endif
		if(ConRet != ERROR_Password){
			RetVal = ORACLE_SHUTDOWN;	
		}
	}
	return RetVal;
}

bool OciQuery::GetSyncState(int &MaxSeq,bool &Appled)
{
	bool RetVal = true;
	if(ConnectOCI()){
		memset(QuerySql,0,sizeof(QuerySql));
		strcpy(QuerySql,SELECT_SYNC_STAT);
		long Lstat = 0;
		if(m_OciApi.ExecSQL(QuerySql,&GetSyncStateDate,(void *)&Lstat,2)){
			RetVal = false;
		}
		if(Lstat > 0)
		{
			MaxSeq = Lstat;
			Appled = true;
		}else{
			MaxSeq = Lstat*(-1);
			Appled = false;
		}
	} else{
		RetVal=false;
	}
	return RetVal;
}

BYTE8 OciQuery::GetAnalyseSCN()
{
	RedologINI ConfigINI;
	ConfigINI.LoadFile(_CONFIG_FILE);
	char strSCN[64] = {0};
	BYTE8 LastSCN = ConfigINI.GetBYTE8Value("LOG","SCN");

	memset(strSCN,0,sizeof(strSCN));

	BYTE8 RetVal = 0;
	if(ConnectOCI()){	
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,SELECT_ANALYSE_SCN,LastSCN);
		if(m_OciApi.ExecSQL1(QuerySql,&GetDataChars,(void *)strSCN) == 0){
			RetVal = Str2BYTE8(strSCN);
		}
	}else{
		RetVal = 0;
	}
	return RetVal;
}

bool OciQuery::AlterDatabaseOpen()
{
	bool RetVal = false;
	if(ConnectOCI()){
		if((0 == m_OciApi.InsertExecSQL(ALTER_DATABASE_OPEN))){
				RetVal = true;
		}
	}else{
		RetVal = false;
	}
	return RetVal;
}

bool OciQuery::CreateDatafile(int FileINdex,const char *FIleName)
{
	bool RetVal = false;
	if(ConnectOCI()){
		memset(QuerySql,0,sizeof(QuerySql));

// 设置参数standby_file_management= manual
		m_OciApi.InsertExecSQL(STANDBYFILE_MANUAL);	
		sprintf(QuerySql,CREATE_DATAFILE,FileINdex,FIleName);	
		if((0 == m_OciApi.InsertExecSQL(QuerySql))){
			RetVal = true;
		}
		m_OciApi.InsertExecSQL(STANDBYFILE_AUTO);

	}else{
		RetVal = false;
	}
	return RetVal;
}

bool OciQuery::ShutdownImmediate()
{
	char cmd[MAX_PATH] = {0};
	char szBuf[MAX_PATH];
	FILE   *pPipe;

#if defined(HAVE_WIN32)
	if (GetCurrentDirectory(MAX_PATH,szBuf) <= 0){
		sprintf(szBuf,"C:\\Program Files\\NETPRO5.02.8\\NETPRO统一数据管理平台客户端");
	}

	sprintf(cmd,"\"%s\\scripts\\OracleScript\\ShutdownOracle.bat\" %s %s %s",szBuf,USER,PASSWD,SID);
#else
	sprintf(cmd,"su - oracle -c \"/usr/NETPRO5.02.8/scripts/ShutdownOracle.sh %s %s %s \" ",USER,PASSWD,SID);

#ifdef _DEUBG_SCRIPT
	cout<<cmd<<endl;
#endif
	if((pPipe = popen(cmd, "r" )) == NULL )
		 return false;
	char *pChar = NULL;
	while(true){
		pChar = fgets(szBuf, 128, pPipe);
		if(pChar == NULL){
			if(errno == EINTR){		// popen执行脚本时，会有系统信号EINTR,可以屏蔽此信号，或者做相关处理
				errno = 0;
				continue;
			}
			else
				break;
		}
		printf(szBuf);
	}
	int RetVal = -1;
	if (feof(pPipe)){
		RetVal = pclose(pPipe);
	}

#endif

	if (RetVal == 0) {
		return true;
	}
#ifdef _DEUBG_SCRIPT
	cout<<"RetVal:\t"<<RetVal<<endl;
	cout<<"errno:\t"<<errno<<endl;
#endif
	return false;
}

bool OciQuery::StartupMount()
{
	char cmd[MAX_PATH] = {0};
	char szBuf[MAX_PATH];
	FILE   *pPipe;

#if defined(HAVE_WIN32)
	if (GetCurrentDirectory(MAX_PATH,szBuf) <= 0){
		sprintf(szBuf,"C:\\Program Files\\NETPRO5.02.8\\NETPRO统一数据管理平台客户端");
	}

	sprintf(cmd,"\"%s\\scripts\\OracleScript\\StartupMount.bat\" %s %s %s",szBuf,USER,PASSWD,SID);
#else	
	sprintf(cmd,"su - oracle -c /usr/NETPRO5.02.8/scripts/StartupMount.sh ");

#ifdef _DEUBG_SCRIPT
	cout<<cmd<<endl;
#endif

	if((pPipe = popen(cmd, "r" )) == NULL )
		 return false;
	char *pChar = NULL;
	while(true){
		pChar = fgets(szBuf, 128, pPipe);
		if(pChar == NULL){
			if(errno == EINTR){		// popen执行脚本时，会有系统信号EINTR,可以屏蔽此信号，或者做相关处理
				errno = 0;
				continue;
			}
			else
				break;
		}
		printf(szBuf);
	}
	int RetVal = -1;
	if (feof(pPipe)){
		RetVal = pclose(pPipe);
	}
#endif
#ifdef _DEUBG_SCRIPT
	cout<<"StartupMount ...END"<<endl;
#endif
	if (RetVal == 0) {
		return true;
	}
	return false;
}

bool OciQuery::Startup()
{
	char cmd[MAX_PATH] = {0};
	char szBuf[MAX_PATH];
	FILE   *pPipe;

#if defined(HAVE_WIN32)
	if (GetCurrentDirectory(MAX_PATH,szBuf) <= 0){
		sprintf(szBuf,"C:\\Program Files\\NETPRO5.02.8\\NETPRO统一数据管理平台客户端");
	}

	sprintf(cmd,"\"%s\\scripts\\OracleScript\\StartupMount.bat\" %s %s %s",szBuf,USER,PASSWD,SID);
#else	
	sprintf(cmd,"su - oracle -c /usr/NETPRO5.02.8/scripts/Startup.sh");

#ifdef _DEUBG_SCRIPT
	cout<<"-----"<<cmd<<endl;
#endif

	if((pPipe = popen(cmd, "r" )) == NULL )
		 return false;
	char *pChar = NULL;
	while(true){
		pChar = fgets(szBuf, 128, pPipe);
		if(pChar == NULL){
			if(errno == EINTR){		// popen执行脚本时，会有系统信号EINTR,可以屏蔽此信号，或者做相关处理
				errno = 0;
				continue;
			}
			else
				break;
		}
		printf(szBuf);
	}
	int RetVal = -1;
	if (feof(pPipe)){
		RetVal = pclose(pPipe);
	}
#endif
	if (RetVal == 0) {
		return true;
	}
	return false;
}

/*
bool OciQuery::RecoverDatabase(char *sUser,char *sPasswd,char *sSID,char *Rac2SID)
{
	char cmd[MAX_PATH] = {0};
	char szBuf[MAX_PATH];
	FILE   *pPipe;

#ifdef		ORACLE_RAC
	if(Rac2SID){
		sprintf(cmd,"su - oracle -c \"/usr/NETPRO5.02.8/scripts/RecoverDatabaseRAC.sh %s %s %s %s \" ",sUser,sPasswd,sSID,Rac2SID);
	}else{
		sprintf(cmd,"su - oracle -c \"/usr/NETPRO5.02.8/scripts/RecoverDatabaseRAC.sh %s %s %s \" ",sUser,sPasswd,sSID);
	}
#else
	if(Rac2SID){
		sprintf(cmd,"su - oracle -c \"/usr/NETPRO5.02.8/scripts/RecoverDatabase.sh %s %s %s one \" ",sUser,sPasswd,sSID);	//归档一次
	}else{
		sprintf(cmd,"su - oracle -c \"/usr/NETPRO5.02.8/scripts/RecoverDatabase.sh %s %s %s \" ",sUser,sPasswd,sSID);
	}
#endif

#ifdef _DEUBG_SCRIPT
	cout<<"-----"<<cmd<<endl;
#endif
	t_debugMsg(t_timingRecLvl,"cmd=%s\n",cmd);

	if((pPipe = popen(cmd, "r" )) == NULL ){
		t_errMsg("popen failed: %s\n",cmd);
		 return false;
	}
	char *pChar = NULL;
	while(true){
		pChar = fgets(szBuf, 128, pPipe);
		if(pChar == NULL){
			if(errno == EINTR){		// popen执行脚本时，会有系统信号EINTR,可以屏蔽此信号，或者做相关处理
				errno = 0;
				continue;
			}
			else
				break;
		}
		t_debugMsg(t_timingRecLvl,szBuf);
	}
	int RetVal = -1;
	if (feof(pPipe)){
		RetVal = pclose(pPipe);
	}
#ifdef _DEUBG_SCRIPT
	cout<<"RecoverDatabase-----END"<<endl;
#endif

	if (RetVal == 0) {
		return true;
	}

	t_errMsg("OciQuery::RecoverDatabase failed\n");
	return false;
}
*/

bool OciQuery::RecoverDatabase(char *sUser,char *sPasswd,char *sSID,char *Rac2SID)
{
	char cmd[MAX_PATH] = {0};
	char szBuf[MAX_PATH];
	FILE   *pPipe;
	const int maxRecTimes=5;
	int i;

#ifdef		ORACLE_RAC
	if(Rac2SID){
		sprintf(cmd,"su - oracle -c \"/usr/NETPRO5.02.8/scripts/RecoverDatabaseRAC.sh %s %s %s %s \" ",sUser,sPasswd,sSID,Rac2SID);
	}else{
		sprintf(cmd,"su - oracle -c \"/usr/NETPRO5.02.8/scripts/RecoverDatabaseRAC.sh %s %s %s \" ",sUser,sPasswd,sSID);
	}
#else
	if(Rac2SID){
		sprintf(cmd,"su - oracle -c \"/usr/NETPRO5.02.8/scripts/RecoverDatabase.sh %s %s %s one \" ",sUser,sPasswd,sSID);	//归档一次
	}else{
		sprintf(cmd,"su - oracle -c \"/usr/NETPRO5.02.8/scripts/RecoverDatabase.sh %s %s %s \" ",sUser,sPasswd,sSID);
	}
#endif

#ifdef _DEUBG_SCRIPT
	cout<<"-----"<<cmd<<endl;
#endif
	t_debugMsg(t_timingRecLvl,"cmd=%s\n",cmd);

	for(i=0;i<maxRecTimes;i++){
		if((pPipe = popen(cmd, "r" )) == NULL ){
			t_errMsg("popen failed: %s\n",cmd);
			return false;
		}
		char *pChar = NULL;
		while(true){
			pChar = fgets(szBuf, 128, pPipe);
			if(pChar == NULL){
				if(errno == EINTR){		// popen执行脚本时，会有系统信号EINTR,可以屏蔽此信号，或者做相关处理
					errno = 0;
					continue;
				}
				else
					break;
			}
			t_debugMsg(t_timingRecLvl,szBuf);
		}
		int RetVal = -1;
		if (feof(pPipe)){
			RetVal = pclose(pPipe);
		}
#ifdef _DEUBG_SCRIPT
	cout<<"RecoverDatabase-----END"<<endl;
#endif

		if (RetVal == 0) {
			return true;
		}
	}

	t_errMsg("OciQuery::RecoverDatabase failed\n");
	return false;
}


BYTE8 OciQuery::GetNextChange() //得到最新的SCN号，因为控制文件是最新的
{
	RedologINI ConfigINI;
	ConfigINI.LoadFile(_CONFIG_FILE);
	char strSCN[64] = {0};
	ConfigINI.LoadValuse("LOG","SCN",strSCN);

	char strNextChange[64] = {0};
	BYTE8 RetVal = 0;
	if(ConnectOCI()){
		if((0 == m_OciApi.InsertExecSQL(ALTER_LOG_DEFER))){
			if((0 == m_OciApi.InsertExecSQL(ALTER_LOG_DEFAULT))){
				//这里还要切一次归档
				m_OciApi.InsertExecSQL(ALTER_SWITCH_LOGFILE);
				memset(QuerySql,0,sizeof(QuerySql));
				sprintf(QuerySql,SELECT_NEXT_CHANGE,strSCN);
				if(m_OciApi.ExecSQL1(QuerySql,&GetDataChars,(void *)strNextChange) == 0){
					RetVal = Str2BYTE8(strNextChange);
				}
			}
		}
	}else{
		RetVal = CONNECT_ERROR;
	}
	return RetVal;
}

bool OciQuery::RenameDatafile(char *StrPath)
{
	char cmd[MAX_PATH] = {0};
	char szBuf[MAX_PATH];
	FILE   *pPipe;	
	sprintf(cmd,"su - oracle -c \"/usr/NETPRO5.02.8/scripts/RenameDatafile.sh %s\"",StrPath);

#ifdef _DEUBG_SCRIPT
	cout<<"-----"<<cmd<<endl;
#endif

	if((pPipe = popen(cmd, "r" )) == NULL )
		 return false;
	char *pChar = NULL;
	while(true){
		pChar = fgets(szBuf, 128, pPipe);
		if(pChar == NULL){
			if(errno == EINTR){		// popen执行脚本时，会有系统信号EINTR,可以屏蔽此信号，或者做相关处理
				errno = 0;
				continue;
			}
			else
				break;
		}
		printf(szBuf);
	}
	int RetVal = -1;
	if (feof(pPipe)){
		RetVal = pclose(pPipe);
	}
	if (RetVal == 0) {
		return true;
	}
	return false;
}

bool OciQuery::GetNewLogfile()
{
	bool RetVal = false;
	if(ConnectOCI()){		//这里还要切一次归档
#ifdef _DEBUG_QUERY_ON
		cout<<"GetNewLogfile Connect OK"<<endl;
#endif
		if(0 == m_OciApi.InsertExecSQL(ALTER_SWITCH_LOGFILE)){
			if(0 == m_OciApi.InsertExecSQL(ALTER_SWITCH_CHECKPOINT)){
				RetVal = true;
			}
		}
	}

	else{
		RetVal = false;
#ifdef _DEBUG_QUERY_ON	
		cout<<"GetNewLogfile Connect ERROR"<<endl;
		cout<<USER<<PASSWD<<SID<<endl;
#endif
	}
	return RetVal;
}


int cb_getTableMetaInfo(void *pList,const char *pVal_,char* FileNum_)
{
	bool bFuncRet = true;
	int iRet = 0;
	int i = 0;
	
	table_meta tableMeta;
	column_meta colMeta;
	LGC_TblsMeta_Mgr* pTblsMetaMgr = (LGC_TblsMeta_Mgr*)pList;

	char* pNext = NULL;
	char* pPrev = NULL;
	
	char* pVal = NULL;
	int FileNum = *(int*)FileNum_;
	
	pVal = new char[strlen(pVal_)+1];
	if(!pVal){
		lgc_errMsg("new failed\n");
		exit(1);
	}
	strcpy(pVal,pVal_);
	
	if(FileNum != 8){
		iRet = -1;
		goto errOut;
	}

	pPrev = pVal;
	for(i=0; i<FileNum; i++){
		pNext = strchr(pPrev,'\n');
		if(!pNext){
			lgc_errMsg("value invalid \n");
			iRet = -1;
			goto errOut;
		}
		*pNext = '\0';
		
		switch(i){
		case 0:
			sscanf(pPrev, "%u", &tableMeta.objNum);
			break;
		case 1:
			strcpy(tableMeta.owner,pPrev);
			break;
		case 2:
			strcpy(tableMeta.tableName,pPrev);
			break;
//		case 3:
//			sscanf(pPrev, "%u", &tableMeta.cols);
//			break;
		case 3:
			colMeta.objNum = tableMeta.objNum;
			sscanf(pPrev, "%hu", &colMeta.colNo);
			break;
		case 4:
			strcpy(colMeta.colName, pPrev);
			break;
		case 5:
			strcpy(colMeta.dataType, pPrev);
			break;
		case 6:
			sscanf(pPrev, "%u", &colMeta.dataMaxLen);
			break;
		case 7: 
			if(pPrev[0] == 'Y'){
				colMeta.nullAble = true;
			}else{
				colMeta.nullAble = false;
			}
			
			break;
		default:
			lgc_errMsg("cb_getTableMetaInfo FieldNum invalid: %u\n",FileNum);
			iRet = -1;
			goto errOut;
			break;
		}

		pPrev = pNext +1;
	}//end for
	
	if(tableMeta.objNum <= 0 || colMeta.colNo <= 0 || colMeta.dataMaxLen < 0){
		lgc_errMsg("tableMeta of query invalid\n");
		iRet = -1;
		goto errOut;
	}

	if(colMeta.colNo == 1){
		bFuncRet = pTblsMetaMgr->setTableMetaInfo(&tableMeta);
		if(!bFuncRet){
			lgc_errMsg("pTblsMetaMgr->setTableMetaInfo failed\n");
			iRet = -1;
			goto errOut;
		}
	}

	bFuncRet = pTblsMetaMgr->setColMetaInfo(tableMeta.objNum, &colMeta);
	if(!bFuncRet){
		lgc_errMsg("pTblsMetaMgr->setColMetaInfo failed\n");
		iRet = -1;
		goto errOut;
	}

errOut:
	if(pVal){
		delete[] pVal;               
	}
	return iRet;
}

bool OciQuery::getTableMetaInfo(LGC_TblsMeta_Mgr* pTablesMetaMgr)
{
	bool bRet = true;
	if(ConnectOCI()){	
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,SELECT_TABLEMETA_INFO);

		char strTemp[512] = {0};
		if(m_OciApi.ExecSQL_tw(QuerySql,&cb_getTableMetaInfo,(void *)pTablesMetaMgr,9)){
			bRet = false;
			goto errOut;
		}
	}
	else{
		bRet=false;
		goto errOut;
	}

errOut:
	return bRet;
}

bool OciQuery::getTableMetaInfo(LGC_TblsMeta_Mgr* pTablesMetaMgr,const char* ownerName)
{
	bool bRet = true;
	if(ConnectOCI()){	
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,SELECT_TABLEMETA_INFO_BYOWNER, ownerName);

		char strTemp[512] = {0};
		if(m_OciApi.ExecSQL_tw(QuerySql,&cb_getTableMetaInfo,(void *)pTablesMetaMgr,8)){
			bRet = false;
			goto errOut;
		}
	}
	else{
		bRet=false;
		goto errOut;
	}

errOut:
	return bRet;
}

bool OciQuery::getTableMetaInfo(LGC_TblsMeta_Mgr* pTablesMetaMgr, const char* owner, const char* tableName)
{
	bool bRet = true;
	if(ConnectOCI()){
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,SELECT_TABLEMETA_INFO_BYOWNERTABLENAME, owner, tableName);
		char strTemp[512] = {0};
		if(m_OciApi.ExecSQL_tw(QuerySql,&cb_getTableMetaInfo,(void*)pTablesMetaMgr,8)){
			bRet = false;
			goto errOut;
		}
	}else{
		bRet = false;
		goto errOut;
	}
errOut:
	return bRet;
}
int cb_getTableCols(void *pList,const char *pVal_,char* FileNum_)
{
	int iRet = 0;
	
	WORD* pCols = (WORD*)pList;
	char* pVal = (char*)pVal_;
	char* p = NULL;

	p = strchr(pVal,'\n');
	if(!p){
		iRet = -1;
		goto errOut;
	}
	*p = '\0';

	sscanf(pVal,"%hu",pCols);

	if(&pCols <=0 ){
		lgc_errMsg("col count fo the table is invalid \n");
		iRet = -1;
		goto errOut;
	}
 
	iRet = QEURY_END_DATA;

errOut:
	return iRet;
}

bool OciQuery::getTableCols(const char* ownerName, const char* tableName, WORD* pCols)
{
	
	bool bRet = true;
	
	if(ConnectOCI()){	
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,SELECT_TABLE_COLS, ownerName,tableName);

		char strTemp[512] = {0};
		if(m_OciApi.ExecSQL_tw(QuerySql,&cb_getTableCols,(void *)pCols,1)){
			bRet = false;
			goto errOut;
		}
	}
	else{
		bRet=false;
		goto errOut;
	}

errOut:
	return bRet;
}


bool OciQuery::lgc_getArchFileList(LGC_RedoFileInfoList* pList, WORD threadId, BYTE8 startSCN,int (*callBackFunc)(void*,const char*,char*))
{
	bool bRet = true;
	if(ConnectOCI()){
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,SELECT_ARCHIVE_FILE_LIST,threadId,startSCN,startSCN,startSCN);
		char strTemp[512] = {0};
		if(m_OciApi.ExecSQL_tw(QuerySql,callBackFunc,(void*)pList,7)){
			bRet = false;
			goto errOut;
		}
	}else{//connect failed
		bRet=false;
		goto errOut;
	}
errOut:
	return bRet;
}

bool OciQuery::lgc_getArchFileListBySeq(LGC_RedoFileInfoList* pList, WORD threadId, DWORD sequence, int (*callBackFunc)(void*,const char*,char*))
{
	bool bRet = true;
	if(ConnectOCI()){
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,LGC_SELECT_ARCHIVE_FILE_LIST_BYSEQ,threadId,sequence);
		if(m_OciApi.ExecSQL_tw(QuerySql,callBackFunc,(void*)pList,7)){
			bRet = false;
			goto errOut;
		}
	}else{
		bRet = false;
		goto errOut;
	}
errOut:
	return bRet;
}

bool OciQuery::lgc_getOnlineFileList(LGC_RedoFileInfoList* pList, WORD threadId, BYTE8 startSCN,int (*callBackFunc)(void*,const char*,char*))
{
	bool bRet = true;
	if(ConnectOCI()){
		memset(QuerySql, 0, sizeof(QuerySql));
		sprintf(QuerySql,SELECT_ONLINE_FILE_LIST,threadId,startSCN,startSCN,startSCN);
		char strTemp[512] = {0};
		if(m_OciApi.ExecSQL_tw(QuerySql,callBackFunc,(void*)pList,9)){
			lgc_errMsg("lgc_getOnlineFileList failed:sql=%s\n", QuerySql);
			bRet = false;
			goto errOut;
		}
	}else{//connect failed
		bRet = false;
		goto errOut;
	}

errOut:
	return bRet;
}

bool OciQuery::lgc_getOnlineFileListBySeq(LGC_RedoFileInfoList* pList, WORD threadId, DWORD sequence,int (*callBackFunc)(void*,const char*,char*))
{
	bool bRet = true;
	if(ConnectOCI()){
		memset(QuerySql, 0, sizeof(QuerySql));
		sprintf(QuerySql,LGC_SELECT_ONLINE_FILE_LIST_BYSEQ,threadId,sequence);
		char strTemp[512] = {0};
		if(m_OciApi.ExecSQL_tw(QuerySql,callBackFunc,(void*)pList,9)){
			bRet = false;
			goto errOut;
		}
	}else{//connect failed
		bRet = false;
		goto errOut;
	}

errOut:
	return bRet;
}


bool OciQuery::lgc_getArchFileInfo(LGC_RedoFileInfo* pArchFileInfo, WORD threadId, DWORD sequence, int (*callBackFunc)(void*,const char*,char*))
{
	bool bRet = true;
	if(ConnectOCI()){
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql, LGC_SELECT_ARCH_FILE_INFO,threadId,sequence);
		char strTemp[512] = {0};
		if(m_OciApi.ExecSQL_tw(QuerySql, callBackFunc,(void*)pArchFileInfo,7)){
			bRet = false;
			goto errOut;
		}
	}else{
		bRet = false;
		goto errOut;
	}
	
errOut:
	return bRet;
}

bool OciQuery::lgc_getOnlineFileInfo(LGC_RedoFileInfo* pOnlineFileInfo, WORD threadId, DWORD sequence, int (*callBackFunc)(void*,const char*,char*))
{
	bool bRet = true;
	if(ConnectOCI()){
		memset(QuerySql, 0, sizeof(QuerySql));
		sprintf(QuerySql, LGC_SELECT_ONLINE_FILE_INFO, threadId,sequence);
		char strTemp[512] = {0};
		if(m_OciApi.ExecSQL_tw(QuerySql,callBackFunc, (void*)pOnlineFileInfo, 9)){
			bRet = false;
			goto errOut;
		}
	}else{
		bRet = false;
		goto errOut;
	}
errOut:
	return bRet;
}

bool OciQuery::lgc_isArchivedOfRedoFile(bool* pIsArchived, WORD threadId, DWORD sequence)
{
	bool bRet = true;
	char isArchived_str[126] = {0};
	*pIsArchived = false;
	if(ConnectOCI()){	
		memset(QuerySql,0,sizeof(QuerySql));
		sprintf(QuerySql,LGC_Is_ARChIVED,threadId, sequence);
		if(m_OciApi.ExecSQL_tw(QuerySql,&GetDataChars,(void *)isArchived_str,1)){
			bRet = false;
			goto errOut;
		}
		if(strncmp(isArchived_str,"YES",strlen("YES")) == 0){
			*pIsArchived = true;
		}else{
			*pIsArchived = false;
		}
	}else{
		bRet = false;
		goto errOut;
	}

errOut:
	return bRet;
}

