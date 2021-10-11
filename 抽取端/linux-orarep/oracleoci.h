//////////////////////////////////////////////////////////////////////
//**************************************************************
//*	文件名	oracleoci.h
//* 设 计 者：wqy	日期：2012-5-15:16
//* 修 改 者：		日期：2012-5-15:16
//* 版 本：
//*
//****************************************************************

#ifndef  _ORACLEOCI_H
#define  _ORACLEOCI_H

#ifdef WIN32 
#include <WTypes.h> // Include this first on Win (bug #35683) 
#endif

#include <cstdio>
//#include <cstdlib>
//#include <string>
//#include <strings.h>
//#include <unistd.h>
//#include <malloc.h>
#include <errno.h>
#include "oci.h"
#include "oratypes.h"
#include "ocidfn.h"
#include "ocidem.h"
#include <string>
#include <vector>
using namespace std;

enum{
	ERROR_OCIInitialize =	-1,
	ERROR_OCIEnvInit	=	-2,
	ERROR_OCIHandleAlloc1 =	-3,
	ERROR_OCIHandleAlloc2 =	-4,
	ERROR_OCIServerAttach =	-5,
	ERROR_OCIHandleAlloc3 =	-6,
	ERROR_OCISetHandle	  =	-7,
	ERROR_OCIHandleAlloc4 =	-8,
	ERROR_UserName		  =	-9,
	ERROR_Password		  =	-10,
	ERROR_OCISessionBegin =	-11,
	ERROR_OCISetSession	  =	-12,
	ERROR_OCIHandleAlloc  =	-13,	
};

//ORA-01403 数据表中查不到符合条件的记录
//ORA-01405 查到记录，但值为空
enum{
	QUERY_ERROR_NO_DATA = 1403,	
	QUERY_ERROR_NULL_DATA = 1405,	
};

enum{
	QEURY_MORE_DATA	= 0,
	QEURY_END_DATA	= 1,
	QEURY_ERROR_DATA	= -1,
};
struct OCIBindData
{
	sb4		DataInt;
	sb8		DataLong;
	char	*DataStr;
	int		Type;
	OCIBindData()
	{
		DataInt = 0;
		DataLong = 0;
		DataStr = NULL;
		Type = 0;	// OCI类型是从1开始的，见ocidfn.h
	}
};
typedef vector<OCIBindData> BindDatas;
#define SQLT_LNG_STR_LEN	64

// -1	处理出错
// 0	正常
// 1	查到数据
typedef int (*HandleFun)(void *pList,const char *pVal,char* FileNum);
#define BUFSIZE 512

class oracleoci
{
private:
	OCIEnv		*pEnv;
	OCIError	*pError;
	OCISvcCtx	*pService;
	OCIServer	*pServer;
	OCISession	*pSession;
	OCIStmt		*pStmt;
	OCIDefine	*pDefn;	// = (OCIDefine *)0;
	int	 	    conn;	//=0; //0:未连接 1：已连接*/
public:
	oracleoci();
	~oracleoci();

	int	Close();
	int Connect(char *User,char *Pwd,char *SID);
	int ExecSQL(char *sqlStr);
	int ExecSQL1(char *sqlStr,HandleFun HFun, void *plist);
	int ExecSQL2(char *sqlStr,HandleFun HFun, void *plist,void *CompareVal = NULL);
	int	ExecSQL(char *sqlStr,HandleFun HFun, void *plist,int nSelectNum);
	int	ExecSQL_tw(char *sqlStr,HandleFun HFun, void *plist,int nSelectNum);
	int InsertExecSQL(char *sqlStr);
	int SelectNull(char *sqlStr);
	
	int	ExecSQL(char *sqlStr,HandleFun HFun, void *plist,int nSelectNum,BindDatas &Datas,int DataNum);	

	char ErrorMsg[512];
	int  ErrorCode;
};

//	对外接口

typedef oracleoci* (*create_oci)();
typedef void (*destroy_oci)(oracleoci*);

#endif
