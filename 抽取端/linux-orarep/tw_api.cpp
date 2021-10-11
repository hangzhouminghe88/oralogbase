#include "tw_api.h"
#include "RedologINI.h"
#include "Mutex.h"

#ifdef WIN32
#else
#include <unistd.h>
#endif

static bool g_isToDetectTime = false;
static Mutex g_isToDetectTime_Mutex;

void test_print()
{	
	printf("hello tianwei\n");
}

/*
 *获取当前的时间字符串 
 *时间字符串的格式: 2014-01-19 18:52
*/
bool tw_time_localNowTimeStr_s(char *nowTime,const int timeStrSize)
{
	bool bRet=true;
	time_t t;
	struct tm tm;
	struct tm *pTM;

	t=time(NULL);
	pTM=localtime(&t);
	if(!pTM){
		bRet=false;
		goto errOut;
	}
	memcpy(&tm,pTM,sizeof(struct tm));

//	strftime(nowTime,timeStrSize,"%d-%b-%Y %H:%M", &tm);
	strftime(nowTime,timeStrSize,"%Y-%m-%d %H:%M:%S", &tm);
errOut:
	return bRet;

}

/*
 *函数功能:从时间字符串中提取日期字符串
 *时间字符串的格式: 2014-01-19 17:41
 *日期字符串的格式: 2014-01-19
*/
bool tw_time_getDate(const char* timeStr,char* dateStr,int dateStrSize)
{
	bool bRet=true;
	char *p=NULL;
	p=strchr((char*)timeStr,' ');
	if(!p){
		bRet=false;
		goto errOut;
	}
	if(dateStrSize < (p-timeStr+1)){
		bRet=false;
		goto errOut;
	}
	strncpy(dateStr,timeStr,p-timeStr);
errOut:
	return bRet;
}

/*
 *函数功能: 从时间字符串中提取小时
 *输入的时间的格式: 2014-01-19 17:41
*/
bool tw_time_getHour(const char* timeStr, int &hour)
{
	bool bRet = true;
	char hourBuf[156] = {0};
	char *p = NULL;
	char *q = NULL;
	char split = ' ';

	split = ' ';
	p=strchr((char*)timeStr,split);
	while(p&&*p==split) ++p;
	if(!p){
		bRet=false;
		goto errOut;
	}
	q=strchr(p,':');
	if(!q){
		bRet=false;
		goto errOut;
	}
	strncpy(hourBuf,p,q-p);
	hour=atoi(hourBuf);
errOut:
	return bRet;
}

/*
*函数功能: 去除_name两边的空格
*/
bool tw_strip_junk(char *_name)
{
	bool bRet= true;
	char *start=NULL, *end=NULL;
	char *p=NULL, *q=NULL;
	start=_name;
	end=_name + strlen(_name)/sizeof(char) -1;
	while((start <= end) && !(*start!=' ' && *end !=' '))
	{
		if( *start == ' ')
		{
			start++;
		}

		if( start > end)
		{
			break;
		}
		if( *end == ' ')
		{
			end--;
		}
	}
	if(start > end){
		bRet=false;
		goto errOut;
	}
	*(end + 1)=0;

	
	p=_name;
	q=start;
	for(p=_name,q=start; *q != 0; p++, q++) *p = *q;
	*p=0;

errOut:
	return bRet;
}

