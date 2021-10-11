//////////////////////////////////////////////////////////////////////
//**************************************************************
//* �ļ���	oracleoci.cpp
//* �� �� �ߣ�wqy	���ڣ�2012-5-15
//* �� �� �ߣ�		���ڣ�2012-5-15
//* �� ����
//*
//****************************************************************

#include "oracleoci.h"
#include <string>
#include <iostream>
#include <sstream>
#include "Defines.h"
#include "lgc_api.h"

using namespace std;

extern "C" oracleoci* CreateClass() {
    return new oracleoci;
}

extern "C" void DestroyClass(oracleoci* pOci) {
	if (pOci)
	{
		delete pOci;
		pOci = NULL;
	}
}

oracleoci::oracleoci()
{
	pEnv = NULL;
	pError = NULL;
	pService = NULL;
	pServer = NULL;
	pSession = NULL;
	pStmt = NULL;
	pDefn = NULL;
	pEnv = 0;

	memset(ErrorMsg,0,sizeof(ErrorMsg));
//	memset(SID,0,sizeof(SID));
	ErrorCode = 0;
}

oracleoci::~oracleoci()
{
	Close();
}

int oracleoci::Close()
{
	if (pStmt)
		OCIHandleFree((dvoid *)pStmt,OCI_HTYPE_STMT);	
	if(pError && pServer)
		OCIServerDetach(pServer,pError,OCI_DEFAULT);
	if(pServer)
		OCIHandleFree((dvoid *)pServer,OCI_HTYPE_SERVER);
	if(pService)
		OCIHandleFree((dvoid *)pService,OCI_HTYPE_SVCCTX);
	if(pError)
		OCIHandleFree((dvoid *)pError,OCI_HTYPE_ERROR);
	if(pSession){
		OCISessionEnd(pService,pError,pSession,OCI_DEFAULT);
		OCIHandleFree((dvoid *)pSession,OCI_HTYPE_SESSION);
	}
	if(pEnv)
		OCIHandleFree((dvoid *)pEnv,OCI_HTYPE_ENV);
	
	pEnv = NULL;
	pError = NULL;
	pService = NULL;
	pServer = NULL;
	pSession = NULL;
	pStmt = NULL;
	pDefn = NULL;
	pEnv = 0;

	return 0;
}

/*======================================================
 * ����ԭ��: int Connect()
 * ˵    ��: ���ӵ�Oracle���ݿ�
 * �������:
 *           char *User		�û���
 *           char *Pwd		����
 *           char *SID		Oracleʵ����
 * �������: ��
 * �� �� ֵ: 0 �ɹ� <0 ʧ��
 *=====================================================*/

