//**************************************************************
//*
//* �� �� �ߣ�wqy ���ڣ�2012-5-16:17
//* �� �� �ߣ�    ���ڣ�2012-5-16:17
//* 
//*		Ϊaix�ϵ�redolog�ļ�����windows�µ���,xlc���޷�����,���ַ�˳�򵹹���
//*
//****************************************************************

#include "Defines.h"

//WORD��2λ
void ChangeWord(WORD &WinWord)
{
	BYTE MyWord[2];
	memcpy(MyWord,&WinWord,sizeof(WORD));

	BYTE Temp = MyWord[0];
	MyWord[0] = MyWord[1];
	MyWord[1] = Temp;

	memcpy(&WinWord,MyWord,sizeof(WORD));

}

void ChangeDWord(DWORD &WinDWord)
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
