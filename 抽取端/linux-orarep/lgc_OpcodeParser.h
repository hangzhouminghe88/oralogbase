
#ifndef LGC_OPCODEPARSER_H
#define LGC_OPCODEPARDER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lgc_Defines.h"
#include "lgc_Structure.h"

class LGC_Change;
class LGC_DmlRowsOutput;

//.............................................
//LGC_OpcodeParser
//.............................................
class LGC_OpcodeParser
{
protected:
	//private member functions
	LGC_Change* m_pChange;
	unsigned short* m_lenArray;
	char** m_dataArray;
	unsigned short m_datas;

public:
	//constructor and desctructor
	LGC_OpcodeParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas);
	virtual ~LGC_OpcodeParser();
public:
	//virtual functions
	virtual int generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput)=0;
	virtual unsigned short getDatas()=0;

protected:
	unsigned short getStartColNo();

public:
	//static member functions
	static LGC_OpcodeParser* createOpcodeParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas);
};


//.............................................
//LGC_OpKdoirpParser
//.............................................
class LGC_OpKdoirpParser: public LGC_OpcodeParser
{
private:
	//member variables
public:
	//constructor and desctructor
	LGC_OpKdoirpParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas);
	virtual ~LGC_OpKdoirpParser();

public:
	//virtual public member funcitos 
	virtual int generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput);
	virtual unsigned short getDatas();

private:
	//private functions
	bool check();
public:
	//static member functions
	static LGC_OpKdoirpParser* createOpKdoirpParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas);
};

//.............................................
//LGC_OpKdodrpParser
//.............................................
class LGC_OpKdodrpParser: public LGC_OpcodeParser
{
private:
	//member variables
public:
	//constructor and desctructor
	LGC_OpKdodrpParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas);
	virtual ~LGC_OpKdodrpParser();

public:
	//virtual public member funcitos 
	virtual int generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput);
	virtual unsigned short getDatas();

private:
	//private functions
	bool check();
public:
	//static member functions
	static LGC_OpKdodrpParser* createOpKdodrpParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas);
};

//.............................................
//LGC_OpKdourpParser
//.............................................
class LGC_OpKdourpParser: public LGC_OpcodeParser
{
private:
	//member variables
public:
	//constructor and desctructor
	LGC_OpKdourpParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas);
	virtual ~LGC_OpKdourpParser();

public:
	//virtual public member funcitos 
	virtual int generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput);
	virtual unsigned short getDatas();

private:
	//private functions
	bool check();
public:
	//static member functions
	static LGC_OpKdourpParser* createOpKdourpParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas);
private:
	static bool checkColNoArray(const WORD* colNo_array, const WORD colNoArrayLen, const BYTE newCols);
	static unsigned char getIncrementalColsOfKdourp(const WORD* colNo_array, const BYTE newCols);

};

//.............................................
//LGC_OpKdoqmParser
//.............................................
class LGC_OpKdoqmParser: public LGC_OpcodeParser
{
private:
	//member variables
public:
	//constructor and desctructor
	LGC_OpKdoqmParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas);
	virtual ~LGC_OpKdoqmParser();

public:
	//virtual public member funcitos 
	virtual int generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput);
	virtual unsigned short getDatas();
	virtual int generateRowsData(LGC_DmlRowsOutput* pDmlRowsOutput);	

private:
	//private functions
	bool check();
public:
	//static member functions
	static LGC_OpKdoqmParser* createOpKdoqmParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas);
};

//.............................................
//LGC_OpKdomfcParser
//.............................................
class LGC_OpKdomfcParser: public LGC_OpcodeParser
{
private:
	//member variables
public:
	//constructor and desctructor
	LGC_OpKdomfcParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas);
	virtual ~LGC_OpKdomfcParser();

public:
	//virtual public member funcitos 
	virtual int generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput);
	virtual unsigned short getDatas();

private:
	//private functions
	bool check();
public:
	//static member functions
	static LGC_OpKdomfcParser* createOpKdomfcParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas);

};


//.............................................
//LGC_OpKdolmnParser
//.............................................
class LGC_OpKdolmnParser: public LGC_OpcodeParser
{
private:
	//member variables
public:
	//constructor and desctructor
	LGC_OpKdolmnParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas);
	virtual ~LGC_OpKdolmnParser();

public:
	//virtual public member funcitos 
	virtual int generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput);
	virtual unsigned short getDatas();

private:
	//private functions
	bool check();
public:
	//static member functions
	static LGC_OpKdolmnParser* createOpKdolmnParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas);

};

//.............................................
//LGC_SupplementalParser
//.............................................
class LGC_SupplementalParser: public LGC_OpcodeParser
{
private:
	//private member functions
public:
	//constructor and desctructor
	LGC_SupplementalParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas);
	virtual ~LGC_SupplementalParser();

public:
	//virtual public member funcitos 
	virtual int generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput);
	virtual unsigned short getDatas();

public:
	//LGC_SupplementalParser特有的接口
	unsigned char getSupLogFlag();
	
public:
	//隐藏了基类的函数
	unsigned short getStartColNo();

private:
	//private functions
	bool check();

public:
	//static member functions
	static LGC_SupplementalParser* createSupplementalParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas);

};

#endif




