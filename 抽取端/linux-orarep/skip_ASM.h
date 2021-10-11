#ifndef _TW_ASM_H_
#define _TW_ASM_H_

// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <unistd.h>

#include <vector>
#include <iostream>
#include <string>
#include <map>
#include <vector>

#include "tw_file.h"
#include "Defines.h"
#include "OciQuery.h"

#include "Mutex.h"


#define AUSIZE 1048576

//////////////////////////
#define Diretc_Ext 60
//#define AUinMetaB 479
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

using namespace std;

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
	BYTE driver[32];              
	DWORD compat;
	WORD dsknum;
	BYTE grptyp;       
	BYTE hdrsts;       
	BYTE dskname[32];      
	BYTE grpname[32];      
	BYTE fgname[32];       
	BYTE capname[32];      
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


/* 10g had space for four sizes, but only 1x was used. The remaining sizes
 * are new in 11, but 10g never references them, so it is safe to change the
 * values
 */
#define KFDASZ_1X      1      /*    1x AU size                     */
#define KFDASZ_2X      2      /*    2x AU size                     */
#define KFDASZ_4X      4      /*    4x AU size                     */
#define KFDASZ_8X      8      /*    8x AU size                     */
#define KFDASZ_16X	   16      /*   16x AU size                     */
#define KFDASZ_32X     32     /*   32x AU size                     */
#define KFDASZ_64X     64      /*   64x AU size                     */
#define KFDASZ_LAST    ((kfdasz)7)      /*   First unused value 11g          */
#define KFDASZ_LAST_10 ((kfdasz)4)      /*   First unused value 10g          */
#define KFDASZ_VALUE(x) ((ub1)(1 << (x)))
#define ExtentSIZE 20000


//////////////////////////

struct DISK
{
	int GroupNum;
	int DiskNum;
	char Path[QUERY_LEN];
	int DiskFd;
	int Openflag;
	DISK(){
		DiskFd = -1;
		GroupNum = -1;
		DiskNum = -1;
		Openflag = 0;
		memset(Path,0,sizeof(Path));
	}
};

// 磁盘组的兼容性版本
enum{
	DISK_COMPAT_10_1 = 0x0a100000,
	DISK_COMPAT_11_1 = 0x0b100000,
	DISK_COMPAT_11_2 = 0x0b200000,
};

struct Group
{
	int GroupNum;
	int Block_Size;
	int AU_Size;
	char Name[NAMESIZE];
	Group()
	{
		GroupNum = -1;
		Block_Size = -1;
		AU_Size = -1;
		memset(Name,0,sizeof(Name));
	}
};

class AULctInDisk
{
public:
	AULctInDisk(int diskNumInGroup,int auNumInDisk){
		m_diskNum = diskNumInGroup;
		m_auNumInDisk = auNumInDisk;
	}
	bool operator < (const  AULctInDisk &other)const
	{
		if(m_diskNum < other.m_diskNum){
			return true;
		}else if(m_diskNum==other.m_diskNum){
			if(m_auNumInDisk<other.m_auNumInDisk){
				return true;
			}
		}
		return false;
	}
	bool operator == (const AULctInDisk &other) const
	{
		return m_diskNum==other.m_diskNum&&m_auNumInDisk==other.m_auNumInDisk;
	}

public:
	int m_diskNum;
	int m_auNumInDisk;
};

struct AusLct{
	int m_drctAusInDrctBlk;
	int m_IdrctAusInDrctBlk;
	int m_drctRedundancy;
	vector<XPTR> m_ausInDrctBlk;
	AusLct(){
		m_drctAusInDrctBlk=0;
		m_IdrctAusInDrctBlk = 0;
		m_drctRedundancy = 0;
		m_ausInDrctBlk.clear();
	}
};

class skip_ASM
{
public:
	enum{OUT_OF_AusVec=1};
public:
	// ASM 磁盘组的信息
	int m_grp_num;
	int m_grp_blockSize;
	int m_grp_auSize;
	int m_grp_redundancy;// redundancy disks of this disk group
	vector<DISK> *m_grp_pDisks;
	
	DWORD m_grp_dbcompat; //database compatible version 
	BYTE  m_1ASMFile_stripeWidth;//条带宽度
	int  m_1ASMFile_stripeDepth;//条带深度


	//一号ASM元文件的入口
	int m_1ASMFile_diskNum;
	int m_1ASMFile_auNum;
	int m_1ASMFile_blockNum;

	char* m_1ASMFile_1BlockBuf;
//	AusLct m_1ASMFile_ausLct;
	vector<XPTR> m_1ASMFile_ausVec;
	bool m_1ASMFile_1BlockLoaded;

	//ASM数据文件的ASM索引号
	int m_asmDataFile_num;
	int m_asmDataFile_blkSize;
	int m_asmDataFile_isDataFile;//可能是归档文件或者数据文件 两者行为相差较大
	
