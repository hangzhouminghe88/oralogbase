#ifndef _TW_RWDATABLOCKS_H_
#define _TW_RWDATABLOCKS_H_
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string>

#include "tw_ASM.h"
#include "tw_file.h"
#include "ChuckSum.h"
#include "tw_api.h"

using namespace std;

/*依赖类的向前申明*/
class tw_ASM;


/*
*读写数据文件时需要的数据文件的信息
*/
struct RWInfoOfFile
{
	char* fileName;
	int totalBlks;
	int blkSize;
	bool bFirstBlk;
	TW_FILE_TYPE fileType;
	OciQuery* pQuery;
	RWInfoOfFile(){
		fileName = NULL;
		totalBlks = -1;
		blkSize = -1;
		bFirstBlk = false;
		fileType = FILE_UNKOWN_TYPE;
		pQuery = NULL;
	}

};

/*
*这是一个纯虚类,主要是为读写数据文件的数据块提供统一的接口
*/
class RWDataBlocks_base
{
public:
	RWDataBlocks_base();
	virtual ~RWDataBlocks_base();

	/*
	*函数功能: 设置数据文件的相关信息
	*输入: fileName-数据文件的全路径 blockSizeInFile-数据文件中的数据块大小
	*返回: true--成功 false--失败
	*/
	virtual bool setFileInfo(const RWInfoOfFile* pRWInfoOfFile)=0;
	
	/*
	*函数功能: 从数据文件中读取数据块
	*输入: blkOffInFile--读取的第一个数据块的块号 blksToRdInFile--读取的数据块数
	*返回: true--成功 false--失败
	*/
	virtual int readBlks(const int blkOffInFile, void* buf, const int blksToRdFromFile);

	virtual int writeBlks(const int blkOffInFile, void* buf, const int blksToWriteToFile);
	
protected:
	char* m_fileName;
	int m_blkSize;
	int m_totalBlks;
	int m_fileType;

};

/*
*用来读写普通数据文件
*/
class RWDataBlocks_commonFile:public RWDataBlocks_base
{
public:
	RWDataBlocks_commonFile();
	virtual ~RWDataBlocks_commonFile();

	virtual bool setFileInfo(const RWInfoOfFile* pRWInfoOfFile);
	virtual int readBlks(const int blkOffInFile, void* buf, const int blksToRdFromFile);
	virtual int writeBlks(const int blksOffInFile, void* buf, const int blksToWriteToFile);
	void updateBlocksOfFile(const int blocks);
private:
	bool m_haveOpened;
	int	m_fd;
	int m_openFlag;
};


/*
*用来读写ASM数据文件
*/
class RWDataBlocks_asmFile:public RWDataBlocks_base
{
public:
	RWDataBlocks_asmFile(OciQuery* pASMQuery);
	virtual ~RWDataBlocks_asmFile();

	virtual bool setFileInfo(const RWInfoOfFile* pRWInfoOfFile);
	virtual int readBlks(const int blkOffInFile, void* buf, const int blksToRdFromFile);
	virtual int writeBlks(const int blkOffInFile, void*buf, const int blksToWriteToFile);
	void updateBlocksOfFile(const int blocks);
private:
	tw_ASM* m_pASM;
	OciQuery* m_pASMQuery;

	static OciQuery* s_pASMQuery_data;
	static OciQuery* s_pASMQuery_arch;
};


/*
*根据文件名，实例化出不同的RWDataBlocks_base的子类。
*接受客户端的读写数据文件的请求,并将读写数据文件的数据块的操作传递给实例化的RWDataBlocks_base的子类
*/
class RWDataBlocks
{
public:
	RWDataBlocks();
	virtual ~RWDataBlocks();

	virtual bool setFileInfo(RWInfoOfFile* pRWInfoOfFile);
	virtual int readBlks(const int blkOffInFile,  void* buf,  const int blksToRdFromFile);
	virtual int writeBlks(const int blkOffInFile, void* buf,  const int blksToWriteToFile);
	void updateBlocksOfFile(const int blocks);
private:
	bool setFileInfo_(RWInfoOfFile* pRWInfoOfFile);
	bool doCheckBlocks(void* blksBuf, int blksToCheck);

private:
	char* m_fileName;
	int m_blkSize;
	int m_fileType;
	int m_totalBlks;
	RWDataBlocks_base *m_pRWDataBlocks_adapted;
};

//////////////////////////////////////////////////////////////

//类的职责: 存储ASM文件的物理au分布信息
class RWDataBlocks_map
{
private://一些类型定义
	typedef map< int,RWDataBlocks* >::iterator mapIteratorType;
	typedef map< int,RWDataBlocks* > mapType;
	typedef int mapFirstType;
	typedef RWDataBlocks* mapSecondType;
	typedef RWDataBlocks  mapSecondDataType;
	typedef pair<mapIteratorType, bool> mapInsertRetType;
private://成员变量
	map< int,RWDataBlocks* > m_RWDataBlocks_baseMap;
	static Mutex s_mutex;
	static RWDataBlocks_map *s_instance;

private:
	RWDataBlocks_map();//单实例模式构造函数私有化 禁止外部直接构造

public:
	virtual ~RWDataBlocks_map(); //单实例模式的析构函数 是公有的
	bool findRWDataBlocks(const int fileNo, RWDataBlocks** ppRWDataBlocks);
	bool insertRWDataBlocks(const int fileNo, RWDataBlocks* pRWDataBlocks);
	
	static RWDataBlocks_map* getInstance();

private:	
	inline static int lock(){
		return s_mutex.Lock();
	}
	inline static int unlock(){
		return s_mutex.Unlock();
	}

	
};

//////////////////////////////////////////////////////////////
#endif



