#ifndef _TW_ASM_H
#define _TW_ASM_H

#include <vector>
#include <iostream>
#include <string>

#include "Defines.h"
#include "skip_ASM.h"

using namespace std;

#define NAMESIZE 32

#define STR_LEN 256

#define ASMHEAD "ORCL:"

class OciQuery;

struct DISK;
struct Group;
class skip_ASM;



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
	BYTE8 FileSize;
	int Blocks;
	bool reload;
	ExternInfos AUIndexs;	// AU索引信息
	int curDiskNum;
	int curAuOffInDisk;
	void Clear();
	ASMFile();
} ;

class tw_ASM
{
private:
	ASMFile *m_pASMFile;
	BYTE *m_pOneBlock;

	vector<DISK>* m_pDisks;	// 指向全局缓存里的一个数组
	Group* m_pGroup;		// 指向全局变量中的Group信息

	int m_fileAuOffset;
	int m_remainBlksInFile;
	int m_totalBlksOfFile;
	bool m_isDataFile;
	int m_fileType;

	char* m_oneAuBuf; //用来做缓存

	skip_ASM* m_pASMAdapted;

public:
	tw_ASM();
	~tw_ASM();

//	bool setASMFileInfo(const char *fileName, const int blkSizeInFile, OciQuery *pASMQuery,bool isDataFile);
	bool setASMFileInfo(const char* fileName, const int blkSizeInFile, OciQuery* pASMQuery, int fileType);

	int readBlksFromFile(const int blkOffInFile_, void* blksBuf, const int blksToRead_,OciQuery* pASMQuery);
	int readBlksFromFile_arch(const int blkOffInFile_, void* blksBuf, const int blksToRead_,OciQuery* pASMQuery);
	int readBlksFromFile_online(const int blkOffInFile_, void* blksBuf, const int blksToRead_,OciQuery* pASMQuery);
	void getOnlineExtentInfo(const int blkOffInFile, int* pExtentNo, int* pAuNoInExtent, int* pChunckNoInAu, int *blkNoInChunk);
	int  readBlksInOnline_inSameChunck(char* buf, int extentNo, int auNoInExtent, int chunckNo, int blkOffInChunck, int blksToRead,OciQuery* pASMQuery);


	inline int totalBlksOfFile()
	{
		return m_totalBlksOfFile;
	}
	inline int remainBlksInFile()
	{
		return m_remainBlksInFile;
	}

private:
	bool getASMFileInfo(const char* fileName, OciQuery *pASMQuery);
	bool getFileNameFromPath(const char* filePath, string& fileBaseName);
	bool getGroupNameFromPath(const char* filePath, string& groupName);
	bool setGroupInfoByGroupName(const char* groupName);
	bool setASMInfoOfFileByName(const char* fileBaseName, OciQuery *pASMQuery);
	bool getGrpFirstFileEntry(const int groupNum, int* diskNum, int* pAuOffInDisk, int* pBlkOffInAu, OciQuery* pASMQuery);
	bool getAuInfoInDisk_byQuery(const int fileIndexInASM, const int auOffInFile, int* pDiskNum, int* pAuOffInDisk, OciQuery* pASMQuery);
	bool getAuInfoInDisk_bySkip(const int fileIndexInASM, const int auOffInFile,  int* pDiskNum, int* pAuOffInDisk,OciQuery* pASMQuery);
	bool getAuInfoInDisk_check(const int fileIndexInASM, const int auOffInFile, int* pDiskNum, int* pAuOffInDisk, OciQuery* pASMQuery);
	bool getAuInfoInDisk_noCheck(const int fileIndexInASM, const int auOffInFile, int* pDiskNum, int* pAuOffInDisk, OciQuery* pASMQuery);
	bool getAuInfoInDisk(const int fileIndexInASM, const int auOffInFile, int* pDiskNum, int* pAuOffInDisk, OciQuery* pASMQuery);
	bool readBlksFromFile_skip(const int blkOffInFile_, void* blksBuf, const int blksToRead_,OciQuery* pASMQuery);
	inline void clearASMFileInfo();
};


#endif

