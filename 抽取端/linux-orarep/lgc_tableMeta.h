#ifndef LGC_TABLEMETA_H
#define LGC_TABLEMETA_H

#include "Defines.h"
#include "OciQuery.h"
#include "tw_api.h"
#include "lgc_structure.h"
#include "lgc_param.h"


/*
#define OWNER_NAME_LEN 64
#define OBJECT_NAME_LEN 128
#define COLUMN_NAME_LEN  106
*/

struct column_meta
{
	DWORD       objNum;
	WORD        colNo;
	char        colName[64];
	char	    dataType[106];
	BYTE8       dataMaxLen;
	bool        nullAble;

	column_meta();
	//column_meta& operator =(const column_meta& other);
};
typedef struct column_meta column_meta;


struct table_meta
{
	DWORD        objNum;
	char	     owner[64];
	char		 tableName[128];
	WORD         cols;
	column_meta* colMeta_array;
	
	table_meta();
	table_meta& operator =(const table_meta& other);
	~table_meta();
};
typedef struct table_meta table_meta;


class LGC_TblsMeta_Mgr
{
private:
	table_meta** m_tableMeta_array;
	DWORD        m_tableMeta_arrayMems;

	OciQuery*    m_pOciQuery;
	OciQuery*    m_pOciQueryCols;

private:
	static Mutex s_mutex;
	static LGC_TblsMeta_Mgr *s_instance;
	

private:
	LGC_TblsMeta_Mgr();
public:
	~LGC_TblsMeta_Mgr();
	static LGC_TblsMeta_Mgr* getInstance();


public:
	bool  getTableName(DWORD objNum, char** pTableName);
	bool  getTableCols(DWORD objNum, WORD *pCols);
	bool  getTableMetaInfo(DWORD objNum, table_meta** ppTableMeta);

	bool  getColName(DWORD objNum, WORD colNo, char** pColName);
	bool  getColDataType(DWORD objNum, WORD colNo, char** pColDataType);

public:
	bool loadTablesMetaInfo();
//public:
private:
	bool setTablesMetaInfo(const char* owner);
	bool setTableMetaInfo(const char* owner, const char* tableName);
	bool delTableMetaInfo(const char* owner, const char* tableName);

public:
	//just use for OciQuery
	bool setTableMetaInfo(const table_meta* ptableMeta);
	bool setColMetaInfo(DWORD objNum, const column_meta* pColMeta);

public:
	bool isFindOfObj(DWORD objNum);

private:	
	inline static int lock(){
		return s_mutex.Lock();
	}
	inline static int unlock(){
		return s_mutex.Unlock();
	}

private:
	bool reallocTableMetaArray(DWORD mems);
	bool freeTableMetaMem(DWORD objNum);
	bool mellocTableMetaMem(DWORD objNum);
};

#endif

