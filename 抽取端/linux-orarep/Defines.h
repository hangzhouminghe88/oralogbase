//////////////////////////////////////////////////////////////////////////
//  [5/12/2012 wqy]  ����һЩ���õ�
//////////////////////////////////////////////////////////////////////////

#ifndef _DEFINES_H
#define _DEFINES_H

#ifndef _ORACLE_64
#define _ORACLE_64
#endif

enum _Version{
	_Oracle_Old,
	_Oracle_8i,
	_Oracle_9i,
	_Oracle_10g,
	_Oracle_11g,
};

enum {
	_INACTIVE,
	_ACTIVE,
	_CURRENT,
	_UNKNOW,
};
enum {
	_REP_STAT_RUN	= 0x01,
	_REP_STAT_PAUSE	= 0x02,
	_REP_STAT_END	= 0x04,
};

enum TW_FILE_TYPE 
{
	FILE_UNKOWN_TYPE         = 0,
	FILE_ARCHIVE_TYPE        = 1,
	FILE_DATA_TYPE           = 2,
	FILE_ONLINE_CURRENT_TYPE = 3,
	FILE_ONLINE_ACTIVE_TYPE  = 4,
	FILE_ONLINE_DEACTIVE_TYPE = 5
	
};

// BLOCKGAP �ж��ٿ�����ʱ�����ڴ�
//#define BLOCKGAP	1000
#define BLOCKGAP	5000

// TIMEWAIT: ��ǰ�����ļ���βʱ�ȴ�ʱ��
#define TIMEWAIT	1000

// ��ʱ���ȴ�ʱ�估����
#define TIMER_TIME	200
#define TIMER_COUNT 5
#define MAX_NAME_LENGTH 512
#define QUERY_LEN 128
#define MAXGROUP 10
#define MAXDISK 10

//#define WITHASM

// AU��С
//#define AUSIZE 4194304
//#define AUSIZE 1048576

// needed below one if program run on AIX
//#define _ORACLE_AIX_64
// ������Windows�ϵ��Դ�AIX���������Ĺ鵵�ļ�
// ����������AIX�ļ���windows���ڴ治��Ӧ
//#define _BYTECHANGE_ON_

//#define  _DEBUG_ON
//#define _BLOCK_DEBUG_Change
//#define _BLOCK_DEBUG_MEM
//#define _DEBUG_QUERY_ON
//#define _DEBUG_ON_ASM_LOG
// ASM����
//#define	_DEBUG_ON_ASM_HIT
//#define _DEBUG_ON_ASM
//#define _DEBUG_ON_ASM_BLOCK
//#define _BLOCK_DEBUG_ASM
//#define _DEBUG_ON_ASM_NEW
//#define _BLOCK_DEBUG_Change_ASM
//#define _DEBUG_QUERY_ON_2
//#define _DEUBG_SCRIPT
//#define _DEBUG_LoadandSkipLogHead

// ���Ʊ����Ƿ�д���ݿ� add by tw 20140920
#define NOT_WRITE

#define MODIFY_SCN_SEQ_OFF
#define ORACLE_RAC

// oracle 10g
#define ORACLE_VERSION _Oracle_10g
// oracle 11g
//#define ORACLE_VERSION _Oracle_11g

//#define _ORACLE_9I

#ifdef _WIN32
	#include <WTypes.h>
//	#include <tchar.h>
	#include <windows.h> 
	#define fdsleep(x) Sleep(x)
	#define MyCreateThread(ThreadID,Thread,Values) CreateThread(NULL,0,(unsigned long (__stdcall *)(void*))Thread,(LPVOID)Values,0,ThreadID)
	#define ReturnType DWORD WINAPI
	#define ThreadID unsigned long
	#define _fseek _fseeki64
typedef unsigned __int64 UBYTE8;
typedef __int64 BYTE8;
typedef unsigned char BYTE;
typedef unsigned short int WORD;

typedef unsigned __int64 UBYTE8;
	#include <windef.h>
	#define _CONFIG_FILE	".\\conf\\Redolog.ini"
#else
	#define _CONFIG_FILE	"../conf/Redolog.ini"
	#define fdsleep(x) usleep((x)*1000)
	#define MyCreateThread(ThreadID,Thread,Values) pthread_create((ThreadID),NULL,Thread,Values)
	#define LPVOID void
	#define ReturnType void*
	#define HANDLE int
	#define ThreadID pthread_t 
	#define GetCurrentThreadId pthread_self
	#define _fseek fseek

	#define myopen open
	#define myread read
	#define mywrite write
	#define mylseek lseek64
	#define myclose close
	#define MAX_PATH 260

typedef unsigned char BYTE;
typedef unsigned short int WORD;
typedef unsigned int DWORD;
typedef unsigned long long BYTE8;

typedef unsigned long long UBYTE8;

#endif

#endif
