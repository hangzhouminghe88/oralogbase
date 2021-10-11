#include "lgc_DmlRowParser.h"
#include "lgc_DmlRow.h"
#include "lgc_MediaFileOutput.h"
#include "lgc_tableMeta.h"
#include "lgc_ora_convert.h"
#include "lgc_api.h"


//....................................................
//class LGC_DmlRowParser
//功能: 解析dmlRow的数据， 并输出
//....................................................

Mutex LGC_DmlRowParser::s_mutex;
LGC_DmlRowParser* LGC_DmlRowParser::s_instance = NULL;

//constructor and desctructor

/*
*构造函数
*/
LGC_DmlRowParser::LGC_DmlRowParser()
{
	return;
}

/*
*析构函数
*/
LGC_DmlRowParser::~LGC_DmlRowParser()
{
	return;
}

//public member functions

/*
*解析dmlRow的数据，并用用pMediaFileOutput输出
*/
int LGC_DmlRowParser::parse(const LGC_DmlRow* pDmlRow, LGC_MediaFileOutput* pMediaFileOutput)
{
	const dml_header& dmlHeader = pDmlRow->getDmlHeader();
	const list<LGC_DmlColumn*>& redoColList = pDmlRow->getRedoColList();
	const list<LGC_DmlColumn*>& undoColList = pDmlRow->getUndoColList();

	const DWORD objNum = dmlHeader.objNum;
	const unsigned char dmlType = dmlHeader.dmlType;
	
	//获取表的元数据
	table_meta* pTableMeta = NULL;
	if(false == LGC_TblsMeta_Mgr::getInstance()->getTableMetaInfo(objNum, &pTableMeta)){
		lgc_errMsg("getTableMetaInfo failed \n");
		return -1;
	}
	if(pTableMeta == NULL){
		lgc_errMsg("pTableMeta is NULL \n");
		return -1;
	}
	
	//根据dmlType调用解析相应的解析方法
	switch(dmlType){
		case DML_INSERT:
		{
			return this->parseInsertDmlRow(*pTableMeta,dmlHeader,redoColList, undoColList, pMediaFileOutput );
		}
		break;
		case DML_DELETE:
		{
			return this->parseDeleteDmlRow(*pTableMeta,dmlHeader,redoColList, undoColList, pMediaFileOutput );
		}
		break;
		case DML_UPDATE:
		{
			return this->parseUpdateDmlRow(*pTableMeta,dmlHeader,redoColList, undoColList, pMediaFileOutput );
		}
		break;
		default:
		{
			lgc_errMsg("dmlType Unkown \n");
			return -1;
		}
	}

	//never to here
	lgc_errMsg("never to here\n");
	return -1;

}

//private member functions

/*
*解析出InsertDmlRow的数据，并用pMediaFileOutput输出
*/
int 
LGC_DmlRowParser::parseInsertDmlRow(const table_meta&			tableMeta, 
                                    const dml_header&			dmlHeader, 
                                    const list<LGC_DmlColumn*>& redoColList, 
									const list<LGC_DmlColumn*>& undoColList, 
									LGC_MediaFileOutput*        pMediaFileOutput
									)
{
	string tableName = LGC_DmlRowParser::getTableName(tableMeta);
	const int size = redoColList.size();

	string dmlText;

	dmlText += "insert into ";
	dmlText += tableName;
	dmlText += " ( ";
	
	list<LGC_DmlColumn*>::const_iterator it;
	int i = 0;
	for( it  = redoColList.begin(), 
		 i   = 0;
		 it != redoColList.end();
		 it++, i++)
	{
			 
		LGC_DmlColumn* pCol = *it;
		dmlText += LGC_DmlRowParser::getColName(tableMeta, *pCol);
		if(i != size - 1){//not last
			dmlText += ", ";
		}	 
	}

	dmlText += " ) values";
	
	dmlText += "( ";
	
	for(it = redoColList.begin(),
		i = 0;
		it != redoColList.end();
		it++, i++)
	{
		LGC_DmlColumn* pCol = *it;

		dmlText += LGC_DmlRowParser::getColValue(tableMeta, *pCol);
		if(i != size - 1){
			dmlText += ", ";
		}		 
	}
	
	dmlText += " )";

	dmlText +=";";

	if(0 > pMediaFileOutput->writeLine(dmlText.data())){
		lgc_errMsg("writeLine failed \n");
		return -1;
	}
	
	//success
	return 0;
}


