
#ifndef _OCIQUERY_H_
#define _OCIQUERY_H_

#include <string>
#include <vector>
#include "oracleoci.h"
#include "Defines.h"
//#include "ASM.h"
#include "tw_ASM.h"
//#include "lgc_define.h"
using namespace std;

// 数据库操作相关都放这里
//select b.sequence#,b.status,b.first_change#,a.member from v$logfile a ,v$log b where a.group#=b.group# order by b.sequence#
#define SELECT_REDOLOG "select b.sequence#,b.status,b.first_change#,a.member from v$logfile a ,v$log b where a.group#=b.group# order by b.FIRST_CHANGE# "
#define SELECT_REDOLOG_CURRENT "select b.sequence#,b.status,b.first_change#,a.member from v$logfile a ,v$log b where a.group#=b.group# and b.status='CURRENT' order by b.FIRST_CHANGE# "

#define SELECT_REDOLOG_BY "select b.sequence#,b.status,b.first_change#,a.member from v$logfile a ,v$log b where a.group#=b.group# and b.status='%s' and b.first_change#>%d  order by b.FIRST_CHANGE#"

#define SELECT_LOG "select sequence#,first_change#,name from v$archived_log"

// 因RAC需要，添加a.THREAD#项
#define SELECT_LOG_SCN_DATA "select a.THREAD#,a.SEQUENCE#,a.FIRST_CHANGE#,a.name from v$archived_log a,v$log b where a.SEQUENCE#=b.SEQUENCE#(+) and STANDBY_DEST='NO' and a.FIRST_CHANGE#>%ld and a.first_time > to_date('%s','yyyy-MM-dd') and (b.STATUS='INACTIVE' or b.status is null) order by FIRST_CHANGE# "

#ifdef ORACLE_RAC
	#define SELECT_LOG_SCN "select a.THREAD#,a.SEQUENCE#,a.FIRST_CHANGE#,a.NEXT_CHANGE#,a.name from v$archived_log a,v$log b where a.SEQUENCE#=b.SEQUENCE#(+) and STANDBY_DEST='NO' and ((a.FIRST_CHANGE#>%ld and a.THREAD#=1) or (a.FIRST_CHANGE#>%ld and a.THREAD#=2)) and (b.STATUS='INACTIVE' or b.status is null) order by FIRST_CHANGE# "
	#define SELECT_LOG_SCN2 "select a.THREAD#,a.SEQUENCE#,a.FIRST_CHANGE#,a.NEXT_CHANGE#,a.name from v$archived_log a,v$log b where a.SEQUENCE#=b.SEQUENCE#(+) and STANDBY_DEST='NO' and ((a.FIRST_CHANGE#>:1 and a.THREAD#=1) or (a.FIRST_CHANGE#>:2 and a.THREAD#=2)) and (b.STATUS='INACTIVE' or b.status is null) order by FIRST_CHANGE# "
#else
	#define SELECT_LOG_SCN "select a.THREAD#,a.SEQUENCE#,a.FIRST_CHANGE#,a.NEXT_CHANGE#,a.name from v$archived_log a,v$log b where a.SEQUENCE#=b.SEQUENCE#(+) and STANDBY_DEST='NO' and a.FIRST_CHANGE#>%ld and (b.STATUS='INACTIVE' or b.status is null) order by FIRST_CHANGE# "
	#define SELECT_LOG_SCN2 "select a.THREAD#,a.SEQUENCE#,a.FIRST_CHANGE#,a.NEXT_CHANGE#,a.name from v$archived_log a,v$log b where a.SEQUENCE#=b.SEQUENCE#(+) and STANDBY_DEST='NO' and a.FIRST_CHANGE#> :1 and (b.STATUS='INACTIVE' or b.status is null) order by FIRST_CHANGE# "
#endif

#define SELECT_ARGLOG "select name from v$archived_log where FIRST_CHANGE#=%ld and STANDBY_DEST='NO'"
#define SELECT_ARGLOG_SEQ "select FIRST_CHANGE#,name from v$archived_log where sequence#=%ld and STANDBY_DEST='NO'"