int oracleoci::Connect(char *User,char *Pwd,char *SID) 
{
	if(NULL==User || NULL==Pwd || NULL==SID){
		return -1;
	}

	char     m_str_error[BUFSIZE] = {0};

	text     *UserName=NULL;
	text     *Password=NULL;
	text     *DbName=NULL;

	//��ʼ���û��������롢���ݿ�
	UserName = (text *)(User);
	Password = (text *)(Pwd);
	DbName   = (text *)(SID);

#ifdef _DEBUG_QUERY_ON
	cout<<User<<Pwd<<SID<<endl;
	cout<<"----------------\n";
#endif

	if ( (OCIInitialize((ub4)OCI_DEFAULT|OCI_THREADED,(dvoid *)0,NULL, NULL,NULL)) != OCI_SUCCESS)
    {
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"Connect's OCIInitialize return<%s> <%d>\n",m_str_error,ErrorCode);
       	return ERROR_OCIInitialize;
    } 

	if ( (OCIEnvInit((OCIEnv **)&pEnv,OCI_DEFAULT,(size_t)0,NULL)) != OCI_SUCCESS )
    {
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"Connect's OCIEnvInit return<%s>\n",m_str_error);
      	return ERROR_OCIEnvInit;
    }


	//���������
    if ( (OCIHandleAlloc((dvoid *)pEnv,(dvoid **)&pError,OCI_HTYPE_ERROR,0,(dvoid **)0)) != OCI_SUCCESS )
    {
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"Connect's OCIHandleAlloc1 return<%s>\n",m_str_error);
       	return ERROR_OCIHandleAlloc1;
    }

	//������������
	if ( ( OCIHandleAlloc((dvoid *)pEnv,(dvoid **)&pServer,OCI_HTYPE_SERVER,0,(dvoid **)0) ) != OCI_SUCCESS )
	{
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"Connect's OCIHandleAlloc2 return<%s>\n",m_str_error);
       	return ERROR_OCIHandleAlloc2;
	}

	//����������������
	if ( ( OCIServerAttach(pServer,pError,DbName,strlen(SID)+1,OCI_DEFAULT) ) !=OCI_SUCCESS )
    {
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"Connect's OCIServerAttach return<%s>\n",m_str_error);
		return ERROR_OCIServerAttach;
	}

	//���������
	if ( ( OCIHandleAlloc((dvoid *)pEnv,(dvoid **)&pService,OCI_HTYPE_SVCCTX,0,(dvoid **)0) ) != OCI_SUCCESS  )
    {
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"Connect's OCIHandleAlloc3 return<%s>\n",m_str_error);
       	return ERROR_OCIHandleAlloc3;
	}

	if ( (OCIAttrSet(pService,OCI_HTYPE_SVCCTX,pServer,0,OCI_ATTR_SERVER,pError)) != OCI_SUCCESS )
    {
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"Connect's OCIAttrSet return<%s>\n",m_str_error);
       	return ERROR_OCISetHandle;
	}

	//�����û����
	if ( (OCIHandleAlloc((dvoid *)pEnv,(dvoid **)&pSession,OCI_HTYPE_SESSION,0,(dvoid **)0) )!= OCI_SUCCESS )
    {
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"Connect's OCIHandleAlloc4 return<%s>\n",m_str_error);
       	return ERROR_OCIHandleAlloc4;
	}

	//�����û���
	if ( ( OCIAttrSet((dvoid *)pSession,OCI_HTYPE_SESSION,(dvoid *)UserName,strlen(User),OCI_ATTR_USERNAME,pError ) ) != OCI_SUCCESS )
    {
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"Connect's OCIAttrSet UserName return<%s>\n",m_str_error);
       	return ERROR_UserName;
	}

	//��������
	if ( ( OCIAttrSet((dvoid *)pSession,OCI_HTYPE_SESSION,(dvoid *)Password,strlen(Pwd),OCI_ATTR_PASSWORD,pError) ) != OCI_SUCCESS )
    {
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"Connect's OCIAttrSet Password return<%s>\n",m_str_error);
       	return ERROR_Password;
	}

	//�����Ự����
	if ( (  OCISessionBegin(pService,pError,pSession,OCI_CRED_RDBMS,OCI_SYSDBA ) ) != OCI_SUCCESS  )
    {
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"Connect's OCISessionBegin return<%s>\n",m_str_error);
		return ERROR_OCISessionBegin; 
    }

	if ( ( OCIAttrSet(pService,OCI_HTYPE_SVCCTX,pSession,0,OCI_ATTR_SESSION,pError) ) != OCI_SUCCESS )
    {
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"Connect's OCIAttrSet return<%s>\n",m_str_error);
		return ERROR_OCISetSession; 
    }

	//���������
	if ( ( OCIHandleAlloc((dvoid *)pEnv,(dvoid **)&pStmt,OCI_HTYPE_STMT,0,(dvoid **)0) ) != OCI_SUCCESS )
    {
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"Connect's OCIHandleAlloc return<%s>\n",m_str_error);
		return ERROR_OCIHandleAlloc; 
    }

	return 0;
}

/*======================================================
 * ����ԭ��: int SelectNull()
 * ˵    ��: ��ȡSQL���
 * �������: char *sqlStr
 *	
 * �� �� ֵ: 0 �ɹ� <0 ʧ��
 *=====================================================*/
int oracleoci::SelectNull(char *sqlStr)
{
	char  m_str_error[BUFSIZE];
	sword Status;
	text  *Sql = NULL;

	ErrorCode = 0;

	Sql = (text *)sqlStr;

	Status = OCIStmtPrepare(pStmt,pError,Sql,(ub4)strlen((char *)Sql), \
				(ub4)OCI_NTV_SYNTAX,(ub4)OCI_DEFAULT);
	if(Status)
    {
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"ExecSQL OCIStmtPrepare return<%s>\n",m_str_error);
		return -1; 
    }

	Status = OCIStmtExecute(pService,pStmt,pError,(ub4)1,(ub4)0, \
		(CONST OCISnapshot *)NULL,(OCISnapshot *)NULL,OCI_DEFAULT);
	if(Status)
	{
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		if(ErrorCode == 24374){ //��������û�еõ���������Ĵ���
			return 0;
		}		
		if(Status==OCI_NO_DATA)
		{
			sprintf(ErrorMsg,"ExecSQL OCIStmtExecute return OCI_NO_DATA \n");
			return -2;
		}
		return -3;
	}
	return 0;
}


/*======================================================
 * ����ԭ��: int ExecSQL()
 * ˵    ��: ��ȡSQL���
 * �������: char *sqlStr
 * �������: char **Res
 *	
 * �� �� ֵ: 0 �ɹ� <0 ʧ��
 *=====================================================*/
