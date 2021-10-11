//**************************************************************
//*
//* ASM.h
//* 设 计 者：wqy 日期：2012-6-25:13
//* 修 改 者：wqy 日期：2012-6-25:13
//* 版 本：
//*
//****************************************************************

#ifndef _ASM_
#define _ASM_

//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>
//#include <unistd.h>

#include <vector>
#include <iostream>
#include <string>

#include <fcntl.h>
#include "Defines.h"
#include "OciQuery.h"
#include "RedologINI.h"
#include "ChuckSum.h"
#include "Unit.h"

#define STR_LEN 256

#if 0
//////////////////////////
#define Diretc_Ext 60
#define AUinMetaB 480
#define F760 S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP
#define POS_AU 1184   // 4a0
#define POS_AU_OFFS 1024  // 4a0-KFFFDB_L
//5312
#define KFBH_L 32
#define KFDHDB_L 236
#define XPTR_L 8
#define KFFFDB_L 160
#define KFFIXB_L 12
#define DKIF_L 20
#define NAMESIZE 32

#define ASMHEAD "ORCL:"
//#define MYOPEN_READ (O_RDONLY|O_DIRECT)
#define RETYE_TIME 3
#define CHECK_SUM_TIMES 5
#define MAX_BLOCK_SIZE 16384

using namespace std;

enum {
	ASM_ERROR_CHECK_BLOCKINDEX = -0x100,
	ASM_ERROR_CHECK_BLOCKSEQ = -0x101,
};
enum{
	ASM_BLOCK_NUM_OFFSET = 4,
	ASM_BLOCK_BIT	= 0x003FFFFF,
	ASM_SEQ_OFFSET = 8,
};

////////////////////////////////////////////////////////////////////
// ASM里的结构体
typedef struct KFFIXB
{
        DWORD dxsn;    
        WORD xtntblk; 
        BYTE dXrs;    
        BYTE ub1spare;
        DWORD ub4spare;
}KFFIXB;

typedef struct XPTR
{
        DWORD au;  
        WORD disk;
        BYTE flags;
        BYTE chk;
}XPTR;

typedef struct KFBH
{ 
        BYTE endian;
        BYTE hard;
        BYTE type;     
        BYTE datfmt;   
        DWORD block_blk;
        DWORD block_obj;
        DWORD check;    
        DWORD fcn_base; 
        DWORD fcn_wrap; 
        DWORD spare1;   
        DWORD spare2;   
}KFBH;

typedef struct KFDHDB
{
        BYTE driver[NAMESIZE];              
        DWORD compat;
        WORD dsknum;
        BYTE grptyp;       
        BYTE hdrsts;       
        BYTE dskname[NAMESIZE];      
        BYTE grpname[NAMESIZE];     
        BYTE fgname[NAMESIZE];
        BYTE capname[NAMESIZE];   
        DWORD crestmp_hi;   
        DWORD crestmp_lo;   
        DWORD mntstmp_hi;   
        DWORD mntstmp_lo;   
        WORD secsize;      
        WORD blksize;      
        DWORD ausize;       
        DWORD mfact;        
        DWORD dsksize;      
        DWORD pmcnt;        
        DWORD fstlocn;      
        DWORD altlocn;      
        DWORD f1b1locn;     
        WORD redomirrors[4];
        DWORD dbcompat;     
        DWORD grpstmp_hi;   
        DWORD grpstmp_lo;
}KFDHDB;

typedef struct KFFFDB
{
        DWORD node_incarn;   
        DWORD node_frlist_num;
        DWORD node_frlist_inc;
        DWORD hibytes;       
        DWORD lobytes;       
        DWORD xtntcnt;       
        DWORD xtnteof;       
        DWORD blkSize;       
        BYTE flags;         
        BYTE fileType;      
        BYTE dXrs;          
        BYTE iXrs;          
        DWORD dXsiz[3];      
        DWORD iXsiz[3];      
        WORD xtntblk;       
        WORD asm_break;
		BYTE priZn;         
        BYTE secZn;         
        WORD ub2spare;      
        DWORD alias[2];      
        BYTE strpwdth;
        BYTE strpsz;        
        WORD usmsz;         
        DWORD crets_hi;      
        DWORD crets_lo;      
        DWORD modts_hi;      
        DWORD modts_lo;      
        DWORD spare[16];     
}KFFFDB;
////////////////////////////
enum {
	_ASM_OK = 1,
	_ERROR_DISK = -1,
	_ERROR_OPEN = -2,
	_ERROR_STARTAU = -3,
	_ERROR_AU_EOF = -0xFF,
	_ERROR_DATA_DALY = -0xFE,	//数据延迟
	_ERROR_AU_BLOCK = -4,	//AU跳块错误
	_ERROR_FILENAME = -5,
	_ERROR_ASM = -99,
	_ERROR_INIT = -100,
	_ERROR_SEEK = -101,
	_ERROR_READ = -102,
	_ERROR_ASM_READ = -103,
	_ERROR_CHECKSUM = -104,
};

enum {
	KFDGTP_INVALID	= 0,
	KFDGTP_EXTERNAL	= 1,
	KFDGTP_NORMAL	= 2,
	KFDGTP_HIGH		= 3,
};

enum {
	EXTERNAL	= 17,
	NORMAL		= 18,
	HIGH		= 19,
};

enum {
	_DISKHEAD	= 1,
	_FILEDIR	= 4,	// 直接索引所在AU
	_INDIRECT	= 12,	// 间接索引
} ;

