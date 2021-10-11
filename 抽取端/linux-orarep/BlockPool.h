//////////////////////////////////////////////////////////////////////////
//****************************************************************
//* BlockPool.h
//*	���ݿ鴦��   [5/12/2012 wqy]
//*
//****************************************************************


#ifndef _BLOCK_POOL
#define _BLOCK_POOL

#include "Defines.h"
#include <stdio.h>
#include <string.h>
#include <list>
#include "ResourcePool.h"

//#include "ASM.h"
#include "tw_rwDataBlocks.h"
#include "OciQuery.h"

using namespace std;

#define BLOCKSIZE	512
#define POOLSIZE	200	
#define BLOCKHEAD	0x10
#define DATASIZE	(BLOCKSIZE-BLOCKHEAD)
#define ReloadTime	3

// 20130531 modify by wqy ��Ϊ0x14

#define RECORDMINSIZE	0x14

#define RECORDSCN_GAP 0xFFFFFFFF
//  ��־����������

enum{
	_BLOCK_NUM_OFFSET = 4,
	_BLOCK_BIT	= 0x003FFFFF,
	_BLOCK_SEQ_OFFSET = 8,
};

enum {
	MIN_RECORD_LEN = 0x18,
	MAX_RECORD_LEN = 0xFFFFFF,
};

enum{
	_CURRENT_EOF	= 3,	//��ǰ���Redolog�ļ���β
	_SKIP_RECORDLEN = 3,
	_SKIP_BLOCK		= 2,
	_OK				= 1,
	_FILE_EOF		= -9997,	
	_ANALYSE_ADDFILE = -0x1117,
	_ANALYSE_EXPAND	= -0x1104,
};

enum ANA_ERROR_ {
	_BLOCK_EOF		= -9998,
	_RECORD_ERROR	= -1,
	_CHANGE_ERROR	= -2,
	_CHUCK_ERROR	= -3,
	_ANALYSE_ERROR	= -4,
	_FILE_ERROR		= -5,
	_ERROR_FILE_OPEN = -6,
	_REDOLOG_ERROR	= -99,	//���ش���
	_ERROR			= -100,
	_ValidNum_Error = -101,
	_ANALYSE_END	= -0xFF,
	_SKIP_BLOCK_ERROR = -102,
	_GetOneBlock_EOF = -9999,
	_ANALYSE_INTERRUPT = -10000,
};

enum{
	_NODE_RECOVER = -100,
};

//��̬��Ա����
class RedoBlockPool
{
private:
//	static FILE *RedoFile;

//#ifdef WITHASM 
//	static ASM *RedoASM;
//#endif

	static RWDataBlocks* RedoRWDataBlocks;
	static int BlkOffInFile;
	
public:
	static BYTE BlockPool[POOLSIZE][BLOCKSIZE];	//���ݿ黺��
	static WORD BlockIndex;				//���浱ǰ����
	static WORD PoolHead;				//��������
	static bool bHaveData;
	static DWORD FileSequence;
	static bool bFirstBlock;
	static bool bLastBlock;

	static bool bASM;
	static char *AUPool;//[AUSIZE];
	static int AUoffset;
	static int AUSize;

	static int ms_RedoBlksReadedInAu; //tw edit 20140612
	static int ms_RedoBlkOffInAu; //tw edit 20140612

	static int FillBlockPool(int &SucceedNum,int FillNum = POOLSIZE/10);
//	static int FillPoolWithASM(int &SucceedNum,int FillNum = POOLSIZE/10);

	static int GetOneBlock(BYTE *&block);	
	static int SkipBlocks(DWORD SkipNum,bool bBoundary = false);