	char *m_asmDataFile_directBlkBuf;
//	map<AULctInDisk,char*> m_asmDataFile_metaAusMap;
//	AusLct m_asmDataFile_ausLct;
	vector<XPTR> m_asmDataFile_ausVec;

	bool m_isDirectOpen;
	char* m_twoAuBuf_forDrctRW;

private:
	static Mutex s_readDisk_mutex;

public:
	skip_ASM();
	virtual ~skip_ASM();

	bool setGroupInfo(int grp_num,const vector<DISK> &disks, int auSize,int blockSize,int oneDiskNum=0, int oneAuNum=2, int oneBlkNum=1);

	bool setASMDataFileInfo(int fileIndexInASM,int blkSizeInFile,bool isDataFile, bool bReset=false);

	bool readOneAUFromFile(const int auOffInFile,char *auBuf, int* pAuSize=NULL);
	bool getAuInfoInDisk(int fileIndex, int auOffInFile,int* pDiskNum,int* pAuOffInDisk);


public:
	bool setOneASMFileEntry(int diskNum, int auNum, int blockNum);
	bool loadOneMetaFileEntryBlock();
	bool loadDataFileEntryBlock(const int dataFileIndexInASM);

	bool readOneAUFromDisk(int diskNum,int auNumInDisk,char *auBuf);

	bool readBlocksFromDisk_InSameAu(int diskNum, int auNumInDisk, int blockNumInAu,int blocksToRead, char *blocksBuf);
	bool readBytesFromDisk_inSameAu(int diskNum, int auNumInDisk, int bytesOffInAu, int bytesToRead, char* bytesBuf);
//	bool skip_ASM::readBytesFromDisk_InSameAu(const int diskNum, const int auNumInDisk_, const int byteOffInAu_, const int bytesToRead_, char* readBuf);

	bool openDisk(int diskNum, int flag);

	bool getAuInfoInDisk_(const vector<XPTR> &ausVec,const int auOffInFile, int *pDiskNum,int *pAuNumInDisk,int *errNo);
	bool getAuInfoInDisk_byDrctBlk(const char* drctBlk, int fileIndexInASM, int auOffInFile, int* pDiskNum, int*pAuOffInDisk, int* errNo);

	bool setGroupDisks(const vector<DISK> &disks);
	bool closeAllDisks();

	bool drctBlkToAusVec(const char *drctBlk, vector<XPTR> &ausVec);
	bool ausInIdrctAuPushBackToAusVec(const char* auBuf,vector<XPTR> &ausVec);
	//加载普通ASM文件的au分部信息
	bool loadDataFileAusVec(int fileIndexInASM);
	bool loadDataFileAusVec_noOptimize(int fileIndexInASM);
	bool loadDataFileAusVec_optimize(int fileIndexInASM);
	
	bool getAuInfoInDisk_optimize(int fileIndex, int auOffInFile,int* pDiskNum,int* pAuOffInDisk);
	bool getAuInfoInDisk_noOptimize(int fileIndex, int auOffInFile,int* pDiskNum,int* pAuOffInDisk);

	bool loadDiskHeaderInfo();
	bool loadOneMetaFileHeaderInfo();

public:
	inline int getAuSize(){
		return m_grp_auSize;
	}
	inline int getGrpBlkSize(){
		return m_grp_blockSize;
	}
	//get database compatible version 
	inline DWORD getDbcompat(){
		return m_grp_dbcompat;
	}
	inline BYTE getStripeWidth()
	{
		return m_1ASMFile_stripeWidth;
	}
	inline int getStripeDepth()
	{
		return m_1ASMFile_stripeDepth;
	}
};

//类的职责: 存储ASM文件的物理au分布信息
class DataFilesAuDistribute
{
private://一些类型定义
	typedef map< int,vector<XPTR>* >::iterator mapIteratorType;
	typedef map< int, vector<XPTR>* > mapType;
	typedef int mapFirstType;
	typedef vector<XPTR>* mapSecondType;
	typedef vector<XPTR>  mapSecondDataType;
	typedef pair<mapIteratorType, bool> mapInsertRetType;
private://成员变量
	map< int,vector<XPTR>* > m_auDistributeOfFiles;
	static Mutex s_mutex;
	static DataFilesAuDistribute *s_instance;

private:
	DataFilesAuDistribute();//单实例模式构造函数私有化 禁止外部直接构造

public:
	virtual ~DataFilesAuDistribute(); //单实例模式的析构函数 是公有的
	bool findAuInfoOfFile(const int fileNo, vector<XPTR>* pAuInfoOfFile);
	bool insertAuInfoOfFile(const int fileNo, const vector<XPTR>* pAuInfoOfFile);
	void clear();
	
	static DataFilesAuDistribute* getInstance();

private:	
	inline static int lock(){
		return s_mutex.Lock();
	}
	inline static int unlock(){
		return s_mutex.Unlock();
	}

private:
	bool copyXPTRVec(const vector<XPTR> &srcVec, vector<XPTR> &dstVec);


	
};
#endif
