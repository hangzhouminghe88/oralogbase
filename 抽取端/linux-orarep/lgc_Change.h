#ifndef LGC_CHANGE_H
#define LGC_CHANGE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>
using namespace std;

#include "lgc_Defines.h"
#include "lgc_Structure.h"


class LGC_RedoRecord;
class LGC_UndoChange;
class LGC_OpcodeParser;
class LGC_SupplementalParser;
class LGC_DmlRowsOutput;
class LGC_DataArrayDump;

enum ChangeType
{
	UNKOWN_UNDOCHANGE=1, 
	INSERT_UNDOCHANGE, 
	DELETE_UNDOCHANGE, 
	UPDATE_UNDOCHANGE, 
	ROWCHAIN_UNDOCHANGE,
	MFC_UNDOCHANGE,
	LMN_UNDOCHANGE,
	MULTIINSERT_UNDOCHANGE,
	MULTIDELETE_UNDOCHANGE,
	INSERT_REDOCHANGE,
	DELETE_REDOCHANGE,
	UPDATE_REDOCHANGE,
	MULTIINSERT_REDOCHANGE,
	ROWCHAIN_REDOCHANGE,
	BEGINT_CHANGE,
	ENDT_CHANGE,
	UNKOWN_CHANGE
};

class LGC_Change
{
private:
	struct ChangeAddr
	{
		unsigned short threadId;
		RBA recordRBA;
		unsigned short changeNo;
	};

protected:
	//member variables
	LGC_RedoRecord* m_pRedoRecord;
	ChangeAddr m_addr;

	log_changeVector_header m_changeHeader;
	unsigned short* m_lenArray;
	unsigned short* m_lenArrayAlign;
	unsigned short  m_datas;
	char**          m_dataArray;
	
	ChangeType		m_changeType;
	LGC_UndoChange* m_pUndoChange;

	LGC_OpcodeParser* m_pOpcodeParser;
	
	//记录对象是否被释放过：0--没有 other-释放
	unsigned char m_beFreed;

private:
	//静态成员变量
	static BYTE8 s_createTimes;
	static BYTE8 s_freeTimes;

//constructor and desctructor
protected:
	LGC_Change(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign, 
	           char** dataArray,unsigned int changeNo);
public:	
	virtual ~LGC_Change();

public:
	//public member functions

	//public virtual functions
	virtual int init()=0;
	virtual bool isNeedAddToTrsct()=0;
	virtual LGC_TransactionId getTransactionId()=0;
	virtual bool isEndOfDml()=0;
	virtual unsigned int getObjNum()=0;
	virtual unsigned short getStartColNo()=0;
	virtual int generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput)=0;

public:
	//这些共有方法不应该被子类隐藏或者覆盖
	bool isUndoChange() const;
	unsigned short getThreadId() const;
	BYTE8 getSCN() const;
	BYTE getOpMajor() const;
	BYTE getOpMinor() const;

	void setUndoChange(LGC_UndoChange* pChange);

	inline ChangeType getChangeType() const{
		return m_changeType;
	}

	inline bool isBeginTrsctChange() const{
		return (m_changeType == BEGINT_CHANGE);
	}

	inline bool isEndTrsctChange() const{
		return (m_changeType == ENDT_CHANGE);
	}

public:
	string toString();
	string changeTypeToString();
	string changeAddrToString();
	string changeTIdToString();
	string objNumToString();

private:
	char* getClearedDumpTextBuf();

public:
	//static member functions
	static LGC_Change* createChange(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& pChangeHeader, 
		                            unsigned short** pLenArray, unsigned short** pLenArrayAlign, char*** pDataArray,
									unsigned int changeNo);

	static void freeChange(LGC_Change* pChange);

};


//.......................................
//class LGC_UndoChange
//.......................................

class LGC_UndoChange:public LGC_Change
{
private:
	//private member variables
	log_opcode_51        m_logOp51;
	log_opcode_51_second m_logOp51Second;
	log_opcode_kdo	     m_logOp51Kdo;
	
	unsigned short m_supLogOff;
	LGC_SupplementalParser* m_pSupplementalParser;
	
	LGC_Change* m_pRedoChange;

//constructor and desctructor
private:
	LGC_UndoChange(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign,
	               char** dataArray,unsigned int changeNo);
public:
	virtual ~LGC_UndoChange();

public:
	//public member functions

	//public virtual functions
	virtual int init();
	virtual bool isNeedAddToTrsct();
	virtual LGC_TransactionId getTransactionId();
	virtual bool isEndOfDml();
	virtual unsigned int getObjNum();
	virtual unsigned short getStartColNo();
	virtual int generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput);

private:
	//private member functions
	int buildParsers();
	bool checkDataNum();

	ChangeType calculateUndoChangeType();
	unsigned short calculateSupLogOff();
	
	LGC_OpcodeParser* createOpcodeParser();
	LGC_SupplementalParser* createSupplementalParser();