	static int readBlks(int blkOffInFile, int blksToRead, void* blksBuf);

/*
	static void SetRedoFile(FILE *pFile,int nSequence){
		RedoFile = pFile;
		BlockIndex = PoolHead;
		bHaveData = false;
		bFirstBlock = true;
		FileSequence = nSequence;
		bLastBlock = false;
#ifdef WITHASM 
		RedoASM = NULL;
#endif
		bASM = false;
	}
#ifdef WITHASM 
	static void SetRedoFile(ASM* pASM,int nSequence){
		RedoFile = NULL;
		BlockIndex = PoolHead;
		bHaveData = false;
		bFirstBlock = true;
		FileSequence = nSequence;
		bLastBlock = false;
		RedoASM = pASM;
		bASM = true;
	}
#endif
*/

	static void SetRedoFile(RWDataBlocks* pRWDataBlocks, int nSequence, bool bASM_=false){
//		RedoFile = NULL;
//		RedoASM = NULL;
		
		RedoRWDataBlocks = pRWDataBlocks;
		BlockIndex = PoolHead; //�����־�����
		bHaveData = false;
		bFirstBlock = true;
		bLastBlock = false;
		FileSequence = nSequence;
		bASM = bASM_;
		BlkOffInFile = 0;
		
		ms_RedoBlksReadedInAu = 0;
		ms_RedoBlkOffInAu = 0;
		
		if(!AUPool){
			AUPool = new char[AUSize];
			if(!AUPool){
				exit(1);
			}
			memset(AUPool,0,AUSize);
		}

	}
};


// ��2��ȥ����ͷ�Ŀ�������redoblock��
class RedoBlock
{

	BYTE *Head1;
	BYTE *Head2;
	DWORD BlockNum1;
	DWORD BlockNum2;
	DWORD LSN1;
	DWORD LSN2;			//log sequence number  ��־���к�
public:
	BYTE *CurrentData;	// ��ǰָ��λ��	
	BYTE BlockCatch[2*DATASIZE]; //
	
	RedoBlock(){};
	~RedoBlock(){};

	WORD BlockOffset;		// 0~495
	WORD ValidNum;
	
	int  ReInit();
	int  LoadNextBlock();
	int  TestBoundary(DWORD Offset);	//���Ա߽�,�������,�������һ��
	int  Move(DWORD Offset);
	int CompareNextBlock();

	int SkipBlocks(DWORD Num,bool bBoundary = false); //

	bool GetData(BYTE &date,int offset=0);
	bool GetData(WORD &date,int offset=0);
	bool GetData(DWORD &date,int offset=0);

	int  GetLeftInOneBlock();
	DWORD GetCurrentNum();
	DWORD GetBlockNum(int FirstOrSecond);
	DWORD GetLSN(int FirstOrSecond);

};

enum ChangeOffset_10G{
	_CHANGE_HEADSIZE	= 0x18,
	_TYPEH	=	0,
	_TYPEL	=	1,	
#ifdef _ORACLE_AIX_64
	_AFN	=	6,
#else
	_AFN	=	4,
#endif
	_DBA	=	8,
	_SCN_BIT	= 0x003FFFFF,
};


/*
#if (ORACLE_VERSION) == (_Oracle_11g)
//oracle 11g�����
enum ChangeState{
	_ADDFILE	=	0x111E,
	_EXPANDFILE	=	0x1104,
};
#else
//oracle 10g�����
enum ChangeState{
	_ADDFILE	=	0x1117,
	_EXPANDFILE	=	0x1104,
};
#endif
*/

enum ChangeState{
	_ADDFILE	=	0x111E,
	_EXPANDFILE	=	0x1104,
};

/*
enum ChangeState{
	_ADDFILE	=	0x1117,
	_EXPANDFILE	=	0x1104,
};
*/



struct Change
{
	WORD HeadLength;	// ʸ��ͷ����
	WORD ControlHeadLength;
	int	 DataLength;	//���ݳ���
	int  ChangeLength;

	BYTE ChangeTypeH;	//	ʸ������ �� 11.2 ChangeTypeH��ʾ11��ChangeTypeL��ʾ2
	BYTE ChangeTypeL;	//

