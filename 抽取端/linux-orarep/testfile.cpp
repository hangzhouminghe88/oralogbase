#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <list>

#include "Defines.h"
#include "BlockPool.h"
//#include "MyLog.h"
#include "ChuckSum.h"
#include "RedologINI.h"
#include "OciQuery.h"

extern FILE *ChangeLog;
extern ResourceList Tranlist;
extern ResourceList ResourcePool;

ReturnType ConsumerThread(LPVOID *Context)
{
	Consumer myConsumer;
	BlockNode myNode;
	FILE *fh = fopen("Consumer.txt", "a");

/* 查询操作
	string mystr;
	OciQuery myQuery;
	myQuery.SetValses("orcl","sys","sys");
	myQuery.GetDatafilePath(mystr,1);
*/
	while (true)
	{		
		if (myConsumer.GetFromProduce() == false){
			std::cout<<"..........LOCKED...........\n";
			fdsleep(500);
			continue;
		}
		
//		fprintf(fh,"Before--------%d------\n",ResourcePool.size());
		while(myConsumer.GetOneNode(myNode))
		{
			fprintf(fh,"%d\t%d\t%d\n",myNode.FileIndex,myNode.BlockIndex,myNode.Num);
		}
		fprintf(fh,"0\t0\t0\n");
		fflush(fh);
		myConsumer.GiveBack2Pool();
//		fprintf(fh,"After--------%d------\n",ResourcePool.size());
	}
//	fclose(fh);
	return (ReturnType)0;
}

void Test(FILE *&myfile)
{
	cout<<myfile<<endl;
}

ReturnType ProducerThread(LPVOID *Context)
{
///////////////////////////当前的redolog文件///////////////////////////////////
	FILE* TestFiles = fopen("TestFiles.txt","r");
	Redologs mylogs;

	int bASM = 1;
	OciQuery myQuery;
	myQuery.SetValses("ASM","sys","sys");
	
	RedologFile OneFile;
	//while(!feof(TestFiles)){
		fscanf(TestFiles,"%s %d",&(OneFile.FileName),&(OneFile.Sequence));
		mylogs.push_back(OneFile);
	//}

//	Test(TestFiles);
///////////////////////////当前的redolog文件///////////////////////////////////

	int BlockNum,SCN,offset;
	char Msg[256] = {0};
//	(int &BlockNum,int &SCN,int &Offset,char *TheReason);
	char file[128];

	FILE *RedoFile;
	RedoLog LogFile;
	for(int i=0;i<mylogs.size();i++)
	{
#ifdef _BLOCK_DEBUG_Change
		sprintf(file,"Change.txt",i);
		ChangeLog = fopen(file,"w+");
#endif
		cout<<mylogs[i].FileName<<"\t"<<mylogs[i].Sequence<<"\tbASM:"<<bASM<<endl;
		LogFile.LoadNewFile(mylogs[i].FileName,mylogs[i].nStatus,mylogs[i].Sequence,bASM,&myQuery);
		LogFile.LoadandSkipLogHead();
		if(false == LogFile.Loadall()){
			cout<<"Test Error:"<<mylogs[i].FileName<<endl;
			LogFile.GetErrorMsg(BlockNum,SCN,offset,Msg);
			cout<<" Detial:\t\n\tBlockNum:\t"<<BlockNum
				<<" SCN:\t"<<SCN<<" offset:\t"<<offset<<" Reason:\t"<<Msg<<endl;
		}
		else{
			cout<<"Test OK:"<<mylogs[i].FileName<<endl;
		}
	}		
	return (ReturnType)0;
}



int main()
{
	
	ThreadID dwThreadID;
	HANDLE hThread = MyCreateThread(&dwThreadID,ConsumerThread,NULL);
	
	ThreadID dwThreadID2;
	HANDLE hThread2 = MyCreateThread(&dwThreadID2,ProducerThread,NULL);
	
	pthread_join(dwThreadID2,NULL);

	return 0;
}
