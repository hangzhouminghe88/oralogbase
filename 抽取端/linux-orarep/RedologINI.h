//**************************************************************
//*
//* 文件名: RedologINI.h
//* 设 计 者：wqy 日期：2012-5-27
//* 说明: 使用开源类SimpleIni操作ini
//*
//****************************************************************

#ifndef _REDOLOG_INI
#define _REDOLOG_INI

#include <string.h>
#include "SimpleIni.h"
#include "Defines.h"

using namespace std;

class RedologINI
{
private:
	CSimpleIniA m_ini;//(true, true, true);
	char m_IniFileName[MAX_NAME_LENGTH];
	bool bLoaded;
public:
	RedologINI(const char* pIniFile = NULL);

	int LoadFile();
	int LoadFile(char *FileName);

	int SaveFile();
	int	SaveFile(char *FileName);

	long LoadSCN();
	bool SaveSCN(long nSCN);

	long GetlongValue(const char * pSection, const char * pKey);
	BYTE8 GetBYTE8Value(const char * pSection, const char * pKey);
	bool SavelongValuse(const char* Section,const char* key, long value);

	bool LoadOCIValuse(string &user,string &passwd, string &SID);
	bool LoadOCIValuse(char *user,char *passwd, char *SID);

	bool LoadValuse(const char* Section,const char* key, char *value);
	bool SaveValuse(const char* Section,const char* key, const char *value);
};

#endif