/*
*解析出deleteDmlRow的数据， 并用pMediaFileOutput输出
*/
int 
LGC_DmlRowParser::parseDeleteDmlRow(const table_meta& tableMeta, 
                                    const dml_header& dmlHeader, 
									const list<LGC_DmlColumn*>& redoColList, 
									const list<LGC_DmlColumn*>& undoColList,
									LGC_MediaFileOutput*        pMediaFileOutput
									)
{
	string tableName = LGC_DmlRowParser::getTableName(tableMeta);
	const int size = undoColList.size();

	string dmlText = "delete from ";
	dmlText += tableName;
	dmlText += " where ";

	list<LGC_DmlColumn*>::const_iterator it;
	int i = 0;
	for(it = undoColList.begin(),
		i = 0;
		it != undoColList.end();
		it++, i++)
	{
		LGC_DmlColumn* pCol = *it;

		dmlText += LGC_DmlRowParser::getColName(tableMeta,  *pCol);
		dmlText += "=";
		dmlText += LGC_DmlRowParser::getColValue(tableMeta, *pCol);
		
		if(i != size -1 ){//not last
			dmlText += " and ";
		}
	}

	dmlText += ";";

	if(0 > pMediaFileOutput->writeLine(dmlText.data()) ){
		lgc_errMsg("writeLine failed \n");
		return -1;
	}

	return 0;
}

/*
*解析出UpdateDmlRow的数据，并用pMediaFileOutput输出
*/
int 
LGC_DmlRowParser::parseUpdateDmlRow(const table_meta& tableMeta, 
                                    const dml_header& dmlHeader, 
									const list<LGC_DmlColumn*>& redoColList, 
									const list<LGC_DmlColumn*>& undoColList,
									LGC_MediaFileOutput*        pMediaFileOutput
									)
{
	string tableName = LGC_DmlRowParser::getTableName(tableMeta);
	const int redoColListSize = redoColList.size();
	const int undoColListSize = undoColList.size();

	string dmlText = "update ";
	dmlText += tableName;
	dmlText += " set ";
	
	list<LGC_DmlColumn*>::const_iterator it;
	int i = 0;
	for(it = redoColList.begin(),
		i = 0;
		it != redoColList.end();
		it++, i++)
	{
		LGC_DmlColumn* pCol = *it;

		dmlText += LGC_DmlRowParser::getColName(tableMeta,  *pCol);
		dmlText += "=";
		dmlText += LGC_DmlRowParser::getColValue(tableMeta, *pCol);
		
		if(i != redoColListSize - 1){//not last
			dmlText += ", ";
		}
	}

	dmlText += " where ";
	
	for(it = undoColList.begin(),
		i = 0;
		it != undoColList.end();
		it++, i++)
	{
		
		LGC_DmlColumn* pCol = *it;

		dmlText += LGC_DmlRowParser::getColName(tableMeta,  *pCol);
		dmlText += "=";
		dmlText += LGC_DmlRowParser::getColValue(tableMeta, *pCol);
		
		if(i != undoColListSize - 1){//not last
			dmlText += " and ";
		}
	}

	dmlText += ";";

	if(0 > pMediaFileOutput->writeLine(dmlText.data())){
		lgc_errMsg("writeline failed \n");
		return -1;
	}

	return 0;
}

//static member functions

/*
*获取单实例模式下的唯一对象
*/
LGC_DmlRowParser* 
LGC_DmlRowParser::getInstance()
{
	if(s_instance == NULL){
		s_mutex.Lock();
		if(s_instance == NULL){
			s_instance = new LGC_DmlRowParser;
			if(s_instance == NULL){
				exit(1);
			}
		}
		s_mutex.Unlock();
	}
	return NULL;
}


//private static member functions

/*
*获取tableName
*/
const string 
LGC_DmlRowParser::getTableName(const table_meta& tableMeta)
{
	string tableName;

	tableName += tableMeta.owner;
	tableName += ".";
	tableName += tableMeta.tableName;

	return tableName;
}

/*
*获取列名
*/
const string 
LGC_DmlRowParser::getColName(const table_meta& tableMeta, 
                             const LGC_DmlColumn& dmlCol
							)
{
	const unsigned short colNo = dmlCol.getColNo();
	
	if(!lgc_check(colNo >= 1 && colNo <= tableMeta.cols ))
	{
		lgc_errMsg("colNo invalid \n");
		exit(1);
	}

	string colName;
	colName += tableMeta.colMeta_array[colNo].colName;
	return colName;
}

