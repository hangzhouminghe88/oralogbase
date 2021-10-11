

#include "RedologINI.h"
#include <sstream>
#include "Defines.h"
#include <iostream>

using namespace std;

RedologINI::RedologINI(const char* pIniFile):m_ini(true, false, true),bLoaded(false)
{
	if(NULL == pIniFile)
	{
		strncpy(m_IniFileName,_CONFIG_FILE,sizeof(m_IniFileName)-1);
	}else{
		strncpy(m_IniFileName,pIniFile,sizeof(m_IniFileName)-1);
	}
	m_IniFileName[sizeof(m_IniFileName)-1] = 0;
}

int RedologINI::LoadFile()
{
	return LoadFile(m_IniFileName);
}

int RedologINI::SaveFile()
{
	return SaveFile(m_IniFileName);
}

int RedologINI::LoadFile(char *FileName)
{
	if(FileName == NULL)
		return false;

	int Ret = m_ini.LoadFile(FileName);
	if(0 == Ret){
		bLoaded = true;
		strncpy(m_IniFileName,FileName,sizeof(m_IniFileName)-1);
		m_IniFileName[sizeof(m_IniFileName)-1] = 0;
	}
	return Ret;
}

int RedologINI::SaveFile(char *FileName)
{
	return m_ini.SaveFile(FileName);
}

long RedologINI::LoadSCN()
{
	if(bLoaded == false)
		LoadFile(_CONFIG_FILE);
	if (bLoaded){
		return m_ini.GetLongValue("LOG","SCN");
	}else{
		return 0;
	}
}
	
bool RedologINI::SaveSCN(long nSCN)
{
	if(bLoaded == false)
		LoadFile(_CONFIG_FILE);
	if (bLoaded){
		string strSCN;
		stringstream sstr;
		sstr<<nSCN;
		sstr>>strSCN;
		m_ini.SetValue("LOG","SCN",strSCN.c_str());
		return SaveFile(m_IniFileName);
	}else{
		return false;
	}
	return true;
}

bool RedologINI::LoadOCIValuse(string &user,string &passwd, string &SID)
{
	if(bLoaded == false)
		LoadFile(_CONFIG_FILE);
	if (bLoaded){
		const char *pStr = NULL;
		pStr = m_ini.GetValue("OCI","USER");
		if(pStr) user = pStr;
		pStr = m_ini.GetValue("OCI","PASSWD");
		if(pStr) passwd = pStr;
		pStr = m_ini.GetValue("OCI","SID");
		if(pStr) SID = pStr;
		if(user.size() == 0||
			SID.size() == 0)
			{
			return false;
			}
	}else{
		return false;
	}
	return true;
}

bool RedologINI::LoadOCIValuse(char *user,char *passwd, char *SID)
{
	if(bLoaded == false)
		LoadFile(_CONFIG_FILE);
	if (bLoaded){
		const char *pStr = NULL;
		pStr = m_ini.GetValue("OCI","USER");
		if(pStr)
			strcpy(user, pStr);
		pStr = m_ini.GetValue("OCI","PASSWD");
		if(pStr)
			strcpy(passwd, pStr);
		pStr = m_ini.GetValue("OCI","SID");
		if(pStr)
			strcpy(SID, pStr);
	}
	return true;
}

bool RedologINI::LoadValuse(const char* Section,const char* key, char *value)
{
	if(bLoaded == false)
		LoadFile(_CONFIG_FILE);
	if (bLoaded){
		const char *pStr = NULL;
		pStr = m_ini.GetValue(Section,key);
		if(pStr){
			strcpy(value, pStr);
			return true;
		}
	}
	return false;
}

bool RedologINI::SaveValuse(const char* Section,const char* key, const char *value)
{
	if(bLoaded == false)
		LoadFile(_CONFIG_FILE);
	if (bLoaded){
		m_ini.SetValue(Section,key,value);
		return SaveFile(m_IniFileName);
	}
	return false;
}


long RedologINI::GetlongValue(const char * pSection, const char * pKey)
{
	if(bLoaded == false)
		LoadFile(_CONFIG_FILE);
	if (bLoaded){
		return m_ini.GetLongValue(pSection,pKey);
	}
	return 0;
}

BYTE8 RedologINI::GetBYTE8Value(const char * pSection, const char * pKey)
{
	if(bLoaded == false)
		LoadFile(_CONFIG_FILE);
	if (bLoaded){
		char strSCN[65];
		LoadValuse(pSection,pKey,strSCN);
		BYTE8 nSCN = 0;
#ifdef _WIN32
		nSCN = _atoi64(strSCN);
#else
		sscanf(strSCN, "%lld", &nSCN);
#endif
		return nSCN;
	}
	return 0;
}

bool RedologINI::SavelongValuse(const char* Section,const char* key, long value)
{
	if(bLoaded == false)
		LoadFile(_CONFIG_FILE);
	if (bLoaded){
		m_ini.SetLongValue(Section,key,value);
		return SaveFile(m_IniFileName);
	}
	return false;
}



