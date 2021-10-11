#include <iostream>

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <list>

#include "Defines.h"
#include "BlockPool.h"
//#include "MyLog.h"
#include "ChuckSum.h"
#include "RedologINI.h"
#include "OciQuery.h"
#include "ASM.h"

using namespace std;

int main()
{
	RedologINI ConfigINI;
	ConfigINI.LoadFile("./Redolog.ini");

	char strUSER[128];
	char strPASSWD[128];
	char strSID[128];

	ConfigINI.LoadOCIValuse(strUSER,strPASSWD, strSID);

	ConfigINI.LoadValuse("OCI","ASM",strSID);

cout<<"strSID:\t"<<strSID<<endl;
cout<<"strUSER:\t"<<strUSER<<endl;
cout<<"strPASSWD:\t"<<strPASSWD<<endl;

	OciQuery myQuery;
	myQuery.SetValses(strSID,strUSER,"sys");

	char strBlock[128];
	char strNum[128];
	char strFile[128];
	int Block;
	int Num;
	int size;

	ConfigINI.LoadValuse("TEST","FILENAME",strFile);
	ConfigINI.LoadValuse("TEST","FILESIZE",strBlock);
	ConfigINI.LoadValuse("TEST","NUM",strNum);

	stringstream sstr;
	sstr<<strBlock<<" "<<strNum;
	sstr>>size;
	sstr>>Num;

cout<<"size:\t"<<size<<" Num:\t"<<Num<<"strFile:\t"<<strFile<<endl;

//	Num = 128*16;
	ASM MyASM;
	
	BYTE *themem = new BYTE[8192*128];

	int file2 = open("asmonefile.test",O_RDWR | O_CREAT,0666);

	int AUs = 8192*128;
	cout<<"AUs:\t"<<AUs<<endl;

	int i;
	int loopnum = size;
	for(i=0;i<loopnum;i++)
	{
		Block = i*Num;
		MyASM.SetAsmValues(strFile,&myQuery);
		MyASM.SetBlockInof(Block,Num);
		MyASM.AnalyseASM(themem);
		write(file2,themem,AUs);
	}

	for(int n=0;n<0x10;n++)	{
			cout<<" 0x"<<hex<<(int)themem[n+0x10000];
		}
		cout<<"\n------------------"<<endl;

	MyASM.CloseAllDisks();

	close(file2);

	return 0;
}