/*
*获取列的值
*/
const string 
LGC_DmlRowParser::getColValue(const table_meta& tableMeta, 
                              const LGC_DmlColumn& dmlCol
							  )
{
	const unsigned short colNo = dmlCol.getColNo();
	
	if(!lgc_check(colNo >= 1 
		          && colNo <= tableMeta.cols))
	{
		lgc_errMsg("colNo invalid \n");
		exit(1);
	}
	
	const column_meta& colMeta = tableMeta.colMeta_array[colNo]; 
	const char* colDataType = colMeta.dataType;

	const char* colData     = dmlCol.getColData();
	const unsigned short colDataLen = dmlCol.getColDataLen();

	string colValue;
	
	COL_DATA colData_converted;
	if(strcmp(colDataType, "DATE") == 0){//date

		raw_to_date((ub1*)colData, colDataLen, &colData_converted);
		
		colValue += "to_date('";
		colValue += colData_converted.common_data.data;
		colValue +="','yyyy-mm-dd hh24:mi:ss'";
		colValue += ")";

	}else if(strcmp_igNum(colDataType, "TIMESTAMP()") == 0){//timestamp

		ub2 scale = getScaleInTimestamp(colDataType);
		
		raw_to_timestamp((ub1*)colData, colDataLen, &colData_converted, scale);

		colValue += "to_timestamp('";
		colValue += colData_converted.common_data.data;
		colValue += "', 'yyyy-mm-dd hh24:mi:ss.ff')";

	}else if(strcmp_igNum(colDataType, "TIMESTAMP() WITH LOCAL TIME ZONE") == 0 ){//timestamp with local time zone
		
		ub2 scale = getScaleInTimestamp(colDataType);

		raw_to_timestamp_ltz((ub1*)colData, colDataLen, &colData_converted, scale);

		colValue += "to_timestamp_tz('";
		colValue += colData_converted.common_data.data;
		colValue += "', 'YYYY-MM-DD HH24:MI:SS.FF TZH:TZM')";

	}else if(strcmp_igNum(colDataType, "TIMESTAMP() WITH TIME ZONE") == 0){
		ub2 scale = getScaleInTimestamp(colDataType);
		
		raw_to_timestamp_tz((ub1*)colData, colDataLen, &colData_converted, scale);

		colValue += "to_timestamp_tz('";
		colValue += colData_converted.common_data.data;
		colValue += "', 'YYYY-MM-DD HH24:MI:SS.FF TZH:TZM')";

	}else if(strcmp_igNum(colDataType, "INTERVAL YEAR() TO MONTH") == 0){

		raw_to_interval_ym((ub1*)colData, colDataLen, &colData_converted);

		colValue +="'";
		colValue +=colData_converted.common_data.data;
		colValue +="'";

	}else if(strcmp_igNum(colDataType, "INTERVAL DAY() TO SECOND()") == 0){

		raw_to_interval_ds((ub1*)colData, colDataLen, &colData_converted);
		
		colValue += "TO_DSINTERVAL('";
		colValue +=colData_converted.common_data.data;
		colValue += "')";

	}else if(strcmp(colDataType,"NUMBER") == 0){//number
		
		raw_to_number((ub1*)colData, colDataLen, &colData_converted);

		colValue += colData_converted.common_data.data;

	}else if(strcmp(colDataType,"BINARY_FLOAT") == 0){

		raw_to_binary_float((ub1*)colData, colDataLen, &colData_converted);

		colValue += colData_converted.common_data.data;

	}else if(strcmp(colDataType,"BINARY_DOUBLE") == 0){

		raw_to_binary_double((ub1*)colData, colDataLen, &colData_converted);

		colValue += colData_converted.common_data.data;

	}else if(strcmp(colDataType, "VARCHAR2") == 0 || strcmp(colDataType, "CHAR") == 0){

		raw_to_char((ub1*)colData, colDataLen, &colData_converted);
		
		colValue += "'";
		colValue += colData_converted.common_data.data;
		colValue +="'";

	}else if(strcmp(colDataType, "LONG") == 0){

		raw_to_char((ub1*)colData, colDataLen, &colData_converted);

		colValue += "'";
		colValue += colData_converted.common_data.data;
		colValue +="'";

	}else if(strcmp(colDataType,"RAW") == 0){

		raw_to_hex((ub1*)colData, colDataLen, &colData_converted);

		colValue +="hextoraw(";
		colValue += colData_converted.common_data.data;
		colValue += ")";

	}else if(strcmp(colDataType, "LONG RAW") == 0){

		raw_to_hex((ub1*)colData, colDataLen, &colData_converted);

		colValue +="hextoraw('";
		colValue += colData_converted.common_data.data;
		colValue += "')";

	}else if(strcmp(colDataType, "ROWID") == 0){

		raw_to_rowid((ub1*)colData, colDataLen, &colData_converted);

		colValue += "'";
		colValue += colData_converted.common_data.data;
		colValue += "'";

	}else if( strcmp(colDataType, "CLOB") == 0){
		
		colValue += "EMPTY_CLOB()";

	}else if(strcmp(colDataType, "BLOB") == 0){

		colValue += "EMPTY_BLOB()";

	}else{
		lgc_errMsg("this col type not surpported:%s\n", colDataType);
		exit(1);
	}
	
	return colValue;
}