bool tw_isToRecoverTime(RedologINI &configINI)
{
	bool bRet = true;
	bool bNowDateBig = false;

	char timeList[256] = {0};
	char nowTime[256] = {0};
	char nowDate[156] = {0};
	char lastTime[256] = {0};
	char lastDate[156] = {0};
	char hourBuf[156]={0};
	vector<int> vecTime;
	char *p;
	char *q;
	char split;
	int hour;
	int lastHour;
	int i;

	int nowHour;
	char nowHourStr[156] = {0};
	const char *timeRecoverSection = "TIMING_RECOVER";

	vecTime.clear();
	configINI.LoadValuse(timeRecoverSection,"LAST_TIME",lastTime);
	tw_strip_junk(lastTime);
	bRet=tw_time_localNowTimeStr_s(nowTime,sizeof(nowTime));
	if(!bRet){
		goto errOut;
	}
	tw_strip_junk(nowTime);
	// 比较上次恢复的日期 和现在的日期
	tw_time_getDate(lastTime,lastDate,sizeof(lastDate));
	tw_time_getDate(nowTime,nowDate,sizeof(nowDate));
	i=strcmp(nowDate,lastDate);
	if(i>0){//现在的日期大于上次恢复的日期
		bNowDateBig=true;
	}else{
		bNowDateBig = false;
	}

	//提取现在时间的小时
	bRet=tw_time_getHour(nowTime,nowHour);
	if(!bRet){
		goto errOut;
	}
	//提取上次恢复时间的小时
	bRet=tw_time_getHour(lastTime,lastHour);
	if(!bRet){
		goto errOut;
	}


	//加载所有恢复时间点
	configINI.LoadValuse(timeRecoverSection,"TIME_LIST",timeList);
	p=strchr(timeList,'[');
	if(!p){
		bRet=false;
		goto errOut;
	}
	++p;
	do{
		q=strchr(p,',');
		if(!q){
			break;
		}
		strncpy(hourBuf,p,q-p);
		tw_strip_junk(hourBuf);
		hour=atoi(hourBuf);
		vecTime.push_back(hour);
		++q;
		p=q;
	}while(true);
	q=strchr(timeList,']');
	if(!q){
		bRet = false;
		goto errOut;
	}
	strncpy(hourBuf,p,q-p);
	tw_strip_junk(hourBuf);
	hour=atoi(hourBuf);
	vecTime.push_back(hour);
	
	vecTime.push_back(24);
	
	//对恢复时间点列表进行排序
	sort(vecTime.begin(),vecTime.end());


	//比较现在小时点在恢复时间点列表中的区间
	for(i=0;i<vecTime.size();i++){
		hour=vecTime[i];
		if(nowHour >= hour){
			if(i+1<vecTime.size()){
				hour=vecTime[i+1];
				if(nowHour<hour){
					break;
				}
			}else if(i+1>=vecTime.size()){
				break;
			}
			
		}
		
	}
	if(i==vecTime.size()){//恢复时间点没到
		bRet= false;
		goto errOut;
	}
	if(bNowDateBig){//恢复时间点到了
		bRet=true;
		goto errOut;
	}
	if(lastHour < vecTime[i]){ //恢复时间点到
		bRet=true;
		goto errOut;
	}else{                // 恢复时间还没到
		bRet = false;
	}
errOut:
	return bRet;
}
/*
bool tw_isToRecoverTime(RedologINI &configINI)
{
	bool bRet = true;
	bool bNowDateBig = false;

	char timeList[256] = {0};
	char nowTime[256] = {0};
	char nowDate[156] = {0};
	char lastTime[256] = {0};
	char lastDate[156] = {0};
	char hourBuf[156]={0};
	vector<int> vecTime;
	char *p;
	char *q;
	char split;
	int hour;
	int lastHour;
	int i;

	int nowHour;
	char nowHourStr[156] = {0};
	const char *timeRecoverSection = "TIMING_RECOVER";

	vecTime.clear();
	configINI.LoadValuse(timeRecoverSection,"LAST_TIME",lastTime);
	tw_strip_junk(lastTime);
	bRet=tw_time_localNowTimeStr_s(nowTime,sizeof(nowTime));
	if(!bRet){
		goto errOut;
	}
	tw_strip_junk(nowTime);
	// 比较上次恢复的日期 和现在的日期
	tw_time_getDate(lastTime,lastDate,sizeof(lastDate));
	tw_time_getDate(nowTime,nowDate,sizeof(nowDate));
	i=strcmp(nowDate,lastDate);
	if(i>0){//现在的日期大于上次恢复的日期
		bNowDateBig=true;
	}else{
		bNowDateBig = false;
	}

	//提取现在时间的小时
	bRet=tw_time_getHour(nowTime,nowHour);
	if(!bRet){
		goto errOut;
	}
	//提取上次恢复时间的小时
	bRet=tw_time_getHour(lastTime,lastHour);
	if(!bRet){
		goto errOut;
	}


	//加载所有恢复时间点
	configINI.LoadValuse(timeRecoverSection,"TIME_LIST",timeList);
	p=strchr(timeList,'[');
	if(!p){
		bRet=false;
		goto errOut;
	}
	do{
		q=strchr(p,',');
		if(!q){
			break;
		}
		++q;
		strncpy(hourBuf,p,q-p);
		tw_strip_junk(hourBuf);
		hour=atoi(hourBuf);
		vecTime.push_back(hour);
		p=q;
	}while(true);
	q=strchr(timeList,']');
	if(!q){
		bRet = false;
		goto errOut;
	}
	++p;
	strncpy(hourBuf,p,q-p);
	tw_strip_junk(hourBuf);
	hour=atoi(hourBuf);
	vecTime.push_back(hour);

	//比较现在小时点在恢复时间点列表中的区间
	for(i=0;i<vecTime.size();i++){
		hour=vecTime[i];
		if(nowHour >= hour){
			if(i+1<vecTime.size()){
				hour=vecTime[i+1];
				if(nowHour<hour){
					break;
				}
			}else if(i+1>=vecTime.size()){
				break;
			}
			
		}
		
	}
	if(i==vecTime.size()){//恢复时间点没到
		bRet= false;
		goto errOut;
	}
	if(bNowDateBig){//恢复时间点到了
		bRet=true;
		goto errOut;
	}
	if(lastHour < vecTime[i]){ //恢复时间点到
		bRet=true;
		goto errOut;
	}else{                // 恢复时间还没到
		bRet = false;
	}
errOut:
	return bRet;
}
*/