int oracleoci::ExecSQL(char *sqlStr)
{
	char  m_str_error[BUFSIZE];
	sword Status;
	text  *Sql = NULL;

	ErrorCode = 0;

	Sql = (text *)sqlStr;

	Status = OCIStmtPrepare(pStmt,pError,Sql,(ub4)strlen((char *)Sql), \
				(ub4)OCI_NTV_SYNTAX,(ub4)OCI_DEFAULT);
	if(Status)
    {
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"ExecSQL OCIStmtPrepare return<%s>\n",m_str_error);
		return -1; 
    }

	Status = OCIStmtExecute(pService,pStmt,pError,(ub4)1,(ub4)0, \
		(CONST OCISnapshot *)NULL,(OCISnapshot *)NULL,OCI_DEFAULT);
	if(Status)
	{
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"ExecSQL OCIStmtExecute return<%s>\n",m_str_error);
		if(Status==OCI_NO_DATA)
		{
			sprintf(ErrorMsg,"ExecSQL OCIStmtExecute return OCI_NO_DATA \n");
			return -2;
		}
		return -3;
	}
	return 0;
}

int oracleoci::InsertExecSQL(char *sqlStr)
{
	char  m_str_error[BUFSIZE];
	sword Status;
	text  *Sql = NULL;

	Sql = (text *)sqlStr;

	Status = OCIStmtPrepare(pStmt,pError,Sql,(ub4)strlen((char *)Sql), \
				(ub4)OCI_NTV_SYNTAX,(ub4)OCI_DEFAULT);
	if(Status)
    {
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"ExecSQL OCIStmtPrepare return<%s>\n",m_str_error);
		return -1; 
    }

//OCIStmtExecute(svc, inserts,  err, 1, 0, NULL, NULL, OCI_DEFAULT | OCI_COMMIT_ON_SUCCESS); 
	Status = OCIStmtExecute(pService,pStmt,pError,(ub4)1,(ub4)0, \
		(CONST OCISnapshot *)NULL,(OCISnapshot *)NULL,OCI_DEFAULT | OCI_COMMIT_ON_SUCCESS);
	if(Status)
	{
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"ExecSQL OCIStmtExecute return<%s>\n",m_str_error);
		if(Status==OCI_NO_DATA)
		{
			sprintf(ErrorMsg,"ExecSQL OCIStmtExecute return OCI_NO_DATA \n");
			return -2;
		}
		return -3;
	}
	return 0;
}

// ���ص�����
int oracleoci::ExecSQL1(char *sqlStr,HandleFun HFun, void *plist)
{
	char  m_str_error[BUFSIZE];
	int Ret;

	sword Status;
	text *Field=NULL;
	text *Sql=NULL;

	ErrorCode = 0;

	Field = new text[BUFSIZE];//(text *)malloc(BUFSIZE);

	Sql = (text *)sqlStr;

	Status = OCIStmtPrepare(pStmt,pError,Sql,(ub4)strlen((char *)Sql), \
						(ub4)OCI_NTV_SYNTAX,(ub4)OCI_DEFAULT);
	if(Status)
	{
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"ExecSQL1 OCIStmtPrepare return<%s>\n",m_str_error);
		ErrorCode = -2;
		goto Error_Out;
	}
	
	Status = OCIDefineByPos(pStmt,&pDefn,pError,1,(dvoid *)Field,  \
			 BUFSIZE,SQLT_STR,(dvoid *)0,(ub2 *)0,(ub2 *)0,OCI_DEFAULT);
	if(Status)
	{
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"ExecSQL1 OCIDefineByPos return<%s>\n",m_str_error);
		ErrorCode = -3;
		goto Error_Out;
	}

	Status = OCIStmtExecute(pService,pStmt,pError,(ub4)1,(ub4)0,  \
					(CONST OCISnapshot *)NULL,(OCISnapshot *)NULL,OCI_DEFAULT);
	if(Status)
	{
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"ExecSQL1 OCIStmtExecute return<%s>\n",m_str_error);
		if(Status==OCI_NO_DATA)
		{
			ErrorCode = 1;
			goto Error_Out;
		}
		else
		{
			ErrorCode = -5;
			goto Error_Out;
		}
	}

	do
	{
		Ret = HFun(plist,(char *)Field,NULL);
		if(Ret == -1)
		{
			sprintf(ErrorMsg,"EXECSQL1 Get Filed Error\n");
			ErrorCode = -1;
			
		}
		
		if (Ret == 1){	//�Ѿ��鵽���ݣ�������ִ����
			break;
		}

		Status = OCIDefineByPos(pStmt,&pDefn,pError,1,(dvoid *)Field, \
			BUFSIZE,SQLT_STR,(dvoid *)0,(ub2 *)0,(ub2 *)0,OCI_DEFAULT);
		if(Status)
		{  
			OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
			sprintf(ErrorMsg,"EXECSQL1 OCIDefineByPos <%s>\n",m_str_error);
			ErrorCode = -6;
			goto Error_Out;
		}

		Status = OCIStmtFetch(pStmt, pError, 1, OCI_FETCH_NEXT, OCI_DEFAULT);
		if(Status)
		{
			OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, 256, OCI_HTYPE_ERROR);
			if(ErrorCode == 1405){  //����BUG
				continue;
			}
			sprintf(ErrorMsg,"EXECSQL OCIStmtFetch<%s>  --- ErrorCode: %d \n",m_str_error,ErrorCode);
			ErrorCode = Status;
			goto Error_Out;
		}
	}while(true);
