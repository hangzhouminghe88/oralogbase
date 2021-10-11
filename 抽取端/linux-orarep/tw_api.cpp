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
 *��ȡ��ǰ��ʱ���ַ��� 
 *ʱ���ַ����ĸ�ʽ: 2014-01-19 18:52
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
 *��������:��ʱ���ַ�������ȡ�����ַ���
 *ʱ���ַ����ĸ�ʽ: 2014-01-19 17:41
 *�����ַ����ĸ�ʽ: 2014-01-19
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
 *��������: ��ʱ���ַ�������ȡСʱ
 *�����ʱ��ĸ�ʽ: 2014-01-19 17:41
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
*��������: ȥ��_name���ߵĿո�
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
	// �Ƚ��ϴλָ������� �����ڵ�����
	tw_time_getDate(lastTime,lastDate,sizeof(lastDate));
	tw_time_getDate(nowTime,nowDate,sizeof(nowDate));
	i=strcmp(nowDate,lastDate);
	if(i>0){//���ڵ����ڴ����ϴλָ�������
		bNowDateBig=true;
	}else{
		bNowDateBig = false;
	}

	//��ȡ����ʱ���Сʱ
	bRet=tw_time_getHour(nowTime,nowHour);
	if(!bRet){
		goto errOut;
	}
	//��ȡ�ϴλָ�ʱ���Сʱ
	bRet=tw_time_getHour(lastTime,lastHour);
	if(!bRet){
		goto errOut;
	}


	//�������лָ�ʱ���
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
	
	//�Իָ�ʱ����б��������
	sort(vecTime.begin(),vecTime.end());


	//�Ƚ�����Сʱ���ڻָ�ʱ����б��е�����
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
	if(i==vecTime.size()){//�ָ�ʱ���û��
		bRet= false;
		goto errOut;
	}
	if(bNowDateBig){//�ָ�ʱ��㵽��
		bRet=true;
		goto errOut;
	}
	if(lastHour < vecTime[i]){ //�ָ�ʱ��㵽
		bRet=true;
		goto errOut;
	}else{                // �ָ�ʱ�仹û��
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
	// �Ƚ��ϴλָ������� �����ڵ�����
	tw_time_getDate(lastTime,lastDate,sizeof(lastDate));
	tw_time_getDate(nowTime,nowDate,sizeof(nowDate));
	i=strcmp(nowDate,lastDate);
	if(i>0){//���ڵ����ڴ����ϴλָ�������
		bNowDateBig=true;
	}else{
		bNowDateBig = false;
	}

	//��ȡ����ʱ���Сʱ
	bRet=tw_time_getHour(nowTime,nowHour);
	if(!bRet){
		goto errOut;
	}
	//��ȡ�ϴλָ�ʱ���Сʱ
	bRet=tw_time_getHour(lastTime,lastHour);
	if(!bRet){
		goto errOut;
	}


	//�������лָ�ʱ���
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

	//�Ƚ�����Сʱ���ڻָ�ʱ����б��е�����
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
	if(i==vecTime.size()){//�ָ�ʱ���û��
		bRet= false;
		goto errOut;
	}
	if(bNowDateBig){//�ָ�ʱ��㵽��
		bRet=true;
		goto errOut;
	}
	if(lastHour < vecTime[i]){ //�ָ�ʱ��㵽
		bRet=true;
		goto errOut;
	}else{                // �ָ�ʱ�仹û��
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
	
	if(level < 0){//�������־��Ϣ
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

	//����ļ���С
	//��ѯ�ļ���С
	curPos=ftell(fp);
	curPos=fseek(fp,0,SEEK_END);
	curPos=ftell(fp);
	//����ļ��Ƿ����
	if(curPos > maxFileSize){//�ļ������������д
		//����ļ�
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


// �ڴ���뺯��
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
*��������: ��������̽��ʱ���Ƿ񵽴�
*����: toDetectTime -- ����̽��ʱ���Ƿ񵽴�
*/

void tw_setToDetectTime(bool toDetectTime)
{
	g_isToDetectTime_Mutex.Lock();
	g_isToDetectTime = toDetectTime;
	g_isToDetectTime_Mutex.Unlock();
	return;
}

/*
*��������: ��������̽��ʱ���Ƿ񵽴�
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
*��������: ̽�������Ƿ����������е�ʱ��� �Ƿ񵽴�
*����: configINI -- ���ڶ�д�����ļ�
*����: true -- ʱ��㵽�� false -- ʱ���û��
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
	// �Ƚ��ϴλָ������� �����ڵ�����
	tw_time_getDate(lastTime,lastDate,sizeof(lastDate));
	tw_time_getDate(nowTime,nowDate,sizeof(nowDate));
	i=strcmp(nowDate,lastTime);
	if(i>0){//���ڵ����ڴ����ϴλָ�������
		bNowDateBig=true;
	}else{
		bNowDateBig = false;
	}

	//��ȡ����ʱ���Сʱ
	bRet=tw_time_getHour(nowTime,nowHour);
	if(!bRet){
		goto errOut;
	}
	//��ȡ�ϴλָ�ʱ���Сʱ
	bRet=tw_time_getHour(lastTime,lastHour);
	if(!bRet){
		goto errOut;
	}


	//�������лָ�ʱ���
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

	//�Իָ�ʱ����б��������
	sort(vecTime.begin(),vecTime.end());


	//�Ƚ�����Сʱ���ڻָ�ʱ����б��е�����
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
	if(i==vecTime.size()){//�ָ�ʱ���û��
		bRet= false;
		goto errOut;
	}
	if(bNowDateBig){//�ָ�ʱ��㵽��
		bRet=true;
		goto errOut;
	}
	if(lastHour < vecTime[i]){ //�ָ�ʱ��㵽
		bRet=true;
		goto errOut;
	}else{                // �ָ�ʱ�仹û��
		bRet = false;
	}
errOut:
	return bRet;
}

/*
*��������: ̽�������Ƿ����������е�ʱ��� ����֮��ִ�еĲ����Ľ��д��
*����: sucess -- ִ�еĲ����Ľ��
*���: configIni -- ����д�����ļ�
*����: true -- ��ʱȫΪtrue
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