#define SELECT_DATAFILE "select file_name from dba_data_files where file_id=%d"
#define SELECT_DATAFILE_ALL "select file_name from dba_data_files"

#define FLASH_CACHE "alter system flush buffer_cache"
#define FLASH_SHARED_POOL "alter system flush shared_pool"
//modify b.space  to b.bytes
#define SELECT_FILEINFO "select a.FILE_NUMBER,b.BLOCK_SIZE,b.bytes,b.BLOCKS from V$ASM_ALIAS a, v$asm_file b where a.group_number=b.group_number and a.FILE_NUMBER=b.FILE_NUMBER and upper(a.NAME)=upper('%s') "
#define SELECT_FILEINFO2 "select a.FILE_NUMBER,b.BLOCK_SIZE,b.bytes,b.BLOCKS from V$ASM_ALIAS a, v$asm_file b where a.group_number=b.group_number and a.FILE_NUMBER=b.FILE_NUMBER and upper(a.NAME)=upper(:1) and a.group_number=:2 "
#define SELECT_GROUP_NUM "select group_number,block_size,allocation_unit_size from v$asm_diskgroup where upper(name)=upper('%s') "
#define SELECT_DISK_INFO "select group_number,disk_number, path from v$asm_disk where group_number=%d order by disk_number "
#define SELECT_AU_INFO "select DISK_KFFXP ,AU_KFFXP from X$KFFXP where GROUP_KFFXP=%d and NUMBER_KFFXP=%d and XNUM_KFFXP=%d "
#define SELECT_AU_INFO2 "select DISK_KFFXP ,AU_KFFXP from X$KFFXP where GROUP_KFFXP=:1 and NUMBER_KFFXP=:2 and XNUM_KFFXP=:3 "

#define SELECT_ALL_GROUP "select group_number,block_size,allocation_unit_size,name from v$asm_diskgroup "
#define SELECT_ALL_DISKS "select group_number,disk_number,path from v$asm_disk order by disk_number "

//#define SELECT_SOURCEDATA "select DISK_KFFXP,AU_KFFXP  from x$kffxp where GROUP_KFFXP=%d and NUMBER_KFFXP=1 and rownum=1 "

#define STANDBYFILE_MANUAL	"alter system set standby_file_management=MANUAL "
#define STANDBYFILE_ADDFILE "alter database create datafile %d as '%s' "
#define STANDBYFILE_RESIZE	"alter database datafile %d resize %ld "
#define STANDBYFILE_AUTO	"alter system set standby_file_management=auto "
#define SELECT_DATAFILE_INFO "select file#,name,creation_change#,bytes,blocks,block_size from v$datafile "
#define SELECT_NEW_DATAFILE "select name from v$datafile where file#=%d "
#define SELECT_NEW_DATAFILE_RAC "select file# from v$datafile where file#=%d "
#define GET_CONTROL_FILE "alter database create standby controlfile as 'control01.ctl' reuse "
#define GETFILESIZE	"select bytes from v$datafile WHERE file#=%d "

#define SELECT_GETSTAT "select database_role,open_mode from v$database"
#define SELECT_SYNC_STAT "select * from(select sequence#,applied from v$archived_log where standby_dest='NO' and deleted='NO' order by first_change# desc) where rownum<2 "

#define SELECT_ANALYSE_SCN "select FIRST_CHANGE# from v$archived_log where STANDBY_DEST='NO' and FIRST_CHANGE#>%ld "
#define SELECT_NEXT_CHANGE "select * from (select NEXT_CHANGE# from v$archived_log where STANDBY_DEST='NO'and FIRST_CHANGE#>%s order by FIRST_CHANGE# desc) where rownum<2 "
#define ALTER_DATABASE_OPEN "alter database open "
#define CREATE_DATAFILE "alter database create datafile %d as '%s' "
#define ALTER_LOG_DEFER "alter system set log_archive_dest_state_2=defer "
#define ALTER_LOG_DEFAULT "alter system set log_archive_dest_state_2=enable "
#define ALTER_SWITCH_LOGFILE "alter system switch logfile "
#define ALTER_SWITCH_CHECKPOINT "alter system checkpoint "

