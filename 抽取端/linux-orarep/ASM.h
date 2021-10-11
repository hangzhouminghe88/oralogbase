//**************************************************************
//*
//* ASM.h
//* �� �� �ߣ�wqy ���ڣ�2012-6-25:13
//* �� �� �ߣ�wqy ���ڣ�2012-6-25:13
//* �� ����
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
// ASM��Ľṹ��
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
	_ERROR_DATA_DALY = -0xFE,	//�����ӳ�
	_ERROR_AU_BLOCK = -4,	//AU�������
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
	_FILEDIR	= 4,	// ֱ����������AU
	_INDIRECT	= 12,	// �������
} ;

// ASM�еĴ�����Ϣ
struct DISK
{
	int GroupNum;
	int DiskNum;
	char Path[STR_LEN];
	int DiskFd;
	int Disklogfd;
	int OpenMode;	// �򿪷�ʽ��ASMҲ��д
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


// AU������Ϣ
struct ExternInfo
{
	int AuType;
	int DiskNum;
	int AuNum;
	int BlockNum;	// ֱ������ʱҪ��
	ExternInfo(){
		AuType = -1;
		AuNum = -1;
		DiskNum = -1;
		BlockNum = 0;
	}
};

typedef vector<ExternInfo> ExternInfos; 

// һ��ASM�ļ���ͷ���������������Ϣ
struct ASMFile
{
	int GroupNum;
	int	ASMFileIndex;
	int AU_Size;		// �Ӵ�����ṹ����ȡ
	int BlockSize;		// һ�㣬�����ļ���8192,�鵵512,Ԫ�����ļ�Ϊ4096
	int MetaBlockSize;	// Ԫ���ݿ��С
	int TotelAUCount;	// �ļ���ռAU * ����� kfffdb.xtntcnt
	int DirectRedundancy;	//ֱ����������� kfffdb.dXrs
	int InDirectRedundancy;	//������������ kfffdb.iXrs
	int bLogfile;
	int FileSize;
	int Blocks;
	bool reload;
	ExternInfos AUIndexs;	// AU������Ϣ
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
		reload = true;	// trueʱ������Ҫ��������
	}
} ;

// ASM�е�Ⱥ����Ϣ
// Metadata:Ԫ����
struct Group
{
	int GroupNum;
	int AU_Size;
	int Block_Size;
	int Redundancy_Type;
	int MetaBlockSize;	// Ԫ���ݿ��С
	ASMFile *pMetadataFile;		// Ԫ�����ļ�����ASM�е�1���ļ�
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


//OCI�ӿ� SetAsmValues������Ҫ�õ�
class OciQuery;

int LoadAllDisksOCI();	//ͨ����ѯ���������д�����Ϣ

class ASM
{
private:
	ASMFile *m_pASMFile;
	BYTE *OneBlock;

	vector<DISK>* m_pDisks;	// ָ��ȫ�ֻ������һ������
	Group* m_pGroup;		// ָ��ȫ�ֱ����е�Group��Ϣ

	int m_FileAuOffset;
	int m_BlockRemain;

public:
	ASM();
	~ASM();

	int SetAsmValues(char *FileName,OciQuery *myQuery,int FileIndex);
	int GetASMFileInfo(char *FileName,OciQuery *myQuery);
	int ClearASMFileInfo();

	int LoadASMFileHead();

	// ͨ�����̺ţ��õ�������Ϣ,ǰ����m_pDisks,m_Groups���Ѿ���ʼ������
	int GetDisk(int DiskNum);	

	//�Ѵӿ��StartBlock��ʼ������BlockNum��������ݷ����ڴ�pMem��
	int GetBlocks(BYTE *pMem,BYTE8 StartBlock,int BlockNum);
	int GetBlocks(BYTE *pMem,ASMFile *pFile,BYTE8 StartBlock,int BlockNum); 

	//�Ѵ�AU��AUStart��ʼ��1���AU���ݷ����ڴ�pMem��
	int GetRemainBlocks();
	int GetOneAu(BYTE* AuPool,DWORD nSeq);
	int GetAU(BYTE *pMem,int AUStart,int nSeq);
	int GetAU(BYTE *pMem,ASMFile *pFile,int AUStart,int nSeq); 

	// ���߼�AU�õ�����AU
	int LogicalAUToPhysicalAU(BYTE8 LogicalAU,XPTR &PhysicalAU,ASMFile *pASMFile,int &DiskNum,int &DiskFD);

	int ASM2MEM(BYTE* TheMem,int DiskNum,int AUNum,int BlockInsideAU,int StartBlockNum,int BlockNum,int nSeq = 0,bool bFristBlock = false);
	int MEM2ASM();

	// StartIndex ��ʾҪ�����ǿ�Ż�SEQ,BlockNumΪ0ʱ������CheckAUSequence
	int CheckMem(BYTE *buf,DWORD StartIndex,int BlockNum,DWORD BlockSize,bool bFirst = false);
	int CheckBlocks(BYTE *buf,int StartBlock,int BlockNum,DWORD BlockSize);
	int CheckAUSequence(BYTE *buf,DWORD Sequence,DWORD BlockSize,bool bFirst);

	int GetAuSize();
	int PrintDebug();

};

#endif

#endif

