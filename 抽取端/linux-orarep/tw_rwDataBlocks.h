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

/*���������ǰ����*/
class tw_ASM;


/*
*��д�����ļ�ʱ��Ҫ�������ļ�����Ϣ
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
*����һ��������,��Ҫ��Ϊ��д�����ļ������ݿ��ṩͳһ�Ľӿ�
*/
class RWDataBlocks_base
{
public:
	RWDataBlocks_base();
	virtual ~RWDataBlocks_base();

	/*
	*��������: ���������ļ��������Ϣ
	*����: fileName-�����ļ���ȫ·�� blockSizeInFile-�����ļ��е����ݿ��С
	*����: true--�ɹ� false--ʧ��
	*/
	virtual bool setFileInfo(const RWInfoOfFile* pRWInfoOfFile)=0;
	
	/*
	*��������: �������ļ��ж�ȡ���ݿ�
	*����: blkOffInFile--��ȡ�ĵ�һ�����ݿ�Ŀ�� blksToRdInFile--��ȡ�����ݿ���
	*����: true--�ɹ� false--ʧ��
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
*������д��ͨ�����ļ�
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
*������дASM�����ļ�
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
*�����ļ�����ʵ��������ͬ��RWDataBlocks_base�����ࡣ
*���ܿͻ��˵Ķ�д�����ļ�������,������д�����ļ������ݿ�Ĳ������ݸ�ʵ������RWDataBlocks_base������
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

//���ְ��: �洢ASM�ļ�������au�ֲ���Ϣ
class RWDataBlocks_map
{
private://һЩ���Ͷ���
	typedef map< int,RWDataBlocks* >::iterator mapIteratorType;
	typedef map< int,RWDataBlocks* > mapType;
	typedef int mapFirstType;
	typedef RWDataBlocks* mapSecondType;
	typedef RWDataBlocks  mapSecondDataType;
	typedef pair<mapIteratorType, bool> mapInsertRetType;
private://��Ա����
	map< int,RWDataBlocks* > m_RWDataBlocks_baseMap;
	static Mutex s_mutex;
	static RWDataBlocks_map *s_instance;

private:
	RWDataBlocks_map();//��ʵ��ģʽ���캯��˽�л� ��ֹ�ⲿֱ�ӹ���

public:
	virtual ~RWDataBlocks_map(); //��ʵ��ģʽ���������� �ǹ��е�
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