private:
	//some get or set properties functions
	inline bool isHaveSupLog(){
		return (m_logOp51Kdo.opcode&0x20);
	}

	inline unsigned short getXidSlot(){
		return m_logOp51.xid_mid;
	}
	
	inline unsigned int getXidSqn(){
		return m_logOp51.xid_low;
	}

	inline bool isNeedFree(){
		return (m_pRedoChange == NULL);
	}

public:
	//特有的接口
	unsigned char getSupLogFlag();
	void setRedoChange(LGC_Change* pRedoChange);

public:
	//static member functions
	static LGC_UndoChange* createUndoChange(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& pChangeHeader, 
		                            unsigned short** pLenArray, unsigned short** pLenArrayAlign, char*** pDataArray,
		                            unsigned int changeNo);
	static void freeUndoChange(LGC_UndoChange* pUndoChange);

};

//.......................................
//class LGC_InsertRedoChange
//.......................................

class LGC_InsertRedoChange:public LGC_Change
{
private:
	//member variables
	log_opcode_kdoirp m_logOpKdoirp;
//constructor and desctructor
private:
	LGC_InsertRedoChange(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign, 
	                     char** dataArray, unsigned int changeNo);
public:
	virtual ~LGC_InsertRedoChange();

public:
	//public member functions

	//public virtual functions
	virtual int init();
	virtual bool isNeedAddToTrsct();
	virtual LGC_TransactionId getTransactionId();
	virtual bool isEndOfDml();
	virtual unsigned int getObjNum();
	virtual unsigned short getStartColNo();
	virtual int generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput);

private:
	//private member functions

public:
	//some get or set properties functions

public:
	//static member functions
	static LGC_InsertRedoChange* createInsertRedoChange(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& pChangeHeader, 
		                            unsigned short** pLenArray, unsigned short** pLenArrayAlign, char*** pDataArray,
		                            unsigned int changeNo);
};


//.......................................
//class LGC_DeleteRedoChange
//.......................................

class LGC_DeleteRedoChange:public LGC_Change
{
private:
	//private member variables
	log_opcode_kdodrp m_logOpKdodrp;
public:
	//public member functions

	//public virtual functions
	virtual int init();
	virtual bool isNeedAddToTrsct();
	virtual LGC_TransactionId getTransactionId();
	virtual bool isEndOfDml();
	virtual unsigned int getObjNum();
	virtual unsigned short getStartColNo();
	virtual int generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput);

private:
	//constructor and desctructor
	LGC_DeleteRedoChange(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign, 
		                 char** dataArray,unsigned int changeNo);
public:
	virtual ~LGC_DeleteRedoChange();

private:
	//private member funtions

public:
	//static member functions
	static LGC_DeleteRedoChange* createDeleteRedoChange(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& pChangeHeader, 
		                            unsigned short** pLenArray, unsigned short** pLenArrayAlign, char*** pDataArray,
		                            unsigned int changeNo);

};



//.......................................
//class LGC_UpdateRedoChange
//.......................................

class LGC_UpdateRedoChange:public LGC_Change
{
private:
	//private member variables
	log_opcode_kdourp m_logOpKdourp;

//constructor and desctructor
private:
	LGC_UpdateRedoChange(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign, 
	                     char** dataArray, unsigned int changeNo);
public:
	virtual ~LGC_UpdateRedoChange();

public:
	//public member functions

	//public virtual functions
	virtual int init();
	virtual bool isNeedAddToTrsct();
	virtual LGC_TransactionId getTransactionId();
	virtual bool isEndOfDml();
	virtual unsigned int getObjNum();
	virtual unsigned short getStartColNo();
	virtual int generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput);

private:
	//private member funtions

public:
	//static member functions
	static LGC_UpdateRedoChange* createUpdateRedoChange(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& pChangeHeader, 
		                            unsigned short** pLenArray, unsigned short** pLenArrayAlign, char*** pDataArray,
		                            unsigned int changeNo);
};


//.......................................
//class LGC_MultiInsertRedoChange
//.......................................

class LGC_MultiInsertRedoChange:public LGC_Change
{
private:
	//private member variables
	log_opcode_kdoqm m_logOpKdoqm;

//constructor and desctructor
private:
	LGC_MultiInsertRedoChange(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign, 
	                          char** dataArray, unsigned int changeNo);
public:
	virtual ~LGC_MultiInsertRedoChange();

public:
	//public virtual functions
	virtual int init();
	virtual bool isNeedAddToTrsct();
	virtual LGC_TransactionId getTransactionId();
	virtual bool isEndOfDml();
	virtual unsigned int getObjNum();
	virtual unsigned short getStartColNo();
	virtual int generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput);

public:
	//public member functions
	int generateRowsData(LGC_DmlRowsOutput* pDmlRowsOutput);

private:
	//private member funtions

public:
	//static member functions
	static LGC_MultiInsertRedoChange* createMultiInsertRedoChange(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& pChangeHeader, 
		                            unsigned short** pLenArray, unsigned short** pLenArrayAlign, char*** pDataArray,
		                            unsigned int changeNo);

};


