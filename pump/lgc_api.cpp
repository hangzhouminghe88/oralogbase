#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "lgc_api.h"
#include "lgc_param.h"

int lgc_logMsg_s(int level, char *file, long line, char *fmd,...)
{
	int iRet = -1;
	static FILE *fp = NULL;

	char fileName[126]={0};
	char errMsg[256]={0};
	char tmStamp[128]={0};
	char* p = NULL;

	static unsigned long sequence=0;
	static const char* fileDir = LGC_Param::getInstance()->getLogDir();
	
	if(level < 0){//不输出日志信息
		return 0;
	}

	strcpy(fileName, fileDir);
	if(fileName[strlen(fileName)-1] == '/'){
		fileName[strlen(fileName)-1] = 0;
	}
	strcpy(fileName+strlen(fileName), "/lgcPump.log");


	static bool bFirst = true;
	const unsigned long maxFileSize=10*1024*1024;
	long curPos;

	va_list args;
	int num=0;
	char *buf=(char *)malloc(1024);
	if(NULL == buf)
	{
		return -1;
	}
	memset(buf,0,1024);
	
	va_start(args,fmd);
	vsprintf(buf+strlen(buf),fmd,args);
	num=strlen(buf);

	va_end(args);

	//open
	if(bFirst){
		fp=fopen(fileName,"a+");
		bFirst=false;
	}

	//check file descripter
	if(fp == NULL || ftell(fp) < 0){
		snprintf(errMsg,sizeof(errMsg),"%s\n",strerror(errno));
		fprintf(stderr,"%s",errMsg);
		exit(-1);
	}

	//write
	if( num != fwrite(buf,1,num,fp) )
	{
		fflush(fp);
		free(buf);
		exit(1);
	}
	fflush(fp);
	free(buf);

	//查询文件大小
	curPos=ftell(fp);
	//检查文件是否过大
	if(curPos > maxFileSize){//文件过大，则清空重写
		//清空文件
		fclose(fp);
		fp=fopen(fileName,"w");
		if(!fp){
			snprintf(errMsg,sizeof(errMsg),"%s\n",strerror(errno));
			exit(-1);
		}
	}

	return 0;

}

int lgc_errMsg_s(char *file, long line, char *fmd,...)
{
	int iRet = -1;
	static FILE *fp = NULL;
	char fileName[126]={0};
	char errMsg[256]={0};
	char tmStamp[128]={0};

	static unsigned long sequence=0;
	static Mutex s_mutex;
	static const char* fileDir = LGC_Param::getInstance()->getLogDir();

	strcpy(fileName, fileDir);
	if(fileName[strlen(fileName)-1] == '/'){
		fileName[strlen(fileName)-1] = 0;
	}
	strcpy(fileName+strlen(fileName), "/pumpErr.log");


	static bool bFirst = true;
	const unsigned long maxFileSize=10*1024*1024;
	long curPos;

//	tw_time_localNowTimeStr_s(tmStamp,128);

	va_list args;
	int num=0;
	char *buf=(char *)malloc(1024);
	if(NULL == buf)
	{
		return -1;
	}
	memset(buf,0,1024);
	//sprintf(buf,"%s, %u %s: ",file,line,tmStamp);
	sprintf(buf,"%s, %u %s %u: ",file,line,tmStamp,sequence++);
	
	va_start(args,fmd);
	vsprintf(buf+strlen(buf),fmd,args);
	num=strlen(buf);

	va_end(args);

	
	s_mutex.Lock();
	//open 
	if(bFirst){
		fp=fopen(fileName,"w");
		bFirst=false;
	}
	//check
	if(fp == NULL || ftell(fp) < 0)
	{
		snprintf(errMsg,sizeof(errMsg),"%s\n",strerror(errno));
		fprintf(stderr,"%s",errMsg);
		exit(-1);
	}
	
	//write
	if( num != fwrite(buf,1,num,fp) )
	{
		fflush(fp);
		free(buf);
		exit(1);
	}

	fflush(fp);
	free(buf);

	//检查文件大小
	//查询文件大小
	curPos=ftell(fp);
	//检查文件是否过大
	if(curPos > maxFileSize){//文件过大，则清空重写
		//清空文件
		fclose(fp);
		fp=fopen(fileName,"w");
		if(!fp){
			snprintf(errMsg,sizeof(errMsg),"%s\n",strerror(errno));
			exit(-1);
			return -1;
		}
	}
	s_mutex.Unlock();

	return 0;

}

int lgc_scnMsg_s(char* file, long line, char *fmd, ...)
{
	return 0;
}

void byteSwap(void* _buf, const int size)
{
	if(size <= 0){
		lgc_errMsg("size invalid \n");
		return;
	}

	char* buf = (char*)_buf;
	int times = size/2;
	int i = 0;
	char tmp = 0;

	for(i=0;i<times;i++){
		tmp = buf[i];
		buf[i] = buf[size-i-1];
		buf[size-i-1] = tmp;
	}

}
