#ifndef _TW_API_H 
#define _TW_API_H 

#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<string.h>
#include<errno.h>
#include<stdarg.h>
#include<vector>


using namespace std;

class RedologINI;

void test_print();

bool tw_time_localNowTimeStr_s(char *nowTime,const int timeStrSize);
bool tw_time_getDate(const char* timeStr,char* dateStr,int dateStrSize);
bool tw_time_getHour(const char* timeStr, int &hour);
bool tw_strip_junk(char *_name);

void tw_setToDetectTime(bool toDetectTime);
bool tw_isToDetectTimeByDrct();


// ÄÚ´æ¶ÔÆëº¯Êý
void *ptr_align (void const *ptr, size_t alignment);

unsigned int tw_sleep(unsigned int seconds);

bool tw_isToRecoverTime(RedologINI &configINI);
bool tw_rcvResultWRTOConfig(RedologINI &configIni,const bool &recoverSuccess);

bool tw_detectResultWRTOConfig(RedologINI &configIni,const bool &success);
bool tw_isToDetectTime(RedologINI &configINI);

#define tw_exit(n) exit(n)

int t_debug(int level, char *file, long line, char *fmd,...);

#if 1
#define t_debugMsg(level,fmd,...) t_debug(level,__FILE__, __LINE__,fmd, ##__VA_ARGS__)
//#define t_errMsg(fmd,...) t_debug(999,__FILE__,__LINE__,fmd,##__VA_ARGS__)
#else
#define t_debugMsg(level,fmd,...)
//#define t_errMsg(fmd,...)
#endif

#define t_errMsg(fmd,...) t_debug(999,__FILE__,__LINE__,fmd,##__VA_ARGS__)
#define t_wnMsg(fmd,...) t_debug(888,__FILE__,__LINE__,fmd,##__VA_ARGS__)

#define tw_exit(n) exit(n)

#define t_dbgLvl 182
#define t_addExpandLvl 185
#define t_redoLogFileLvl 186
#define t_rdDtFileLvl 192
#define t_timingRecLvl 185

#define t_rwDataBlksLvl -198
#define t_rwDataBlksLvl2 -199

#define t_dbglvl -182
#define t_dbglvl2 -183
#define t_dbglvl3 -184
#define t_tmpLvl -197
#define t_dbgChangesLvl -186
#define t_analyseThreadLvl -187
#define t_logRecordLvl -188
#define t_getRedoNodeLvl -190

#endif

