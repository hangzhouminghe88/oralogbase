

#include "lgc_api.h"
//#include "lgc_param.h"

void getDataStr(WORD dataLen, char* pDataSrc, char* prtStr)
{
	const int prtLen = dataLen > 20? 20:dataLen;
	
	char* p = NULL;
	int j;
	
	if(dataLen == 0){
		strcpy(prtStr, "null");
		return;
	}
	
	p = prtStr;

	for(j=0; j<prtLen; j++){
			sprintf(p,"%.2x", (unsigned char)pDataSrc[j]);
			p += strlen(p);
	}

}

/*
int lgc_logMsg_s(int level, char *file, long line, char *fmd,...)
{
	int iRet = -1;
	static FILE *fp = NULL;

	char fileName[126]={0};
	char errMsg[256]={0};
	char tmStamp[128]={0};

	static unsigned long sequence=0;
	
	if(level < 0){//不输出日志信息
		return 0;
	}

	snprintf(fileName,sizeof(fileName), "%s", "/home/tw/lgc/log/lgc.log");


	static bool bFirst = true;
	const unsigned long maxFileSize=10*1024*1024;
	long curPos;
	

	
	if(bFirst){
		fp=fopen(fileName,"a+");
		bFirst=false;
	}else{
		if(ftell(fp) < 0){//fill is remove or damaged
			snprintf(errMsg,sizeof(errMsg),"%s\n",strerror(errno));
			fprintf(stderr,"%s",errMsg);
			exit(-1);
		}
	}

	if(fp == NULL)
	{
		snprintf(errMsg,sizeof(errMsg),"%s\n",strerror(errno));
		fprintf(stderr,"%s",errMsg);
		exit(-1);
		return -1;
	}

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
	

	tw_time_localNowTimeStr_s(tmStamp,128);

	va_list args;
	int num=0;
	char *buf=(char *)malloc(1024);
	if(NULL == buf)
	{
		return -1;
	}
	memset(buf,0,1024);
	//sprintf(buf,"%s, %u %s: ",file,line,tmStamp);
	//sprintf(buf,"%s, %u %s %u: ",file,line,tmStamp,sequence++);
	
	va_start(args,fmd);
	vsprintf(buf+strlen(buf),fmd,args);
	num=strlen(buf);

	va_end(args);


	if( num != fwrite(buf,1,num,fp) )
	{
		fflush(fp);
		free(buf);
		exit(1);
		return -1;
	}

	fflush(fp);
	free(buf);
	return 0;

}
*/


int lgc_logMsg_s(int level, char *file, long line, char *fmd,...)
{
	int iRet = -1;
	static FILE *fp = NULL;

	char fileName[126]={0};
	char errMsg[256]={0};
	char tmStamp[128]={0};
	char* p = NULL;

	static unsigned long sequence=0;
	static Mutex s_mutex;
	//static const char* fileDir = LGC_Param::getInstance()->getLogDir();
	static const char* fileDir = "/home/tw/lgc/log";

	if(level < 0){//不输出日志信息
		return 0;
	}

	strcpy(fileName, fileDir);
	if(fileName[strlen(fileName)-1] == '/'){
		fileName[strlen(fileName)-1] = 0;
	}
	strcpy(fileName+strlen(fileName), "/lgc.log");


	static bool bFirst = true;
	const unsigned long maxFileSize=10*1024*1024;
	long curPos;

	tw_time_localNowTimeStr_s(tmStamp,128);

	va_list args;
	int num=0;
	char *buf=(char *)malloc(1024);
	if(NULL == buf)
	{
		return -1;
	}
	memset(buf,0,1024);
	//sprintf(buf,"%s, %u %s: ",file,line,tmStamp);
	//sprintf(buf,"%s, %u %s %u: ",file,line,tmStamp,sequence++);
	
	va_start(args,fmd);
	vsprintf(buf+strlen(buf),fmd,args);
	num=strlen(buf);

	va_end(args);

	
	s_mutex.Lock();
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
	s_mutex.Unlock();





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
	//static const char* fileDir = LGC_Param::getInstance()->getLogDir();
	static const char* fileDir = "/home/tw/lgc/log";

	strcpy(fileName, fileDir);
	if(fileName[strlen(fileName)-1] == '/'){
		fileName[strlen(fileName)-1] = 0;
	}
	strcpy(fileName+strlen(fileName), "/lgcErr.log");


	static bool bFirst = true;
	const unsigned long maxFileSize=10*1024*1024;
	long curPos;

	tw_time_localNowTimeStr_s(tmStamp,128);

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
		//fp=fopen(fileName,"a+");
		fp=fopen(fileName,"wb");
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
	int iRet = -1;
	static FILE *fp = NULL;
	char fileName[126]={0};
	char errMsg[256]={0};
	char tmStamp[128]={0};

	static unsigned long sequence=0;
	static Mutex s_mutex;
//	static const char* fileDir = LGC_Param::getInstance()->getLogDir();
	static const char* fileDir = "/home/tw/lgc/log";
	strcpy(fileName, fileDir);
	if(fileName[strlen(fileName)-1] == '/'){
		fileName[strlen(fileName)-1] = 0;
	}
	strcpy(fileName+strlen(fileName), "/lgcSCN.log");

	static bool bFirst = true;
	const unsigned long maxFileSize=100*1024*1024;
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
	//sprintf(buf,"%s, %u %s %u: ",file,line,tmStamp,sequence++);
	
	va_start(args,fmd);
	vsprintf(buf+strlen(buf),fmd,args);
	num=strlen(buf);

	va_end(args);

	
	s_mutex.Lock();
	//open
	if(bFirst){
		fp=fopen(fileName,"a+");
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
		}
	}
	s_mutex.Unlock();

	return 0;

}




