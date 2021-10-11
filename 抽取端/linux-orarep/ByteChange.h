//**************************************************************
//*
//* 设 计 者：wqy 日期：2012-5-16:17
//* 修 改 者：    日期：2012-5-16:17
//* 
//*		为aix上的redolog文件能在windows下调试,xlc下无法调试,将字符顺序倒过来
//*
//****************************************************************

#include "Defines.h"

//WORD是2位
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