	RedoBlock *m_pBlock;

public:
	int  nSkipBlock;
	WORD FileNum;
	DWORD BlockNum;
	DWORD FileSize;	// ��չ�ļ�ʱҪ�õ�

	Change(RedoBlock *Block);
	~Change(){};

	void GetValues(WORD &nFileNum,DWORD &nBlockNum);
	bool CalValues();
	bool GetValuesInside(int ChangeType,int offset); //��ʸ���ڲ��õ�����

	int  GetChangeLength();	//���������������˼������ݿ�.
	int  CalChangeLength(bool &SkipOneBlock);

	int  CalSkipBlockNum(); //����ChangeLength��������˼���
	int	 GetSkepBlockNum();

	WORD GetFixOffset(WORD nOffset);

	WORD GetChangeState();	//  �õ�ʸ���Ŀ�ͷ��

};

typedef list<Change> Changelist;

enum HeadOffset_10G{
	_VLD	=	4,
	_HSCN	=	6,
	_LSCN	=	8,
	_SSCN	=	12,
	_HEADSIZE=	12,
};

class RedoRecord
{	
public:
	RedoBlock *m_pBlock;
	int RedoRecordLength;	// 4�ֽ�  ��2���ֽ�
	int RedoRecordHeadlen;
	int RedoRecordLeftlen;	//���³���
	BYTE VLD;
	DWORD	m_SCN;
	WORD	m_HSCN;
	WORD	m_SSCN;

	int nSkipBlock;		// ��������Block ����ÿ��Change��nSkipBlock��ӵĺ�
	Changelist m_Changes;	

	RedoRecord(RedoBlock *Block);
	~RedoRecord();
	
	int  GetNextChange(bool &bSkipBlock);
	int  LoadallChanges();
	
	int  SkipRecordHead(int OracleVersion);
	int  HandleRecordHead(DWORD &nSCN,WORD &nHSCN,WORD &SSCN);	//����¼����Ϊ0ʱ
	int  LoadRecordHead();

};

class RedoLog
{
private:
	RedoBlock *OperateBlock;
	DWORD CurrentSCN;

	DWORD ChangeNum;
	Producer m_NodeProducer;
	int m_ErrCode;

	char RedoLogName[MAX_NAME_LENGTH];
	BYTE OneBlock[BLOCKSIZE];
	FILE *m_FILE;
	FILE *m_FileHead;	//��ȡEndSCN��

#ifdef WITHASM 
//	ASM m_ASM;		//��ASM����
#endif
	RWDataBlocks* m_pRWDataBlocks;
public:

	static int InsertLocked();
	static int InsertUnlocked();
	static bool  bInterrupt; //�����жϷ���
	static int SetInterrupt(bool bInter){ 
		bInterrupt = bInter;
		t_debugMsg(t_tmpLvl,"SetInterrupt: bInterrupt=%d\n",bInterrupt);
		return 0;
	};
	

	DWORD Sequence;
	DWORD Status;
	BYTE8 BeginSCN;
	BYTE8 EndSCN;
	WORD  HSCN;
	WORD  SSCN;
	
	DWORD m_Count;

	RedoLog();
	~RedoLog();

	DWORD GetEndSCN();

	bool LoadandSkipLogHead();		//����2��
	int LoadNewFile(char *FileName,int nStatus,int nSequence,bool bASM = false,OciQuery *myQuery = NULL);
//	int LoadNewFile(char *FileName, int blksInFile,blkSize,int nStatus,int nSequence,bool bASM,OciQuery *pASMQuery)

	int GetNextRecord();
	int Loadall();
	void RedoAnalyseEnd();

	bool InsertSpecialNode(int Node);
	bool InsertOneNode(int nFile,int nBlock);
	void GetErrorMsg(int &BlockNum,int &SCN,int &Offset,char *TheReason);
};

#endif