Error_Out:
	if (Field)
	{
		delete []Field;
		Field = NULL;
	}
	return ErrorCode;
}

int oracleoci::ExecSQL2(char *sqlStr,HandleFun HFun, void *plist,void *CompareVal)
{
    char m_str_error[BUFSIZE];
	int Ret;
	sword Status;
	text *Field1=NULL,*Field2=NULL;
	text *Sql=NULL;

	ErrorCode = 0;

	Sql = (text *)sqlStr;
	Field1 = new text[BUFSIZE*2];//(text *)malloc(BUFSIZE);
	Field2 = new text[BUFSIZE];//(text *)malloc(BUFSIZE);

	Status = OCIStmtPrepare(pStmt,pError,Sql,(ub4)strlen((char *)Sql), \
			        (ub4)OCI_NTV_SYNTAX,(ub4)OCI_DEFAULT);
	if(Status)
    {  
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"EXECSQL2 OCIStmtPrepare <%s>\n",m_str_error);
		ErrorCode = -2;
		goto Error_Out;
	}

	Status = OCIDefineByPos(pStmt,&pDefn,pError,1,(dvoid *)Field1, \
			 BUFSIZE,SQLT_STR,(dvoid *)0,(ub2 *)0,(ub2 *)0,OCI_DEFAULT);
	if(Status)
	{
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"EXECSQL2 OCIDefineByPos<%s>\n",m_str_error);
		ErrorCode = -3;
		goto Error_Out;
	}

	Status = OCIDefineByPos(pStmt,&pDefn,pError,2,(dvoid *)Field2, \
			 BUFSIZE,SQLT_STR,(dvoid *)0,(ub2 *)0,(ub2 *)0,OCI_DEFAULT);
	if(Status)
	{
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"EXECSQL2 OCIDefineByPos<%s>\n",m_str_error);
		ErrorCode = -4;
		goto Error_Out;
	}

	Status = OCIStmtExecute(pService,pStmt,pError,(ub4)1,(ub4)0, \
		(CONST OCISnapshot *)NULL,(OCISnapshot *)NULL,OCI_DEFAULT);
	if(Status)
	{
		if(Status==OCI_NO_DATA)
		{
			OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
			sprintf(ErrorMsg,"EXECSQL2 OCIStmtExecute<%s>\n",m_str_error);
			ErrorCode = 1;
			goto Error_Out;
		}
		else
		{
			OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
			sprintf(ErrorMsg,"EXECSQL2 OCIStmtExecute<%s>\n",m_str_error);
			ErrorCode = -5;
			goto Error_Out;
		}
	}

	do
	{
		strcat((char*)Field1,"\n");
		strcat((char*)Field1,(char *)Field2);
		
		Ret = HFun(plist,(char *)Field1,(char *)CompareVal);
		if(Ret == -1)
		{
			sprintf(ErrorMsg,"EXECSQL2 Get Filed Error\n");
			ErrorCode = -1;
			
		}
		
		if (Ret == 1){	//�Ѿ��鵽���ݣ�������ִ����
			break;
		}

		
		Status=OCIDefineByPos(pStmt,&pDefn,pError,1,(dvoid *)Field1, \
			BUFSIZE,SQLT_STR,(dvoid *)0,(ub2 *)0,(ub2 *)0,OCI_DEFAULT);
		if(Status)
		{
			OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
			sprintf(ErrorMsg,"EXECSQL2 OCIDefineByPos Field1:<%s>\n",m_str_error);
			ErrorCode = -3;
			goto Error_Out;
		}
		Status=OCIDefineByPos(pStmt,&pDefn,pError,2,(dvoid *)Field2, \
			BUFSIZE,SQLT_STR,(dvoid *)0,(ub2 *)0,(ub2 *)0,OCI_DEFAULT);
		if(Status)
		{
			OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
			sprintf(ErrorMsg,"EXECSQL2 OCIDefineByPos Field2:<%s>\n",m_str_error);
			ErrorCode = -3;
			goto Error_Out;
		}

		Status = OCIStmtFetch(pStmt, pError, 1, OCI_FETCH_NEXT, OCI_DEFAULT);
		if(Status)
		{
			OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, 256, OCI_HTYPE_ERROR);
			if(ErrorCode == 1405){  //����BUG
				continue;
			}
			sprintf(ErrorMsg,"EXECSQL OCIStmtFetch<%s>  --- ErrorCode: %d \n",m_str_error,ErrorCode);
			ErrorCode = Status;
			goto Error_Out;
		}
	}while(true);

Error_Out:
	if (Field1)
	{
		delete []Field1;
		Field1 = NULL;
	}
	if (Field2)
	{
		delete []Field2;
		Field2 = NULL;
	}
	return ErrorCode;
}

