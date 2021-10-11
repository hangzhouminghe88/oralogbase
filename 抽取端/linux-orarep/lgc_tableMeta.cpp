#include <string.h>

#include "lgc_tableMeta.h"
#include "lgc_api.h"


column_meta::column_meta()
{
	objNum = 0;
	colNo = 0;
	memset(colName, 0, 64);
	memset(dataType,0, 106);
	dataMaxLen = 0;
	nullAble = false;

	return;
}

table_meta::table_meta()
{
	objNum = 0;
	memset(owner, 0, 64);
	memset(tableName, 0, 128);
	cols = 0;
	colMeta_array = NULL;

	return;
}

table_meta& table_meta::operator =(const table_meta& other)
{
	this->objNum = other.objNum;
	memcpy(this->owner, other.owner, 64);
	memcpy(this->tableName, other.tableName,128);
	this->cols = other.cols;
	this->colMeta_array = new column_meta[this->cols+1];
	if(!this->colMeta_array){
		exit(1);
	}
	memcpy(this->colMeta_array,0,(this->cols+1)*sizeof(column_meta));

	if(other.colMeta_array){
		memcpy(this->colMeta_array, other.colMeta_array, (this->cols+1)*sizeof(column_meta) );
	}

	return *this;
}

table_meta::~table_meta()
{
	if(colMeta_array){
		delete[] colMeta_array;
		colMeta_array = NULL;
	}

	return;
}


Mutex LGC_TblsMeta_Mgr::s_mutex;
LGC_TblsMeta_Mgr* LGC_TblsMeta_Mgr::s_instance = NULL;

LGC_TblsMeta_Mgr::LGC_TblsMeta_Mgr()
{

	m_tableMeta_arrayMems = 10000;
	m_tableMeta_array = new table_meta*[m_tableMeta_arrayMems];
	if(!m_tableMeta_array){
		exit(1);
	}
	memset(m_tableMeta_array,0,m_tableMeta_arrayMems*sizeof(table_meta*));

	m_pOciQuery = new OciQuery();
	if(!m_pOciQuery){
		exit(1);
	}
	m_pOciQueryCols = new OciQuery();
	if(!m_pOciQueryCols){
		lgc_errMsg("new failed");
		exit(1);
	}

	const char* user = LGC_Param::getInstance()->getOciUser();
	const char* passwd = LGC_Param::getInstance()->getOciPasswd();
	const char* sid   = LGC_Param::getInstance()->getOciSid();

	m_pOciQuery->SetValses(sid,user,passwd);
	m_pOciQueryCols->SetValses(sid,user,passwd);

	return;
}

LGC_TblsMeta_Mgr::~LGC_TblsMeta_Mgr()
{
	for(int i=0; i< m_tableMeta_arrayMems; i++){
		if(m_tableMeta_array[i]){
			delete m_tableMeta_array[i];
			m_tableMeta_array[i] = NULL;
		}
	}
	
	delete[] m_tableMeta_array;
	m_tableMeta_array = NULL;

	if(m_pOciQuery){
		delete m_pOciQuery;
		m_pOciQuery = NULL;
	}

	if(m_pOciQueryCols){
		delete m_pOciQueryCols;
		m_pOciQueryCols = NULL;
	}

	return;
}

LGC_TblsMeta_Mgr* LGC_TblsMeta_Mgr::getInstance()
{
	if(!s_instance){
		lock();
		if(!s_instance){
			s_instance=new LGC_TblsMeta_Mgr;
			if(!s_instance){
				lgc_errMsg("new failed\n");
				exit(1);
			}
		}
		unlock();
	}
errOut:
	return s_instance;

}

bool  LGC_TblsMeta_Mgr::getTableName(DWORD objNum, char** pTableName)
{
	if( (objNum >= m_tableMeta_arrayMems) || (m_tableMeta_array[objNum] == NULL) ){
		lgc_errMsg("LGC_TblsMeta_Mgr::getTableName failed\n");
		return false;
	}
	
	if(m_tableMeta_array[objNum]->tableName[0] == 0){
		lgc_errMsg("LGC_TblsMeta_Mgr::getTableName failed\n");
		return false;
	}
	
	*pTableName  =  m_tableMeta_array[objNum]->tableName;

	return true;
}