bool tw_rcvResultWRTOConfig(RedologINI &configIni,const bool &recoverSuccess)
{
	bool bRet = true;
	char nowTimeStr[256] = {0};
	const char *TimingRecSec="TIMING_RECOVER";
	if(!recoverSuccess){
		goto errOut;
	}
	bRet=tw_time_localNowTimeStr_s(nowTimeStr,256);
	if(!bRet){
		goto errOut;
	}
	configIni.SaveValuse(TimingRecSec,"LAST_TIME",nowTimeStr);
errOut:
	return bRet;
}

int t_debug(int level, char *file, long line, char *fmd,...)
{
	FILE *fp;
	char fileName[256]={0};
	char errMsg[256]={0};
	char tmStamp[128]={0};

	static unsigned long sequence=0;
	
	if(level < 0){//不输出日志信息
		return 0;
	}

	snprintf(fileName,sizeof(fileName), "%s%d%s", "/usr/NETPRO5.02.8/data/t_debug", level, ".txt");


	static bool bFirst = true;
	const unsigned long maxFileSize=10*1024*1024;
	long curPos;
	

	
	if(bFirst){
		fp=fopen(fileName,"w+");
		bFirst=false;
	}else{
		fp=fopen(fileName,"a+");
	}

	if(fp == NULL)
	{
		snprintf(errMsg,sizeof(errMsg),"%s\n",strerror(errno));
		exit(-1);
		return -1;
	}

	//检查文件大小
	//查询文件大小
	curPos=ftell(fp);
	curPos=fseek(fp,0,SEEK_END);
	curPos=ftell(fp);
	//检查文件是否过大
	if(curPos > maxFileSize){//文件过大，则清空重写
		//清空文件
		fclose(fp);
		fp=fopen(fileName,"w");
		if(!fp){
			snprintf(errMsg,sizeof(errMsg),"%s\n",strerror(errno));
			tw_exit(-1);
			return -1;
		}
	}

	tw_time_localNowTimeStr_s(tmStamp,128);

	va_list args;
	int num=0;
	char *buf=(char *)malloc(1024);
	if(NULL == buf)
	{
		fclose(fp);
		return -1;
	}
	memset(buf,0,1024);
	//sprintf(buf,"%s, %u %s: ",file,line,tmStamp);
	sprintf(buf,"%s, %u %s %u: ",file,line,tmStamp,sequence++);
	
	va_start(args,fmd);
	vsprintf(buf+strlen(buf),fmd,args);
	num=strlen(buf);

	va_end(args);
	if( num != fwrite(buf,1,num,fp) )
	{
		fflush(fp);
		fflush(fp);
		fclose(fp);
		free(buf);
		exit(1);
		return -1;
	}

	fflush(fp);
	fflush(fp);
	fclose(fp);
	free(buf);
	return 0;
}

unsigned int tw_sleep(unsigned int seconds)
{
	unsigned int uiRet=0;
	unsigned long ul;
#ifdef WIN32
	ul=Sleep(1000*secondes);
	uiRet=ul/1000;
#else
	uiRet=sleep(seconds);
#endif

	return uiRet;
}


// 内存对齐函数
void *ptr_align (void const *ptr, size_t alignment)
{
#ifdef _DEBUG_ON_ASM
	cout<<"----------ptr_align"<<endl;
#endif
  char const *p0 = (char const *)ptr;
  char const *p1 = p0 + alignment - 1;
  return (void *) (p1 - (size_t) p1 % alignment);
}


/*
*函数功能: 设置主库探活时间是否到达
*输入: toDetectTime -- 主库探活时间是否到达
*/

void tw_setToDetectTime(bool toDetectTime)
{
	g_isToDetectTime_Mutex.Lock();
	g_isToDetectTime = toDetectTime;
	g_isToDetectTime_Mutex.Unlock();
	return;
}

/*
*函数功能: 设置主库探活时间是否到达
*/