int	oracleoci::ExecSQL_tw(char *sqlStr,HandleFun HFun, void *plist,int nSelectNum)
{
	char m_str_error[BUFSIZE];
	int Ret;
	sword Status;
	text *Sql=NULL;

	Sql = (text *)sqlStr;
	ErrorCode = 0;
	
	int Fieldnum = nSelectNum & 0xFFFF;	//��16λ
	int LineNum = nSelectNum >> 16;//��16λ

	text **Field=NULL;
	Field = new text*[Fieldnum];
	memset(Field,0,sizeof(text*)*Fieldnum);
	string strOut;

	Status = OCIStmtPrepare(pStmt,pError,Sql,(ub4)strlen((char *)Sql), \
			        (ub4)OCI_NTV_SYNTAX,(ub4)OCI_DEFAULT);
	if(Status)
    {  
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"EXECSQL OCIStmtPrepare <%s>\n",m_str_error);
		lgc_errMsg("ExecSql failed: sql=%s ErrorMsg=%s\n",sqlStr, ErrorMsg);
		ErrorCode = -2;
		goto Error_Out;
	}


	for(int i=0;i<Fieldnum;i++)
	{	
		Field[i] = new text[BUFSIZE];
		if(NULL == Field[i]){
			lgc_errMsg(" new failed\n");
			exit(1);
			goto Error_Out;
		}
		Status = OCIDefineByPos(pStmt,&pDefn,pError,i+1,(dvoid *)Field[i], BUFSIZE,SQLT_STR,(dvoid *)0,(ub2 *)0,(ub2 *)0,OCI_DEFAULT);
		if(Status)
		{
			OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, 256, OCI_HTYPE_ERROR);
			sprintf(ErrorMsg,"EXECSQL OCIDefineByPos<%s>\n",m_str_error);
			ErrorCode = i*-1;
			lgc_errMsg("ExecSql failed: sql=%s ErrorMsg=%s\n",sqlStr, ErrorMsg);
			goto Error_Out;
		}
	}


	Status = OCIStmtExecute(pService,pStmt,pError,(ub4)1,(ub4)0, \
		(CONST OCISnapshot *)NULL,(OCISnapshot *)NULL,OCI_DEFAULT);
	if(Status)
	{
		if(Status==OCI_NO_DATA)
		{
			OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
			sprintf(ErrorMsg,"EXECSQL ErrorCode: %d OCIStmtExecute<%s>\n",ErrorCode,m_str_error);
			//lgc_errMsg("ExecSql failed: sql=%s ErrorMsg=%s\n",sqlStr, ErrorMsg);
			ErrorCode = 0;
			goto Error_Out;
		}
		else
		{
			OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
			if(ErrorCode != 1405){  //����BUG
				sprintf(ErrorMsg,"EXECSQL ErrorCode: %d OCIStmtExecute2<%s>\n",ErrorCode,m_str_error);
				ErrorCode = -5;
				lgc_errMsg("ExecSql failed: sql=%s ErrorMsg=%s\n",sqlStr, ErrorMsg);
				goto Error_Out;
			}			
		}
	}

	do
	{
		strOut = "";
		for (int i=0;i<Fieldnum; i++ )
		{
			strOut += (char *)Field[i];
			strOut += "\n";
		}

#ifdef _DEBUG_QUERY_ON
		cout<<'\t'<<strOut<<endl;
#endif
		
		Ret = HFun(plist,strOut.c_str(),(char *)&nSelectNum);
		if(Ret <0 )
		{
			sprintf(ErrorMsg,"EXECSQL Get Filed Error\n");
			ErrorCode = -1;
			lgc_errMsg("ExecSql failed: sql=%s ErrorMsg=%s strOut=%s \n",sqlStr, ErrorMsg, strOut);
			goto Error_Out;
		}
		
		if (Ret == QEURY_END_DATA){	//��ֹ�õ�����
			break;
		}
		
		for(int i=0;i<Fieldnum;i++)
		{	
			Status = OCIDefineByPos(pStmt,&pDefn,pError,i+1,(dvoid *)Field[i], BUFSIZE,SQLT_STR,(dvoid *)0,(ub2 *)0,(ub2 *)0,OCI_DEFAULT);
			if(Status)
			{
				OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, 256, OCI_HTYPE_ERROR);
				sprintf(ErrorMsg,"EXECSQL OCIDefineByPos<%s>\n",m_str_error);
				lgc_errMsg("ExecSql failed: sql=%s ErrorMsg=%s\n",sqlStr, ErrorMsg);
				ErrorCode = i*-1;
				goto Error_Out;
			}
		}
		Status = OCIStmtFetch(pStmt, pError, 1, OCI_FETCH_NEXT, OCI_DEFAULT);
		if(Status)
		{
			if(Status==OCI_NO_DATA)
			{
				OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, 256, OCI_HTYPE_ERROR);
				sprintf(ErrorMsg,"EXECSQL ErrorCode: %d OCIStmtFetch<%s>\n",ErrorCode,m_str_error);
				ErrorCode = 0;
				//lgc_errMsg("ExecSql failed: sql=%s ErrorMsg=%s\n",sqlStr, ErrorMsg);
				goto Error_Out;
			} else {			
				OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, 256, OCI_HTYPE_ERROR);
				if(ErrorCode == 1405 ){  //����BUG
					continue;
				}
				sprintf(ErrorMsg,"EXECSQL ErrorCode: %d OCIStmtFetch<%s> \n",ErrorCode, m_str_error);
				lgc_errMsg("ExecSql failed: sql=%s ErrorMsg=%s\n",sqlStr, ErrorMsg);
				ErrorCode = Status;
				goto Error_Out;
			}
		}
	}while(true);		