bool  LGC_TblsMeta_Mgr::getTableMetaInfo(DWORD objNum, table_meta** ppTableMeta)
{
	if( (objNum >= m_tableMeta_arrayMems) || (m_tableMeta_array[objNum] == NULL) ){
		lgc_errMsg("LGC_TblsMeta_Mgr::getTableMetaInfo\n");
		return false;
	}

	if(m_tableMeta_array[objNum]->objNum != objNum){
		lgc_errMsg("LGC_TblsMeta_Mgr::getTableMetaInfo\n");
		return false;
	}

	*ppTableMeta = m_tableMeta_array[objNum];

	return true;
}


bool  LGC_TblsMeta_Mgr::getTableCols(DWORD objNum, WORD *pCols)
{
	if( (objNum >= m_tableMeta_arrayMems) || (m_tableMeta_array[objNum] == NULL) || m_tableMeta_array[objNum]->objNum != objNum){
		lgc_errMsg("LGC_TblsMeta_Mgr::getTableCols\n");
		return false;
	}

	*pCols = m_tableMeta_array[objNum]->cols;
	
	return true;
}


bool  LGC_TblsMeta_Mgr::getColName(DWORD objNum, WORD colNo, char** pColName)
{
	if( (objNum >= m_tableMeta_arrayMems) || (m_tableMeta_array[objNum] == NULL) 
		|| (colNo > m_tableMeta_array[objNum]->cols) ){
		lgc_errMsg("LGC_TblsMeta_Mgr::getColName \n");
		return false;
	}

	*pColName = m_tableMeta_array[objNum]->colMeta_array[colNo].colName;

	return true;
}



bool  LGC_TblsMeta_Mgr::getColDataType(DWORD objNum, WORD colNo, char** pColDataType)
{
	if( (objNum >= m_tableMeta_arrayMems) || (m_tableMeta_array[objNum] == NULL) ||
		 (colNo > m_tableMeta_array[objNum]->cols) ){
		lgc_errMsg("LGC_TblsMeta_Mgr::getColDataType \n");
		return false;
	}

	*pColDataType = m_tableMeta_array[objNum]->colMeta_array[colNo].dataType;

	return true;
}

static void eraseChr(string& _str, char c)
{
        char* str = new char[strlen(_str.data())+1];
        if(!str){
				lgc_errMsg("new failed\n");
                exit(1);
        }
        strcpy(str,_str.data());

        char *p, *q;

        p = str;
        q = str;

        for(q=str;*q != '\0'; q++){
                if(*q == c){

                }else{
                        *p++=*q;
                }
        }

        *p = '\0';
        _str.clear();
        _str = str;

        delete[] str;
        str = NULL;
}

bool LGC_TblsMeta_Mgr::loadTablesMetaInfo()
{
	bool bRet = true;
	string tableList = LGC_Param::getInstance()->getTableList();
	string tableExcludeList = LGC_Param::getInstance()->getExcludeTableList();
	eraseChr(tableList, ' ');
	eraseChr(tableExcludeList, ' ');
	
	char owner[126] = {0};
	char tableName[125] = {0};

	const char *p, *q, *tmp;
	p = tableList.data();
	q = tableList.data();
	
	p++;
	while(  *p != '\0' && (q = strchr(p,',')) != NULL){

		memset(owner,0,sizeof(owner));
		memset(tableName,0,sizeof(tableName));

		if( (tmp = strchr(p,'.')) == NULL){
			lgc_errMsg("tableList invalid\n");
			exit(1);
		}
		strncpy(owner, p, tmp-p);
		tmp++;
		strncpy(tableName,tmp, q-tmp);

		if(strcmp(tableName,"*") == 0){
			bRet = this->setTablesMetaInfo(owner);
		}else{
			bRet = this->setTableMetaInfo(owner, tableName);
		}
		if(!bRet){
			lgc_errMsg("setTableMetaInfo failed:owner=%s tableName=%s\n", owner, tableName);
			goto errOut;
		}

		p = ++q;
	}

	p = tableExcludeList.data();
	q = tableExcludeList.data();
	p++;
	while( *p != '\0' && (q = strchr(p,',')) != NULL){
		memset(owner,0,sizeof(owner));
		memset(tableName,0,sizeof(tableName));

		if( (tmp = strchr(p,'.')) == NULL ){
			lgc_errMsg("tableExclude invalid\n");
			exit(1);
		}
		strncpy(owner,p,tmp-p);
		tmp++;
		strncpy(tableName,tmp,q-tmp);

		if(*tmp == '*'){
			lgc_errMsg("tableExcludeList invalid\n");
			exit(1);
		}

		bRet = this->delTableMetaInfo(owner, tableName);
		if(!bRet){
			lgc_errMsg("delTableMetaInfo failed:owner=%s tableName=%s\n", owner, tableName);
			bRet = false;
			goto errOut;
		}
		p = ++q;
		fprintf(stdout, "exclude table: %s.%s\n", owner, tableName);
	}

errOut:
	return bRet;
}