#define SLELCT_DAUL "select null from dual "

#define SELECT_TABLEMETA_INFO "SELECT a.object_id, a.owner, a.object_name, b.num_rows, c.COLUMN_ID, c.COLUMN_NAME, c.DATA_TYPE, c.DATA_LENGTH, c.NULLABLE FROM dba_objects a, dba_tables b, dba_tab_columns c where a.owner = b.owner AND b.owner = c.owner AND a.object_name = b.table_name AND b.table_name = c.table_name order by a.object_id,c.COLUMN_ID"
//#define SELECT_TABLEMETA_INFO_BYOWNER "SELECT a.object_id, a.owner, a.object_name, b.num_rows, c.COLUMN_ID, c.COLUMN_NAME, c.DATA_TYPE, c.DATA_LENGTH, c.NULLABLE FROM dba_objects a, dba_tables b, dba_tab_columns c where a.owner=upper('%s') AND a.owner = b.owner AND b.owner = c.owner AND a.object_name = b.table_name AND b.table_name = c.table_name order by a.object_id,c.COLUMN_ID"
#define SELECT_TABLEMETA_INFO_BYOWNER          "SELECT a.object_id, a.owner, a.object_name, c.COLUMN_ID, c.COLUMN_NAME, c.DATA_TYPE, c.DATA_LENGTH, c.NULLABLE FROM dba_objects a, dba_tables b, dba_tab_columns c where a.object_type in ('TABLE PARTITION','TABLE') AND a.owner=upper('%s') AND a.owner = b.owner AND b.owner = c.owner AND a.object_name = b.table_name AND b.table_name = c.table_name order by a.object_id, c.COLUMN_ID"
#define SELECT_TABLEMETA_INFO_BYOWNERTABLENAME "SELECT a.object_id, a.owner, a.object_name, c.COLUMN_ID, c.COLUMN_NAME, c.DATA_TYPE, c.DATA_LENGTH, c.NULLABLE FROM dba_objects a, dba_tables b, dba_tab_columns c where a.object_type in ('TABLE PARTITION','TABLE') AND a.owner=upper('%s') AND b.table_name=upper('%s') AND a.owner = b.owner AND b.owner = c.owner AND a.object_name = b.table_name AND b.table_name = c.table_name order by a.object_id, c.COLUMN_ID"
#define SELECT_TABLE_COLS "SELECT max(column_id) FROM dba_tab_columns WHERE owner=upper('%s') AND upper(table_name)=upper('%s')"
#define OCI_CONNECT_RETRYTIME 5
#define SELECT_ARCHIVE_FILE_LIST "select a.THREAD#,a.SEQUENCE#,a.FIRST_CHANGE#,a.NEXT_CHANGE#,a.name, a.BLOCK_SIZE, a.BLOCKS from v$archived_log a where a.STANDBY_DEST='NO' and a.ARCHIVED='YES' and a.DELETED ='NO' and (a.name is not null ) and a.THREAD#=%ld and ( (a.FIRST_CHANGE#<=%ld and a.NEXT_CHANGE#>%ld) or a.first_change#>=%ld) order by FIRST_CHANGE#"
#define LGC_SELECT_ARCH_FILE_INFO "select a.THREAD#,a.SEQUENCE#,a.FIRST_CHANGE#,a.NEXT_CHANGE#,a.name, a.BLOCK_SIZE, a.BLOCKS from v$archived_log a where a.STANDBY_DEST='NO' and a.ARCHIVED='YES' and a.DELETED ='NO' and (a.name is not null ) and a.THREAD#=%ld and a.SEQUENCE#=%ld order by FIRST_CHANGE#"
//#define SELECT_ONLINE_FILE_LIST  "select log.thread#, log.sequence#,log.first_change#,log.next_change#,logfile.member,log.blocksize,log.bytes/log.blocksize as blocks, log.status,log.archived  from  v$log log, v$logfile logfile where  log.group#=logfile.group# and logfile.type='ONLINE' and log.thread#=%ld and ((log.first_change#<=%ld and log.next_change#>%ld) or log.first_change#>=%ld) order by log.first_change#"
#define LGC_SELECT_ARCHIVE_FILE_LIST_BYSEQ "select a.THREAD#,a.SEQUENCE#,a.FIRST_CHANGE#,a.NEXT_CHANGE#,a.name, a.BLOCK_SIZE, a.BLOCKS from v$archived_log a where a.STANDBY_DEST='NO' and a.ARCHIVED='YES' and a.DELETED ='NO' and (a.name is not null ) and a.THREAD#=%ld and a.SEQUENCE#>=%ld order by FIRST_CHANGE#"
//#define LGC_SELECT_ONLINE_FILE_LIST_BYSEQ "select log.thread#, log.sequence#,log.first_change#,log.next_change#,logfile.member,log.blocksize,log.bytes/log.blocksize as blocks, log.status,log.archived  from  v$log log, v$logfile logfile where  log.group#=logfile.group# and logfile.type='ONLINE' and log.thread#=%ld and log.sequence#>=%ld order by log.first_change#"