//.......................................
//class LGC_RowChainRedoChange
//.......................................

class LGC_RowChainRedoChange:public LGC_Change
{
private:
	//private member functions
	log_opcode_kdoirp m_logOpKdoirp;

//constructor and desctructor
private:
	LGC_RowChainRedoChange(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign, 
	                        char** dataArray, unsigned int changeNo);
public:
	virtual ~LGC_RowChainRedoChange();

public:
	//public member functions

	//public virtual functions
	virtual int init();
	virtual bool isNeedAddToTrsct();
	virtual LGC_TransactionId getTransactionId();
	virtual bool isEndOfDml();
	virtual unsigned int getObjNum();
	virtual unsigned short getStartColNo();
	virtual int generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput);

private:
	//private member functions

public:
	//static member functions
	static LGC_RowChainRedoChange* createRowChainRedoChange(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& pChangeHeader, 
		                            unsigned short** pLenArray, unsigned short** pLenArrayAlign, char*** pDataArray,
		                            unsigned int changeNo);

};


//.......................................
//class LGC_BeginTChange
//.......................................

class LGC_BeginTChange:public LGC_Change
{
private:
	//private member functions
	log_opcode_52 m_logOp52;
	BYTE8 m_beginSCN;
//constructor and desctructor
private:
	LGC_BeginTChange(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign, 
	                 char** dataArray, unsigned int changeNo);
public:
	virtual ~LGC_BeginTChange();

public:
	//public member functions

	//public virtual functions
	virtual int init();
	virtual bool isNeedAddToTrsct();
	virtual LGC_TransactionId getTransactionId();
	virtual bool isEndOfDml();
	virtual unsigned int getObjNum();
	virtual unsigned short getStartColNo();
	virtual int generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput);

public:
	//BeginTChange特有的接口
	BYTE8 getTrsctBeginSCN() const;

private:
	//private member functions	

private:
	//some get or set properties functions
	inline unsigned short getXidSlot(){
		return m_logOp52.slot;
	}
	
	inline unsigned int getXidSqn(){
		return m_logOp52.seq;
	}

public:
	//static member functions
	static LGC_BeginTChange* createBeginTChange(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& pChangeHeader, 
		                            unsigned short** pLenArray, unsigned short** pLenArrayAlign, char*** pDataArray,
		                            unsigned int changeNo);
};


//.......................................
//class LGC_EndTChange
//.......................................

class LGC_EndTChange:public LGC_Change
{
private:
	//private member functions
	log_opcode_54 m_logOp54;
	BYTE8 m_commitSCN;

//constructor and desctructor
private:
	LGC_EndTChange(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign, 
	               char** dataArray, unsigned int changeNo);
public:
	virtual ~LGC_EndTChange();

public:
	//public member functions

	//public virtual functions
	virtual int init();
	virtual bool isNeedAddToTrsct();
	virtual LGC_TransactionId getTransactionId();
	virtual bool isEndOfDml();
	virtual unsigned int getObjNum();
	virtual unsigned short getStartColNo();
	virtual int generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput);

public:
	//EndTChange特有接口
	BYTE8 getCommitSCN() const;
	bool isTrsctRollback() const;

private:
	//private member functions

private:
	//some get or set properties functions
	inline WORD getXidSlot(){
		return m_logOp54.slt;
	}
	
	inline DWORD getXidSqn(){
		return m_logOp54.sqn;
	}

public:
	//static member functions
	static LGC_EndTChange* createEndTChange(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& pChangeHeader, 
		                            unsigned short** pLenArray, unsigned short** pLenArrayAlign, char*** pDataArray,
		                            unsigned int changeNo);
};

//.......................................
//class LGC_UnkownChange
//.......................................

class LGC_UnkownChange:public LGC_Change
{
//constructor and desctructor
private:
	LGC_UnkownChange(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign, 
	                 char** dataArray, unsigned int changeNo);
public:
	virtual ~LGC_UnkownChange();

public:
	//public member functions

	//public virtual functions
	virtual int init();
	virtual bool isNeedAddToTrsct();
	virtual LGC_TransactionId getTransactionId();
	virtual bool isEndOfDml();
	virtual unsigned int getObjNum();
	virtual unsigned short getStartColNo();
	virtual int generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput);

public:
	//static member functions
	static LGC_UnkownChange* createUnkownChange(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& pChangeHeader, 
		                            unsigned short** pLenArray, unsigned short** pLenArrayAlign, char*** pDataArray,
		                            unsigned int changeNo);
};


//helper class

//....................................................
//class LGC_DataArrayDump
//....................................................
class LGC_DataArrayDump
{
private:
	unsigned short* m_lenArray;
	unsigned short m_datas;
	char**         m_dataArray;

public:
	LGC_DataArrayDump(char** dataArray, unsigned short* lenArray, unsigned short datas);
	~LGC_DataArrayDump();

public:
	char* getHexDump();

};

#endif