bool LGC_TblsMeta_Mgr::setTablesMetaInfo(const char* owner)
{
	bool bRet = true;
	OciQuery& ociQuery = *m_pOciQuery;
	bRet = ociQuery.getTableMetaInfo(this,owner);
	if(!bRet){
		lgc_errMsg("setTablesMetaInfo failed:owner=%s\n", owner);
		goto errOut;
	}

errOut:
	return bRet;
}

bool LGC_TblsMeta_Mgr::setTableMetaInfo(const char* owner, const char* tableName)
{
	bool bRet = true;

	OciQuery& ociQuery = *m_pOciQuery;
	bRet = ociQuery.getTableMetaInfo(this,owner,tableName);
	if(!bRet){
		lgc_errMsg("setTableMetaInfo failed:onwer=%s tableName=%s\n", owner, tableName);
		goto errOut;
	}
errOut:
	return bRet;
}

bool LGC_TblsMeta_Mgr::delTableMetaInfo(const char* owner, const char* tableName)
{
	DWORD i = 0;

	for(i=0; i<m_tableMeta_arrayMems; i++){
		if ( m_tableMeta_array[i] != NULL){
			if(strcasecmp(owner,m_tableMeta_array[i]->owner) == 0 && strcasecmp(tableName, m_tableMeta_array[i]->tableName) == 0){
				delete m_tableMeta_array[i];
				m_tableMeta_array[i] = NULL;
			}
		}
	}
	return true;
}

bool LGC_TblsMeta_Mgr::setTableMetaInfo(const table_meta* pTableMeta_)
{
	bool bRet = true;
	
	table_meta* pTableMeta = (table_meta*)pTableMeta_;
	DWORD objNum = pTableMeta->objNum;
	
	bRet = m_pOciQueryCols->getTableCols(pTableMeta->owner,pTableMeta->tableName, &pTableMeta->cols);
	if(!bRet){
		lgc_errMsg("m_pOciQueryCols->getTableCols\n");
		bRet = false;
		goto errOut;
	}

	bRet = this->reallocTableMetaArray(objNum+1);
	if(!bRet){
		lgc_errMsg("reallocTableMetaArray\n");
		goto errOut;
	}

	bRet = this->freeTableMetaMem(objNum);
	if(!bRet){
		lgc_errMsg("freeTableMetaMem \n");
		goto errOut;
	}

	bRet = this->mellocTableMetaMem(objNum);
	if(!bRet){
		lgc_errMsg("mellocTableMetaMem \n");
		goto errOut;
	}
	


	memcpy(m_tableMeta_array[objNum],pTableMeta,sizeof(table_meta));

	m_tableMeta_array[objNum]->colMeta_array = new column_meta[m_tableMeta_array[objNum]->cols+1];
	if(!m_tableMeta_array[objNum]->colMeta_array){
		lgc_errMsg("new failed \n");
		bRet = false;
		goto errOut;
	}
	memset(m_tableMeta_array[objNum]->colMeta_array,0,(m_tableMeta_array[objNum]->cols+1)*sizeof(column_meta));


errOut:
	return bRet;
}

