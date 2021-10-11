#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "Defines.h"
#include "BlockPool.h"
#include "ChuckSum.h"
#include "RedologINI.h"
#include "OciQuery.h"

int main()
{
	cout<<"INI�����ļ�����:\n\t";
	cout<<"��ȡ ../conf/Redolog.ini:\t";
	RedologINI ConfigINI;
	ConfigINI.LoadFile("../conf/Redolog.ini");

	char strTemp[128] = {0};
	ConfigINI.LoadValuse("OCI","INSTANCE",strTemp);

	cout<<"������ORACLEʵ����:\t"<<strTemp<<endl;
	
	char strUSER[MAX_NAME_LENGTH]= {0};
	char strPASSWD[MAX_NAME_LENGTH] = {0};
	char strSID[MAX_NAME_LENGTH] = {0};
	char strUSER_ASM[MAX_NAME_LENGTH]= {0};
	char strPASSWD_ASM[MAX_NAME_LENGTH] = {0};
	char strSID_ASM[MAX_NAME_LENGTH] = {0};
	char strUSER2[MAX_NAME_LENGTH]= {0};
	char strPASSWD2[MAX_NAME_LENGTH] = {0};
	char strSID2[MAX_NAME_LENGTH] = {0};
	ConfigINI.LoadOCIValuse(strUSER,strPASSWD,strSID);
	ConfigINI.LoadValuse("OCI", "USER2", strUSER2);
	ConfigINI.LoadValuse("OCI", "PASSWD2", strPASSWD2);
	ConfigINI.LoadValuse("OCI", "SID2", strSID2);

	cout<<"OCI�ӿڲ���(��ȷ���Ѿ����ú�ORACLE_HOME):\n\n";

	cout<<"������״̬��ѯ:\n\t";

	OciQuery myQuery,myQuery2;
	myQuery.SetValses(strSID,strUSER,strPASSWD);
	myQuery2.SetValses(strSID2,strUSER2,strPASSWD2);

	int State = myQuery.GetOracleState();
	if(State == 0xff){
		cout<<"\t����:"<<strSID2<<"״̬δ֪\n";
	}else{
		if((State & ORACLE_PRIMARY)){
			cout<<"\t����:"<<strSID<<"Ϊ����\n";
		}else if((State & ORACLE_STANDBY)){
			cout<<"\t����:"<<strSID<<"Ϊ����\n";
		}else{
			cout<<"\t����:"<<strSID<<"Ϊ����״̬δ֪\n";
		}

		cout<<"\t��״̬: "<<State<<endl;
	}

	State = myQuery2.GetOracleState();
	if(State == 0xff){
		cout<<"\t����:"<<strSID2<<"״̬δ֪\n";
	}else{
		if((State & ORACLE_PRIMARY)){
			cout<<"\t����:"<<strSID2<<"Ϊ����\n";
		}else if((State & ORACLE_STANDBY)){
			cout<<"\t����:"<<strSID2<<"Ϊ����\n";
		}else{
			cout<<"\t����:"<<strSID2<<"Ϊ����״̬δ֪\n";
		}

		cout<<"\t��״̬: "<<State<<endl;
	}

//	cout<<"���ɿ����ļ�����:\n";
//	char controlfile[512] = {0};
//	myQuery.GetNewControlFile(controlfile);
//	cout<<controlfile<<endl;

	string myDatafiles;
	myQuery.GetAllDataFileName((char *)&myDatafiles);
	cout<<myDatafiles.c_str()<<endl;

	char YesNO[128] = {0};
	ConfigINI.LoadValuse("LOG","ENABLE",YesNO);

	cout<<"�Ƿ��¼��־:";
	if(0 == strcmp(YesNO,"yes")){
		cout<<"\t��\n";
	}else{
		cout<<"\t��\n";
	}

	long LastSCN = ConfigINI.LoadSCN();
	ConfigINI.LoadValuse("LOG","DATE",strTemp);

	cout<<"\n\tUSER:\t"<<strUSER<<"\n\tPASSWORD:\t"<<strPASSWD
		<<"\n\tSID:\t"<<strSID<<"\n\tSCN:\t"<<LastSCN
		<<"\n\tDATA:\t"<<strTemp<<endl;	

	Redologs ArchivedLogs;


	ArchivedLogs.clear();
	long RACSCN = ConfigINI.GetlongValue("RAC2","SCN");
	myQuery.GetArchivedLogBySCN_DATE(ArchivedLogs,LastSCN,NULL,RACSCN);

	cout<<"RAC��ѯ���:\t��ѯ������"<<ArchivedLogs.size()<<endl;
	if(ArchivedLogs.size() >0){
	cout<<"���ǰ2��:\n\t"<<ArchivedLogs[0].FileName<<"\n\t";
		if(ArchivedLogs.size() >1){
			cout<<ArchivedLogs[1].FileName<<endl;
		}
	}else{
		char strMessage[MAX_NAME_LENGTH] = {0};
		myQuery.GetErrorMsg(strMessage);
		cout<<"������Ϣ:\t"<<strMessage<<endl;
	}

	char FILENAME2[256] = {0};
	BYTE8 mySCN;
	myQuery.GetArchivedLogBySEQ(FILENAME2,&mySCN,107);
	cout<<"OCI���ԣ�\t ��ѯ107���ļ���"<<endl;
	cout<<FILENAME2<<endl;
	cout<<"---SCN----"<<mySCN<<endl;

	char FILENAME[256] = {0};
	cout<<"OCI���ԣ�\t ��ѯ1���ļ���"<<endl;
		myQuery.GetDatafilePath(FILENAME,1);
	cout<<FILENAME<<endl;
	cout<<"-------"<<endl;

	cout<<"�Ƿ�֧��ASM:"<<endl;

	char Arch[128] = {0};
	ConfigINI.LoadValuse("LOG","ASM",Arch);

	if(0 == strcmp(Arch,"YES")){
		cout<<"\t��\n";
	}else{
		cout<<"\t��\n";
		return 0;
	}

	cout<<"BYTE:\t"<<sizeof(BYTE)<<endl
		<<"WORD:\t"<<sizeof(WORD)<<endl
		<<"DWORD:\t"<<sizeof(DWORD)<<endl
		<<"long:\t"<<sizeof(long)<<endl;
		
	return 0;
}