#ifdef DEF_ORACLE_9I
#define SELECT_ONLINE_FILE_LIST "select log.thread#, log.sequence#,log.first_change#,281474976710655 as next_change#,logfile.member,512,log.bytes/512 as blocks, log.status,log.archived from  v$log log, v$logfile logfile where log.group#=logfile.group# and logfile.type='ONLINE' and log.thread#=%ld and (log.first_change#<=%ld ) order by log.first_change# desc"
#define LGC_SELECT_ONLINE_FILE_LIST_BYSEQ "select log.thread#, log.sequence#,log.first_change#,281474976710655 as next_change#,logfile.member,512,log.bytes/512 as blocks, log.status,log.archived from  v$log log, v$logfile logfile where  log.group#=logfile.group# and logfile.type='ONLINE' and log.thread#=%ld and log.sequence#>=%ld order by log.first_change#"
#define LGC_SELECT_ONLINE_FILE_INFO "select log.thread#, log.sequence#,log.first_change#,log.next_change#,logfile.member,log.blocksize,log.bytes/log.blocksize as blocks, log.status,log.archived  from  v$log log, v$logfile logfile where  log.group#=logfile.group# and logfile.type='ONLINE' and log.thread#=%ld and log.sequence#=%ld order by log.first_change#"
#else
#define SELECT_ONLINE_FILE_LIST "select log.thread#, log.sequence#,log.first_change#,281474976710655 as next_change#,logfile.member,512,log.bytes/512 as blocks, log.status,log.archived from  v$log log, v$logfile logfile where log.group#=logfile.group# and logfile.type='ONLINE' and logfile.IS_RECOVERY_DEST_FILE = 'NO' and log.thread#=%ld and (log.first_change#<=%ld ) order by log.first_change# desc"
#define LGC_SELECT_ONLINE_FILE_LIST_BYSEQ "select log.thread#, log.sequence#,log.first_change#,281474976710655 as next_change#,logfile.member,512,log.bytes/512 as blocks, log.status,log.archived from  v$log log, v$logfile logfile where  log.group#=logfile.group# and logfile.type='ONLINE' and logfile.IS_RECOVERY_DEST_FILE = 'NO'  and log.thread#=%ld and log.sequence#>=%ld order by log.first_change#"
#define LGC_SELECT_ONLINE_FILE_INFO "select log.thread#, log.sequence#,log.first_change#,log.next_change#,logfile.member,log.blocksize,log.bytes/log.blocksize as blocks, log.status,log.archived  from  v$log log, v$logfile logfile where  log.group#=logfile.group# and logfile.type='ONLINE' and log.thread#=%ld and log.sequence#=%ld order by log.first_change#"
#endif

#define LGC_Is_ARChIVED "select a.ARCHIVED from v$archived_log a where a.STANDBY_DEST='NO' and a.ARCHIVED='YES' and a.DELETED ='NO' and (a.name is not null ) and a.THREAD#=%ld and a.SEQUENCE#=%ld order by FIRST_CHANGE#"

enum {
	QUERY_OK =1,
	CONNECT_ERROR = -1,
	QUERY_ERROR = -2,
	QUERY_NO_DATA = 1403,
	ERROR_NON_FILEINDEX = -0xff,
	ERROR_NULL = -99,
};