// ASM中的磁盘信息
struct DISK
{
	int GroupNum;
	int DiskNum;
	char Path[STR_LEN];
	int DiskFd;
	int Disklogfd;
	int OpenMode;	// 打开方式，ASM也可写
	DISK(){
		DiskFd = 0;
		GroupNum = -1;
		DiskNum = -1;
		Disklogfd = 0;
		OpenMode = -1;
		memset(Path,0,sizeof(Path));
	}
	DISK(const DISK& Node)
	{
		this->GroupNum	= Node.GroupNum;
		this->DiskNum	= Node.DiskNum;
		this->DiskFd	= Node.DiskFd;
		strncpy(this->Path,Node.Path,sizeof(this->Path));
		this->DiskFd	= Node.DiskFd;
		this->Disklogfd	= Node.Disklogfd;
		this->OpenMode	= Node.OpenMode;
	}
};


// AU索引信息
struct ExternInfo
{
	int AuType;
	int DiskNum;
	int AuNum;
	int BlockNum;	// 直接索引时要用
	ExternInfo(){
		AuType = -1;
		AuNum = -1;
		DiskNum = -1;
		BlockNum = 0;
	}
};

typedef vector<ExternInfo> ExternInfos; 

// 一个ASM文件的头部包含了冗余度信息
struct ASMFile
{
	int GroupNum;
	int	ASMFileIndex;
	int AU_Size;		// 从磁盘组结构体里取
	int BlockSize;		// 一般，数据文件是8192,归档512,元数据文件为4096
	int MetaBlockSize;	// 元数据块大小
	int TotelAUCount;	// 文件所占AU * 冗余度 kfffdb.xtntcnt
	int DirectRedundancy;	//直接索引冗余度 kfffdb.dXrs
	int InDirectRedundancy;	//间接索引冗余度 kfffdb.iXrs
	int bLogfile;
	int FileSize;
	int Blocks;
	bool reload;
	ExternInfos AUIndexs;	// AU索引信息
	void Clear(){
		GroupNum		= -1;
		ASMFileIndex	= 0;
		AU_Size			= 0;
		BlockSize		= 0;
		MetaBlockSize	= 0;
		TotelAUCount	= 0;
		DirectRedundancy	= 0;
		InDirectRedundancy	= 0;
		FileSize = 0;
		Blocks = 0;
		bLogfile = 0;
		reload = true;
		AUIndexs.clear();
	}
	ASMFile(){
		Clear();
		reload = true;	// true时，内容要重新载入
	}
} ;

// ASM中的群组信息
// Metadata:元数据
struct Group
{
	int GroupNum;
	int AU_Size;
	int Block_Size;
	int Redundancy_Type;
	int MetaBlockSize;	// 元数据块大小
	ASMFile *pMetadataFile;		// 元数据文件，即ASM中的1号文件
	char Name[NAMESIZE];
	Group()
	{
		GroupNum = -1;
		AU_Size = -1;
		Block_Size = -1;
		Redundancy_Type = -1;
		pMetadataFile = NULL;
		memset(Name,0,sizeof(Name));
	}
};


//OCI接口 SetAsmValues函数中要用到
class OciQuery;

int LoadAllDisksOCI();	//通过查询，载入所有磁盘信息

class ASM
{
private:
	ASMFile *m_pASMFile;
	BYTE *OneBlock;

	vector<DISK>* m_pDisks;	// 指向全局缓存里的一个数组
	Group* m_pGroup;		// 指向全局变量中的Group信息

	int m_FileAuOffset;
	int m_BlockRemain;

public:
	ASM();
	~ASM();

	int SetAsmValues(char *FileName,OciQuery *myQuery,int FileIndex);
	int GetASMFileInfo(char *FileName,OciQuery *myQuery);
	int ClearASMFileInfo();

	int LoadASMFileHead();

	// 通过磁盘号，得到磁盘信息,前提是m_pDisks,m_Groups都已经初始化过了
	int GetDisk(int DiskNum);	

	//把从块号StartBlock开始，连续BlockNum个块的内容放入内存pMem中
	int GetBlocks(BYTE *pMem,BYTE8 StartBlock,int BlockNum);
	int GetBlocks(BYTE *pMem,ASMFile *pFile,BYTE8 StartBlock,int BlockNum); 

	//把从AU号AUStart开始，1块的AU内容放入内存pMem中
	int GetRemainBlocks();
	int GetOneAu(BYTE* AuPool,DWORD nSeq);
	int GetAU(BYTE *pMem,int AUStart,int nSeq);
	int GetAU(BYTE *pMem,ASMFile *pFile,int AUStart,int nSeq); 

	// 从逻辑AU得到物理AU
	int LogicalAUToPhysicalAU(BYTE8 LogicalAU,XPTR &PhysicalAU,ASMFile *pASMFile,int &DiskNum,int &DiskFD);

	int ASM2MEM(BYTE* TheMem,int DiskNum,int AUNum,int BlockInsideAU,int StartBlockNum,int BlockNum,int nSeq = 0,bool bFristBlock = false);
	int MEM2ASM();

	// StartIndex 表示要检查的是块号或SEQ,BlockNum为0时，调用CheckAUSequence
	int CheckMem(BYTE *buf,DWORD StartIndex,int BlockNum,DWORD BlockSize,bool bFirst = false);
	int CheckBlocks(BYTE *buf,int StartBlock,int BlockNum,DWORD BlockSize);
	int CheckAUSequence(BYTE *buf,DWORD Sequence,DWORD BlockSize,bool bFirst);

	int GetAuSize();
	int PrintDebug();

};

#endif

#endif