bool LGC_TblsMeta_Mgr::setColMetaInfo(DWORD objNum, const column_meta* pColMeta)
{
	if(objNum >= m_tableMeta_arrayMems){
		lgc_errMsg("LGC_TblsMeta_Mgr::setColMetaInfo failed: objNum=%u m_tableMeta_arrayMems=%u\n", objNum, m_tableMeta_arrayMems);
		return false;
	}

	bool bRet = true;
	table_meta *pTableMeta = NULL;
	
	pTableMeta = m_tableMeta_array[objNum];
	if(!pTableMeta){
		lgc_errMsg("LGC_TblsMeta_Mgr::setColMetaInfo failed \n");
		return false;
	}
	
	if(pColMeta->colNo > pTableMeta->cols){
		lgc_errMsg("LGC_TblsMeta_Mgr::setColMetaInfo failed \n");
		bRet = false;
		goto errOut;
	}

	pTableMeta->colMeta_array[pColMeta->colNo] = *pColMeta;

	

	fprintf(stdout, "objNum=%u owner=%s tableName=%s cols=%u colNo=%u colName=%s dataType=%s dataMaxLen=%u colNullAble=%u\n", 
		           pTableMeta->objNum, pTableMeta->owner, pTableMeta->tableName,pTableMeta->cols,pColMeta->colNo, pColMeta->colName, pColMeta->dataType, pColMeta->dataMaxLen,pColMeta->nullAble);
	fflush(stdout);

errOut:
	return bRet;
}




bool LGC_TblsMeta_Mgr::isFindOfObj(DWORD objNum)
{
	bool bFind = false;
	bFind = (objNum < m_tableMeta_arrayMems) && (m_tableMeta_array[objNum]) && (m_tableMeta_array[objNum]->objNum == objNum);
	return bFind;
}

bool LGC_TblsMeta_Mgr::reallocTableMetaArray(DWORD members_)
{	
	if(members_ <= m_tableMeta_arrayMems){
		return true;
	}

	bool bRet = true;
	const DWORD members = members_ + 10000;
	table_meta** pTableMetaArrayNew = NULL;
	
	pTableMetaArrayNew = new table_meta*[members];
	if(!pTableMetaArrayNew){
		lgc_errMsg("new failed \n");
		bRet = false;
		goto errOut;
	}
	memset(pTableMetaArrayNew,0,members*sizeof(table_meta*));

	memcpy(pTableMetaArrayNew,m_tableMeta_array, m_tableMeta_arrayMems*sizeof(table_meta*));

	delete[] m_tableMeta_array;

	m_tableMeta_array = pTableMetaArrayNew;
	m_tableMeta_arrayMems = members;

errOut:
	return bRet;
}


bool LGC_TblsMeta_Mgr::freeTableMetaMem(DWORD objNum)
{
	if(objNum >= m_tableMeta_arrayMems){
		lgc_errMsg("LGC_TblsMeta_Mgr::freeTableMetaMem failed \n");
		return false;
	}

	bool bRet = true;
	if( m_tableMeta_array[objNum] ){
		delete m_tableMeta_array[objNum];
		m_tableMeta_array[objNum] = NULL;
	}

errOut:
	return bRet;
}


bool LGC_TblsMeta_Mgr::mellocTableMetaMem(DWORD objNum)
{
	bool bRet = true;
	bRet = this->reallocTableMetaArray(objNum+1);
	if(!bRet){
		lgc_errMsg("LGC_TblsMeta_Mgr::mellocTableMetaMem failed \n");
		goto errOut;
	}

	if( m_tableMeta_array[objNum]){
		lgc_errMsg("LGC_TblsMeta_Mgr::mellocTableMetaMem failed \n");
		bRet = false;
		goto errOut;
	}

	m_tableMeta_array[objNum] = new table_meta();
	if(!m_tableMeta_array[objNum]){
		lgc_errMsg("LGC_TblsMeta_Mgr::mellocTableMetaMem failed \n");
		bRet = false;
		goto errOut;
	}


errOut:
	return bRet;
}


