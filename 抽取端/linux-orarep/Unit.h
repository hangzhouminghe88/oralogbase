//**************************************************************
//*
//* �� �� �ߣ�wqy ���ڣ�2012-5-16:17
//* �� �� �ߣ�    ���ڣ�2012-5-16:17
//* 
//*		Ϊaix�ϵ�redolog�ļ�����windows�µ���,xlc���޷�����,���ַ�˳�򵹹���
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

//WORD��2λ
void ChangeWord(WORD &WinWord);
void ChangeDWord(DWORD &WinDWord);

BYTE8 Str2BYTE8(char *pStr);//�ַ���ת��ΪBYTE8

bool GetLocaltimeStr(char *pTime,int nstrlen);

#endif