enum{
	STATE_UNKNOW	=0xFF,
	ORACLE_MOUNT	=0x01,
	ORACLE_OPEN		=0x02,
	ORACLE_OPEN_APPLY = 0x03,
	ORACLE_STANDBY	=0x10,	
	ORACLE_PRIMARY	=0x20,
	ORACLE_SHUTDOWN =0X00,
};

// 先声明
struct ASMFile;

// 全局变量，用于存放数据库里查询到的
struct  Datafile
{
	DWORD	FileNum;
	char	FileName[MAX_NAME_LENGTH];
	BYTE8	CreationChange;
	BYTE8	FileSize;
	bool	bExpand;
	DWORD	ASMFileIndex;
	DWORD	GroupNum;
	DWORD	BlockSize;
	DWORD	Blocks;
	ASMFile *pASMFile;

	Datafile()
	{
		FileNum = 0;
		memset(FileName,0,sizeof(FileName));
		CreationChange = 0;
		FileSize = 0;
		bExpand = false;	//是否被扩展过了
		ASMFileIndex = -1;
		GroupNum = -1;
		pASMFile = NULL;
	}
	Datafile(const Datafile& Node);
	Datafile& operator=( const Datafile& b);
};

typedef vector<Datafile> Datafiles;

struct RedologFile
{
	char    FileName[MAX_NAME_LENGTH];
	BYTE8	BeginSCN;
	BYTE8	NEXTSCN;
	DWORD	nStatus;
	DWORD	Sequence;
	DWORD	Thread;
	RedologFile()
	{
		memset(FileName,0,sizeof(FileName));
		BeginSCN = 0;
		NEXTSCN = 0;
		nStatus = _INACTIVE;		// 0 表示inactive 1 表示active 2表示current
		Sequence = 0;
		Thread = 1;
	}
	RedologFile(const RedologFile& Node)
	{
		this->BeginSCN = Node.BeginSCN;
		this->NEXTSCN = Node.NEXTSCN;
		this->nStatus = Node.nStatus;
		strncpy(this->FileName,Node.FileName,sizeof(this->FileName));
		this->Sequence = Node.Sequence;
		this->Thread = Node.Thread;
	}
};

struct DISK;
struct Group;
struct ASMFile;

typedef vector<RedologFile> Redologs;
typedef vector<DISK> VDisks;
typedef vector<vector<DISK> > VVDisks;
typedef vector<Group> VGroups;

//class LGC_TblsMeta_Mgr;
class LGC_RedoFileInfoList;
class LGC_RedoFileInfo;
class LGC_TblsMeta_Mgr;

class OciQuery
{
public:
	OciQuery();
	OciQuery(const char *sid,const char *user, const char *passwd);
	~OciQuery();
	
	bool ConnectOCI();

	void SetValses(const char *sid,const char *user, const char *passwd);

	int GetDatafilePath(string &mystr,int Index); //把Index写到查询语句中去
	int GetDatafilePath(char *mystr,int Index);

	bool GetAllDatafileInfo(Datafiles *datafiles,BYTE8 &MaxCreationChange);
	WORD GetNewDatafileInfo(int FileIndex,char *filename);
	BYTE8 GetExpandFileSize(int FileIndex);

	int GetASMFileInfo(ASMFile *pASMFile,char *mystr);
//	int GetASMFileInfo2(ASMFile *pASMFile,char *mystr);
    int GetASMFileInfo2(ASMFile *pASMFile,char *mystr,int Groupnum);
	int GetGroupInfo(Group *pGroup,char *mystr);
	int GetASMDisks(VDisks *pDisks,int GroupNum);

	// 得到所有Group与Disk信息
	int GetAllGroupInfo(VGroups *pGroup);
	int GetAllASMDisks(VVDisks *pDisks,int GroupSize); 

	// 不通过元数据来查询AU信息，直接查询视图来找AU信息
//	int GetAUInfo(ASMFile *pASMFile,int GroupNum,int FileIndex,int AUIndex);
	int GetAUInfo2(ASMFile *pASMFile,BindDatas &Datas,int DataNum);	//绑定变量方法