bool tw_isToDetectTimeByDrct()
{
	bool bRet = false;
	g_isToDetectTime_Mutex.Lock();
	bRet=g_isToDetectTime;
	g_isToDetectTime_Mutex.Unlock();

	return bRet;
}

/*
*函数功能: 探测主库是否还在正常运行的时间点 是否到达
*输入: configINI -- 用于读写参数文件
*返回: true -- 时间点到了 false -- 时间点没到
*/
bool tw_isToDetectTime(RedologINI &configINI)
{
	bool bRet = true;
	bool bNowDateBig = false;

	char timeList[256] = {0};
	char nowTime[256] = {0};
	char nowDate[156] = {0};
	char lastTime[256] = {0};
	char lastDate[156] = {0};
	char hourBuf[156]={0};
	vector<int> vecTime;
	char *p;
	char *q;
	char split;
	int hour;
	int lastHour;
	int i;

	int nowHour;
	char nowHourStr[156] = {0};
	const char *timeRecoverSection = "TIMING_DETECT";

	vecTime.clear();
	configINI.LoadValuse(timeRecoverSection,"LAST_TIME",lastTime);
	tw_strip_junk(lastTime);
	bRet=tw_time_localNowTimeStr_s(nowTime,sizeof(nowTime));
	if(!bRet){
		goto errOut;
	}
	tw_strip_junk(nowTime);
	// 比较上次恢复的日期 和现在的日期
	tw_time_getDate(lastTime,lastDate,sizeof(lastDate));
	tw_time_getDate(nowTime,nowDate,sizeof(nowDate));
	i=strcmp(nowDate,lastTime);
	if(i>0){//现在的日期大于上次恢复的日期
		bNowDateBig=true;
	}else{
		bNowDateBig = false;
	}

	//提取现在时间的小时
	bRet=tw_time_getHour(nowTime,nowHour);
	if(!bRet){
		goto errOut;
	}
	//提取上次恢复时间的小时
	bRet=tw_time_getHour(lastTime,lastHour);
	if(!bRet){
		goto errOut;
	}


	//加载所有恢复时间点
	configINI.LoadValuse(timeRecoverSection,"TIME_LIST",timeList);
	p=strchr(timeList,'[');
	if(!p){
		bRet=false;
		goto errOut;
	}
	++p;
	do{
		q=strchr(p,',');
		if(!q){
			break;
		}
		strncpy(hourBuf,p,q-p);
		tw_strip_junk(hourBuf);
		hour=atoi(hourBuf);
		vecTime.push_back(hour);
		++q;
		p=q;
	}while(true);
	q=strchr(timeList,']');
	if(!q){
		bRet = false;
		goto errOut;
	}
	strncpy(hourBuf,p,q-p);
	tw_strip_junk(hourBuf);
	hour=atoi(hourBuf);
	vecTime.push_back(hour);

	vecTime.push_back(24);

	//对恢复时间点列表进行排序
	sort(vecTime.begin(),vecTime.end());


	//比较现在小时点在恢复时间点列表中的区间
	for(i=0;i<vecTime.size();i++){
		hour=vecTime[i];
		if(nowHour >= hour){
			if(i+1<vecTime.size()){
				hour=vecTime[i+1];
				if(nowHour<hour){
					break;
				}
			}else if(i+1>=vecTime.size()){
				break;
			}

		}

	}
	if(i==vecTime.size()){//恢复时间点没到
		bRet= false;
		goto errOut;
	}
	if(bNowDateBig){//恢复时间点到了
		bRet=true;
		goto errOut;
	}
	if(lastHour < vecTime[i]){ //恢复时间点到
		bRet=true;
		goto errOut;
	}else{                // 恢复时间还没到
		bRet = false;
	}
errOut:
	return bRet;
}

/*
*函数功能: 探测主库是否还在正常运行的时间点 到了之后执行的操作的结果写到
*输入: sucess -- 执行的操作的结果
*输出: configIni -- 用于写配置文件
*返回: true -- 暂时全为true
*/
bool tw_detectResultWRTOConfig(RedologINI &configIni,const bool &success)
{
	bool bRet = true;
	char nowTimeStr[256] = {0};
	const char *TimingRecSec="TIMING_DETECT";
	
	if(!success){
		goto errOut;
	}
	t_wnMsg("tw_detectResultWRTOConfig\n");
	bRet=tw_time_localNowTimeStr_s(nowTimeStr,256);
	if(!bRet){
		goto errOut;
	}
	configIni.SaveValuse(TimingRecSec,"LAST_TIME",nowTimeStr);
errOut:
	return bRet;
}

