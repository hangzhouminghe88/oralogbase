//**************************************************************
//*
//* 设 计 者：wqy 日期：2012-5-16:17
//* 修 改 者：    日期：2012-5-16:17
//* 
//*		为aix上的redolog文件能在windows下调试,xlc下无法调试,将字符顺序倒过来
//*
//****************************************************************


#ifndef _UNIT_
#define _UNIT_


#include "Defines.h"
#include <time.h>
#include <string.h>
#include <stdio.h>

#ifdef WIN32
	#define MYOPEN_READ (_O_BINARY|_O_RDONLY)
	#define MYOPNE_WRITE (_O_BINARY|_O_WRONLY)
	#define MYOPNE_READ_WRITE (_O_BINARY|_O_RDWR)
#else

	#define MYOPEN_READ O_RDONLY
	#define MYOPNE_WRITE O_WRONLY
	#define MYOPNE_READ_WRITE O_RDWR
	#define _O_CREAT O_CREAT
#endif

//WORD是2位
void ChangeWord(WORD &WinWord);
void ChangeDWord(DWORD &WinDWord);

BYTE8 Str2BYTE8(char *pStr);//字符串转化为BYTE8

bool GetLocaltimeStr(char *pTime,int nstrlen);

#endif