	bool GetArchivedLogBySCN_DATE(Redologs &Logs,BYTE8 SCN,const char* DATE = NULL,BYTE8 Rac2SCN = 0); //num为返回项目数
	bool GetArchivedLogBySCN_DATE2(Redologs &Logs,BYTE8 SCN,const char* DATE = NULL,BYTE8 Rac2SCN = 0);// 绑定变量的方法查询
	bool GetArchivedLogName(Redologs &Logs);	//得到所有的归档文件
	bool GetArchivedLogBySCN(char *Logfile,long SCN);
	bool GetArchivedLogBySEQ(char *Logfile,BYTE8 *SCN,long SEQ);

	bool GetRedologNames(Redologs &Logs);
	bool GetCurrentRedolog(Redologs &Logs);

	bool GetRedologInactive(Redologs &Logs,int linenum = -0xFF,int SCN =0);

	bool InsertExeSQL(char *mystr);
	bool TestConnect();
	bool FlashOracleCache();

	bool GetNewControlFile(char *ControlFile);
	bool GetNewLogfile();

	int GetOracleState();	//得到oracle的打开状态
	bool GetSyncState(int &MaxSeq,bool &Appled);	//得到主备库同步状态

	void close();

	void GetErrorMsg(char *msg);
	char *GetLastQuery();

	int GetAllDataFileName(string *names,char *path = NULL);
	int GetAllDataFileName(char *names); //xlc编译环境对于string支持不好

	BYTE8 GetAnalyseSCN();//得到正在分析的归档的SCN号
	BYTE8 GetNextChange();

	bool AlterDatabaseOpen();
	bool CreateDatafile(int FileINdex,const char *FIleName);

	//用脚本来实现的函数
	bool ShutdownImmediate();
	bool StartupMount();
	bool Startup();
	bool RecoverDatabase(char *sUser= NULL,char *sPasswd=NULL,char *sSID=NULL,char *Rac2SID=NULL);
	bool RenameDatafile(char *StrPath);

	bool getTableMetaInfo(LGC_TblsMeta_Mgr* pTablesMetaMgr);
	bool getTableMetaInfo(LGC_TblsMeta_Mgr* pTablesMetaMgr, const char* ownerName);
	bool getTableMetaInfo(LGC_TblsMeta_Mgr* pTablesMetaMgr, const char* owner, const char* tableName);
	bool getTableCols(const char* ownerName, const char* tableName, WORD* pCols);
	
	bool lgc_getArchFileList( LGC_RedoFileInfoList* pList, WORD threadId, BYTE8 startSCN, int (*callBackFunc)(void*,const char*,char*));
	bool lgc_getOnlineFileList(LGC_RedoFileInfoList* pList, WORD threadId, BYTE8 startSCN, int (*callBackFunc)(void*,const char*,char*));
	bool lgc_getArchFileListBySeq(LGC_RedoFileInfoList* pList, WORD threadId, DWORD sequence, int (*callBackFunc)(void*,const char*,char*));
	bool lgc_getOnlineFileListBySeq(LGC_RedoFileInfoList* pList, WORD threadId, DWORD sequence,int (*callBackFunc)(void*,const char*,char*));
	
	bool lgc_getArchFileInfo(LGC_RedoFileInfo* pArchFileInfo, WORD threadId, DWORD sequence, int (*callBackFunc)(void*,const char*,char*));
    bool lgc_getOnlineFileInfo(LGC_RedoFileInfo* pOnlineFileInfo, WORD threadId, DWORD sequence, int (*callBackFunc)(void*,const char*,char*));
	bool lgc_isArchivedOfRedoFile(bool* pIsArchived, WORD thread, DWORD sequence);

	inline char* sid(){
		return SID;
	}
	inline char* usr(){
		return USER;
	}
	inline char* passwd(){
		return PASSWD;
	}
	
private:
	oracleoci m_OciApi;
	bool bConnected;
	char SID[256];
	char USER[256];
	char PASSWD[256];
	char QuerySql[512];

};

#endif