Error_Out:
#ifdef _DEBUG_QUERY_ON
			cout<<'\t'<<ErrorMsg<<endl;
#endif
	for(int i=0;i<Fieldnum;i++)
	{
		if(Field[i]){
			delete Field[i];
			Field[i] = NULL;
		}
	}
	if(Field){
		delete Field;
		Field = NULL;
	}

	return ErrorCode;


}
int oracleoci::ExecSQL(char *sqlStr,HandleFun HFun, void *plist,int nSelectNum)
{
	char m_str_error[BUFSIZE];
	int Ret;
	sword Status;
	text *Sql=NULL;

	Sql = (text *)sqlStr;
	ErrorCode = 0;
	
	int Fieldnum = nSelectNum & 0xFFFF;	//��16λ
	int LineNum = nSelectNum >> 16;//��16λ

	text **Field=NULL;
	Field = new text*[Fieldnum];
	memset(Field,0,sizeof(text*)*Fieldnum);
	string strOut;

	Status = OCIStmtPrepare(pStmt,pError,Sql,(ub4)strlen((char *)Sql), \
			        (ub4)OCI_NTV_SYNTAX,(ub4)OCI_DEFAULT);
	if(Status)
    {  
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"EXECSQL OCIStmtPrepare <%s>\n",m_str_error);
		ErrorCode = -2;
		goto Error_Out;
	}


	for(int i=0;i<Fieldnum;i++)
	{	
		Field[i] = new text[BUFSIZE];
		if(NULL == Field[i]){
			goto Error_Out;
		}
		Status = OCIDefineByPos(pStmt,&pDefn,pError,i+1,(dvoid *)Field[i], BUFSIZE,SQLT_STR,(dvoid *)0,(ub2 *)0,(ub2 *)0,OCI_DEFAULT);
		if(Status)
		{
			OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, 256, OCI_HTYPE_ERROR);
			sprintf(ErrorMsg,"EXECSQL OCIDefineByPos<%s>\n",m_str_error);
			ErrorCode = i*-1;
			goto Error_Out;
		}
	}


	Status = OCIStmtExecute(pService,pStmt,pError,(ub4)1,(ub4)0, \
		(CONST OCISnapshot *)NULL,(OCISnapshot *)NULL,OCI_DEFAULT);
	if(Status)
	{
		if(Status==OCI_NO_DATA)
		{
			OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
			sprintf(ErrorMsg,"EXECSQL ErrorCode: %d OCIStmtExecute<%s>\n",ErrorCode,m_str_error);
			ErrorCode = 1;
			goto Error_Out;
		}
		else
		{
			OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
                            256, OCI_HTYPE_ERROR);
			if(ErrorCode != 1405){  //����BUG
				sprintf(ErrorMsg,"EXECSQL ErrorCode: %d OCIStmtExecute2<%s>\n",ErrorCode,m_str_error);
				ErrorCode = -5;
				goto Error_Out;
			}			
		}
	}

	do
	{
		strOut = "";
		for (int i=0;i<Fieldnum; i++ )
		{
			strOut += (char *)Field[i];
			strOut += "\n";
		}

#ifdef _DEBUG_QUERY_ON
		cout<<'\t'<<strOut<<endl;
#endif
		
		Ret = HFun(plist,strOut.c_str(),(char *)&LineNum);
		if(Ret == QEURY_ERROR_DATA)
		{
			sprintf(ErrorMsg,"EXECSQL Get Filed Error\n");
			ErrorCode = -1;
			goto Error_Out;
		}
		
		if (Ret == QEURY_END_DATA){	//��ֹ�õ�����
			break;
		}
		
		for(int i=0;i<Fieldnum;i++)
		{	
			Status = OCIDefineByPos(pStmt,&pDefn,pError,i+1,(dvoid *)Field[i], BUFSIZE,SQLT_STR,(dvoid *)0,(ub2 *)0,(ub2 *)0,OCI_DEFAULT);
			if(Status)
			{
				OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, 256, OCI_HTYPE_ERROR);
				sprintf(ErrorMsg,"EXECSQL OCIDefineByPos<%s>\n",m_str_error);
				ErrorCode = i*-1;
				goto Error_Out;
			}
		}
		Status = OCIStmtFetch(pStmt, pError, 1, OCI_FETCH_NEXT, OCI_DEFAULT);
		if(Status)
		{
			if(Status==OCI_NO_DATA)
			{
				OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, 256, OCI_HTYPE_ERROR);
				sprintf(ErrorMsg,"EXECSQL ErrorCode: %d OCIStmtFetch<%s>\n",ErrorCode,m_str_error);
				ErrorCode = 1;
				goto Error_Out;
			} else {			
				OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, 256, OCI_HTYPE_ERROR);
				if(ErrorCode == 1405 ){  //����BUG
					continue;
				}
				sprintf(ErrorMsg,"EXECSQL ErrorCode: %d OCIStmtFetch<%s> \n",ErrorCode, m_str_error);
				ErrorCode = Status;
				goto Error_Out;
			}
		}
	}while(true);		

