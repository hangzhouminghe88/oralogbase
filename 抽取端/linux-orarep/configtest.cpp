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
	cout<<"INI配置文件测试:\n\t";
	cout<<"读取 ../conf/Redolog.ini:\t";
	RedologINI ConfigINI;
	ConfigINI.LoadFile("../conf/Redolog.ini");

	char strTemp[128] = {0};
	ConfigINI.LoadValuse("OCI","INSTANCE",strTemp);

	cout<<"此主机ORACLE实例名:\t"<<strTemp<<endl;
	
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

	cout<<"OCI接口测试(请确定已经配置好ORACLE_HOME):\n\n";

	cout<<"主备库状态查询:\n\t";

	OciQuery myQuery,myQuery2;
	myQuery.SetValses(strSID,strUSER,strPASSWD);
	myQuery2.SetValses(strSID2,strUSER2,strPASSWD2);

	int State = myQuery.GetOracleState();
	if(State == 0xff){
		cout<<"\t主机:"<<strSID2<<"状态未知\n";
	}else{
		if((State & ORACLE_PRIMARY)){
			cout<<"\t主机:"<<strSID<<"为主库\n";
		}else if((State & ORACLE_STANDBY)){
			cout<<"\t主机:"<<strSID<<"为备库\n";
		}else{
			cout<<"\t主机:"<<strSID<<"为主备状态未知\n";
		}

		cout<<"\t打开状态: "<<State<<endl;
	}

	State = myQuery2.GetOracleState();
	if(State == 0xff){
		cout<<"\t主机:"<<strSID2<<"状态未知\n";
	}else{
		if((State & ORACLE_PRIMARY)){
			cout<<"\t主机:"<<strSID2<<"为主库\n";
		}else if((State & ORACLE_STANDBY)){
			cout<<"\t主机:"<<strSID2<<"为备库\n";
		}else{
			cout<<"\t主机:"<<strSID2<<"为主备状态未知\n";
		}

		cout<<"\t打开状态: "<<State<<endl;
	}

//	cout<<"生成控制文件测试:\n";
//	char controlfile[512] = {0};
//	myQuery.GetNewControlFile(controlfile);
//	cout<<controlfile<<endl;

	string myDatafiles;
	myQuery.GetAllDataFileName((char *)&myDatafiles);
	cout<<myDatafiles.c_str()<<endl;

	char YesNO[128] = {0};
	ConfigINI.LoadValuse("LOG","ENABLE",YesNO);

	cout<<"是否记录日志:";
	if(0 == strcmp(YesNO,"yes")){
		cout<<"\t是\n";
	}else{
		cout<<"\t否\n";
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

	cout<<"RAC查询结果:\t查询到行数"<<ArchivedLogs.size()<<endl;
	if(ArchivedLogs.size() >0){
	cout<<"结果前2行:\n\t"<<ArchivedLogs[0].FileName<<"\n\t";
		if(ArchivedLogs.size() >1){
			cout<<ArchivedLogs[1].FileName<<endl;
		}
	}else{
		char strMessage[MAX_NAME_LENGTH] = {0};
		myQuery.GetErrorMsg(strMessage);
		cout<<"错误信息:\t"<<strMessage<<endl;
	}

	char FILENAME2[256] = {0};
	BYTE8 mySCN;
	myQuery.GetArchivedLogBySEQ(FILENAME2,&mySCN,107);
	cout<<"OCI测试：\t 查询107号文件："<<endl;
	cout<<FILENAME2<<endl;
	cout<<"---SCN----"<<mySCN<<endl;

	char FILENAME[256] = {0};
	cout<<"OCI测试：\t 查询1号文件："<<endl;
		myQuery.GetDatafilePath(FILENAME,1);
	cout<<FILENAME<<endl;
	cout<<"-------"<<endl;

	cout<<"是否支持ASM:"<<endl;

	char Arch[128] = {0};
	ConfigINI.LoadValuse("LOG","ASM",Arch);

	if(0 == strcmp(Arch,"YES")){
		cout<<"\t是\n";
	}else{
		cout<<"\t否\n";
		return 0;
	}

	cout<<"BYTE:\t"<<sizeof(BYTE)<<endl
		<<"WORD:\t"<<sizeof(WORD)<<endl
		<<"DWORD:\t"<<sizeof(DWORD)<<endl
		<<"long:\t"<<sizeof(long)<<endl;
		
	return 0;
}

