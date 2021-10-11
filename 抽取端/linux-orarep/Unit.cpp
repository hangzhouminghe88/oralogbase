//**************************************************************
//*
//* 设 计 者：wqy 日期：2012-5-16:17
//* 修 改 者：    日期：2012-5-16:17
//* 
//*		为aix上的redolog文件能在windows下调试,xlc下无法调试,将字符顺序倒过来
//*
//****************************************************************

#include "Unit.h"

//WORD是2位
void Changeword(WORD &WinWord)
{
	BYTE MyWord[2];
	memcpy(MyWord,&WinWord,sizeof(WORD));

	BYTE Temp = MyWord[0];
	MyWord[0] = MyWord[1];
	MyWord[1] = Temp;

	memcpy(&WinWord,MyWord,sizeof(WORD));

}

void Changedword(DWORD &WinDWord)
{
	BYTE MyWord[4];
	memcpy(MyWord,&WinDWord,sizeof(DWORD));
	
	BYTE Temp = MyWord[0];
	MyWord[0] = MyWord[3];
	MyWord[3] = Temp;

	Temp = MyWord[1];
	MyWord[1] = MyWord[2];
	MyWord[2] = Temp;
	
	memcpy(&WinDWord,MyWord,sizeof(DWORD));

}

BYTE8 Str2BYTE8(char *pStr)
{
	BYTE8 nB8 = 0;
#ifdef _WIN32
	nB8 = _atoi64(pStr);
#else
	sscanf(pStr, "%lld", &nB8);
#endif
	return nB8;
}


bool GetLocaltimeStr(char *pTime,int nstrlen)
{
	bool retVal = true;
	struct tm *tm;
#ifdef WIN64
	__time64_t now;
	_time64(&now);
	tm = _localtime64(&now);
#else
#ifdef WIN32
	__time32_t now = time(NULL);
	tm = _localtime32(&now);
#else
	time_t t;
	time(&t);
	tm= localtime(&t);
#endif
#endif

	strftime(pTime, nstrlen, "%Y-%m-%d %H:%M:%S", tm);
	return retVal;
}