Error_Out:
#ifdef _DEBUG_QUERY_ON
			cout<<'\t'<<ErrorMsg<<endl;
#endif
	for(int i=0;i<Fieldnum;i++)
	{
		if(Field[i]){
			delete Field[i];
			Field[i] = NULL;
		}
	}
	if(Field){
		delete Field;
		Field = NULL;
	}

	return ErrorCode;
}
int oracleoci::ExecSQL(char *sqlStr,HandleFun HFun, void *plist,int nSelectNum,BindDatas &Datas,int DataNum)
{
	char m_str_error[BUFSIZE];
	int Ret;
	sword Status;
	text *Sql=NULL;
	int len = Datas.size();
	int type = 0;
	Sql = (text *)sqlStr;
	ErrorCode = 0;
	int Fieldnum = nSelectNum & 0xFFFF;	//��16λ
	int LineNum = nSelectNum >> 16;//��16λ
	text **Field=NULL;
	Field = new text*[Fieldnum];
	memset(Field,0,sizeof(text*)*Fieldnum);
	string strOut;
	if(len < DataNum){
		sprintf(ErrorMsg,"������������Ӧ\n");
		ErrorCode = -100;
		goto Error_Out;
	}
	Status = OCIStmtPrepare(pStmt,pError,Sql,(ub4)strlen((char *)Sql), \
		(ub4)OCI_NTV_SYNTAX,(ub4)OCI_DEFAULT);
	if(Status)
	{  
		OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
			256, OCI_HTYPE_ERROR);
		sprintf(ErrorMsg,"EXECSQL OCIStmtPrepare <%s>\n",m_str_error);
		ErrorCode = -2;
		goto Error_Out;
	}
	OCIBind* bindhp_temp;
	for (int i=1;i<=len;++i)
	{
		type = Datas[i-1].Type;
		switch (type)
		{
		case SQLT_INT:
#ifdef _DEBUG_QUERY_ON
				cout<<"BIND :\t"<<i<<endl;
				cout<<"SQLT_INT"<<'\t'<<Datas[i-1].DataInt<<endl;
#endif
			Status = OCIBindByPos(pStmt,&bindhp_temp,pError,(ub4)i,(dvoid *)&(Datas[i-1].DataInt),(sb4)sizeof(sb4),SQLT_INT,(dvoid *) 0,(ub2 *)0,(ub2 *)0,(ub4) 0, (ub4 *) 0,(ub4) OCI_DEFAULT);
			break;
		case SQLT_LNG:
			{	// ����64λ���֣�ֻ����������
				char ComStr[SQLT_LNG_STR_LEN];
				memset(ComStr,0,sizeof(ComStr));
				stringstream sstr;
				sstr<<Datas[i-1].DataLong;
				sstr>>ComStr;
				int len = strlen(ComStr)+1;
#ifdef _DEBUG_QUERY_ON
				cout<<"BIND :\t"<<i<<endl;
				cout<<"SQLT_LNG"<<'\t'<<ComStr<<endl;
				cout<<"len:\t"<<len<<endl;
#endif
				Status = OCIBindByPos(pStmt,&bindhp_temp,pError,(ub4)i,(dvoid *)ComStr,(sb4)len,SQLT_STR,(dvoid *) 0,(ub2 *)0,(ub2 *)0,(ub4) 0, (ub4 *) 0,(ub4) OCI_DEFAULT);
			}
			break;
		default:
			{
				char *pstr = Datas[i-1].DataStr;
				int len = strlen(pstr)+1;//ע�⣬����Ҫ+1,Ҫ����0
#ifdef _DEBUG_QUERY_ON
				cout<<"BIND :\t"<<i<<endl;
				cout<<"SQLT_STR"<<'\t'<<pstr<<endl;
#endif
				Status = OCIBindByPos(pStmt,&bindhp_temp,pError,(ub4)i,(dvoid *)pstr,(sb4)len,SQLT_STR,(dvoid *) 0,(ub2 *)0,(ub2 *)0,(ub4) 0, (ub4 *) 0,(ub4) OCI_DEFAULT);
			}
			break;
		}		
		if(Status)
		{  
			OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
				256, OCI_HTYPE_ERROR);
			sprintf(ErrorMsg,"EXECSQL OCIStmtPrepare <%s>\n",m_str_error);
			ErrorCode = -99;
			goto Error_Out;
		}
	}
	for(int i=0;i<Fieldnum;i++)
	{	
		Field[i] = NULL;
		Field[i] = new text[BUFSIZE];
		if(NULL == Field[i]){
			goto Error_Out;
		}
		Status = OCIDefineByPos(pStmt,&pDefn,pError,i+1,(dvoid *)Field[i], BUFSIZE,SQLT_STR,(dvoid *)0,(ub2 *)0,(ub2 *)0,OCI_DEFAULT);
		if(Status)
		{
			OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, 256, OCI_HTYPE_ERROR);
			sprintf(ErrorMsg,"EXECSQL OCIDefineByPos<%s>\n",m_str_error);
			ErrorCode = i*-1;
			goto Error_Out;
		}
	}
	Status = OCIStmtExecute(pService,pStmt,pError,(ub4)1,(ub4)0, \
		(CONST OCISnapshot *)NULL,(OCISnapshot *)NULL,OCI_DEFAULT);
#ifdef _DEBUG_QUERY_ON
	cout<<"OCIStmtExecute Status :\t"<<Status<<endl;
#endif
	if(Status)
	{
		if(Status==OCI_NO_DATA)
		{
			OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
				256, OCI_HTYPE_ERROR);
			sprintf(ErrorMsg,"EXECSQL ErrorCode: %d OCIStmtExecute<%s>\n",ErrorCode,m_str_error);
			ErrorCode = 1;
			goto Error_Out;
		}
		else
		{
			OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, \
				256, OCI_HTYPE_ERROR);
			if(ErrorCode != 1405){  //����BUG
				sprintf(ErrorMsg,"EXECSQL ErrorCode: %d OCIStmtExecute2<%s>\n",ErrorCode,m_str_error);
				ErrorCode = -5;
				goto Error_Out;
			}			
		}
	}
	do
	{
		strOut = "";
		for (int i=0;i<Fieldnum; i++ )
		{
			strOut += (char *)Field[i];
			strOut += "\n";
		}
#ifdef _DEBUG_QUERY_ON
		cout<<'\t'<<strOut<<endl;
#endif
		Ret = HFun(plist,strOut.c_str(),(char *)&LineNum);
		if(Ret == QEURY_ERROR_DATA)
		{
			sprintf(ErrorMsg,"EXECSQL Get Filed Error\n");
			ErrorCode = -1;
			goto Error_Out;
		}
		if (Ret == QEURY_END_DATA){	//��ֹ�õ�����
			break;
		}
		for(int i=0;i<Fieldnum;i++)
		{	
			Status = OCIDefineByPos(pStmt,&pDefn,pError,i+1,(dvoid *)Field[i], BUFSIZE,SQLT_STR,(dvoid *)0,(ub2 *)0,(ub2 *)0,OCI_DEFAULT);
			if(Status)
			{
				OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, 256, OCI_HTYPE_ERROR);
				sprintf(ErrorMsg,"EXECSQL OCIDefineByPos<%s>\n",m_str_error);
				ErrorCode = i*-1;
				goto Error_Out;
			}
		}
		Status = OCIStmtFetch(pStmt, pError, 1, OCI_FETCH_NEXT, OCI_DEFAULT);
		if(Status)
		{
			if(Status==OCI_NO_DATA)
			{
				OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, 256, OCI_HTYPE_ERROR);
				sprintf(ErrorMsg,"EXECSQL ErrorCode: %d OCIStmtFetch<%s>\n",ErrorCode,m_str_error);
				ErrorCode = 1;
				goto Error_Out;
			} else {			
				OCIErrorGet(pError, 1, NULL, &ErrorCode, (OraText *)m_str_error, 256, OCI_HTYPE_ERROR);
				if(ErrorCode == 1405){  //����BUG
					continue;
				}
				sprintf(ErrorMsg,"EXECSQL ErrorCode: %d OCIStmtFetch<%s> \n",ErrorCode, m_str_error);
				ErrorCode = Status;
				goto Error_Out;
			}
		}
	}while(true);		
Error_Out:
#ifdef _DEBUG_QUERY_ON
	cout<<'\t'<<ErrorMsg<<endl;
#endif
	for(int i=0;i<Fieldnum;i++)
	{
		if(Field[i]){
			delete Field[i];
			Field[i] = NULL;
		}
	}
	if(Field){
		delete Field;
		Field = NULL;
	}
	return ErrorCode;
}
