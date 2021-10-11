#include "lgc_api.h"
#include "lgc_OpcodeParser.h"
#include "lgc_Change.h"
#include "lgc_DmlRowsOutput.h"

#define COLDATA_NOTFINISH 0
#define COLDATA_FINISH	  1

/*
*解析change vector里面的opcode
*提取出里面的dml columns data
*/

//.......................................................
//abstract class LGC_OpcodeParser
//一个dml行由多个列组成, dml行可能跨多个数据块，但是一个LGC_OpcodeParser只解解析做同一个数据块中的列数据
//.......................................................

//constructor and desctructor

/*
*构造函数
*/
LGC_OpcodeParser::LGC_OpcodeParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas)
{
	m_pChange = pChange;
	m_lenArray = lenArray;
	m_dataArray = dataArray;
	m_datas = datas;

	return;
}

/*
*析构函数
*/
LGC_OpcodeParser::~LGC_OpcodeParser()
{
	return;
}

//virtual member functions
/*
*解析出列数据，并用pDmlRowsOutput输出
*/
int LGC_OpcodeParser::generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	return 0;
}

/*
*该opcode占用了change vector的dataArray里的多少个data
*/
unsigned short LGC_OpcodeParser::getDatas()
{
	return 0;
}

//子类不该隐藏或者覆盖这些函数
/*
*该opcode包含了多列的数据，该函数获取起始列的列号
*
*/
unsigned short LGC_OpcodeParser::getStartColNo()
{
	return m_pChange->getStartColNo();
}
	
//static public member functions

/*
*创建OpcodeParser
*/
LGC_OpcodeParser* LGC_OpcodeParser::createOpcodeParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas)
{
	ChangeType changeType = pChange->getChangeType();

	LGC_OpcodeParser* pOpcodeParser = NULL;

	switch(changeType){
	case UNKOWN_UNDOCHANGE:
	case UNKOWN_CHANGE:
	case BEGINT_CHANGE:
	case ENDT_CHANGE:
	case MULTIINSERT_UNDOCHANGE:// 无需解析
	case MULTIDELETE_UNDOCHANGE:// 无需解析
		{
			return NULL;
		}
		break;
	case INSERT_UNDOCHANGE:
		{//create OpKdoirpParser
			pOpcodeParser = LGC_OpKdoirpParser::createOpKdoirpParser(pChange,lenArray,dataArray,datas);
		}
		break;
	case DELETE_UNDOCHANGE:
		{//create OpKdodrpParser
			pOpcodeParser = LGC_OpKdodrpParser::createOpKdodrpParser(pChange,lenArray,dataArray,datas);
		}
		break;
	case UPDATE_UNDOCHANGE: 
		{//create OpKdourpParser
			pOpcodeParser = LGC_OpKdourpParser::createOpKdourpParser(pChange,lenArray,dataArray,datas);
		}
		break;	
	case ROWCHAIN_UNDOCHANGE:
		{//create OpKdoirpParser
			pOpcodeParser = LGC_OpKdoirpParser::createOpKdoirpParser(pChange,lenArray,dataArray,datas);
		}
		break;
	case MFC_UNDOCHANGE:
		{//create KdomfcParser
			pOpcodeParser = LGC_OpKdomfcParser::createOpKdomfcParser(pChange,lenArray,dataArray,datas);
		}
		break;	
	case LMN_UNDOCHANGE://
		{
			pOpcodeParser = LGC_OpKdolmnParser::createOpKdolmnParser(pChange,lenArray,dataArray,datas);
		}
		break;
	case INSERT_REDOCHANGE:
		{//create KdoirpParser
			pOpcodeParser = LGC_OpKdoirpParser::createOpKdoirpParser(pChange,lenArray,dataArray,datas);
		}
		break;	
	case DELETE_REDOCHANGE:
		{//create KdodrpParser
			pOpcodeParser = LGC_OpKdodrpParser::createOpKdodrpParser(pChange,lenArray,dataArray,datas);
		}
		break;	
	case UPDATE_REDOCHANGE:
		{//create KdourpParser
			pOpcodeParser = LGC_OpKdourpParser::createOpKdourpParser(pChange,lenArray,dataArray,datas);
		}
		break;		
	case ROWCHAIN_REDOCHANGE:
		{//create KdoirpParser
			pOpcodeParser = LGC_OpKdoirpParser::createOpKdoirpParser(pChange,lenArray,dataArray,datas);
		}
		break;	
	case MULTIINSERT_REDOCHANGE:
		{//create KdoqmParser
			pOpcodeParser = LGC_OpKdoqmParser::createOpKdoqmParser(pChange,lenArray,dataArray,datas);
		}
		break;
	default:
		{//error
			return NULL;
		}
		break;
	}
	
	//success
	return pOpcodeParser;
}


//................................................
//class LGC_OpKdoirpParser
//向一个数据块中插入列数据时产生的操作码;
//第一个data是log_opcode_kdoirp结构 
//接下来的几个data是列数据
//................................................

//constructor and desctructor

/*
*构造函数
*/
LGC_OpKdoirpParser::LGC_OpKdoirpParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas)
						:LGC_OpcodeParser( pChange, lenArray, dataArray, datas)
{
	return;
}

/*
*析构函数
*/
LGC_OpKdoirpParser::~LGC_OpKdoirpParser()
{
	return;
}

//virtual public member funcitos 

/*
*解析出列数据，并用pDmlRowOutput输出
*/
int LGC_OpKdoirpParser::generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	log_opcode_kdoirp* pLogOpKdoirp = (log_opcode_kdoirp*)m_dataArray[0];
	const WORD cols = pLogOpKdoirp->cc;
	//数据块中第一列数据是否完整
	const bool bNeedData_inPrevBlk = (pLogOpKdoirp->flag & 0x02);
	//最后一列数据是否完整
	const bool bNeedData_inNextBlk = (pLogOpKdoirp->flag & 0x01);
	const unsigned short startColNo = this->getStartColNo();

	unsigned short colNo = 0;
	unsigned int colLen = 0;
	char* colData = NULL;

	for(int i=0; i < cols; i++){
		colNo = startColNo + i;
		colLen = m_lenArray[1+i];
		colData = m_dataArray[1+i];
		
		//write column data
		if(i == cols - 1 && bNeedData_inNextBlk){//last
			if( 0 > pDmlRowsOutput->writeOneColumn(colData, colLen, colNo, COLDATA_NOTFINISH) ){
				lgc_errMsg("writeOneColumn failed \n");
				return -1;
			}		
			continue;
		}

		if( 0 > pDmlRowsOutput->writeOneColumn(colData, colLen, colNo, COLDATA_FINISH) ){
			lgc_errMsg("writeOneColumn failed \n");
			return -1;
		}
	}

	//success 
	return 0;
}

/*
*OpKdoirpParser占了多少个Data
*/
unsigned short LGC_OpKdoirpParser::getDatas()
{
	log_opcode_kdoirp* pLogOpKdoirp = (log_opcode_kdoirp*)m_dataArray[0];
	const WORD cols = pLogOpKdoirp->cc;
	return (1+cols);
}

//private functions

/*
*检查合法性
*/
bool LGC_OpKdoirpParser::check()
{
	return (m_lenArray[0] >= sizeof(log_opcode_kdoirp)
		    && this->getDatas() <= m_datas);
}

//static member functions

/*
*创建OpKdoirpParser
*/
LGC_OpKdoirpParser* 
LGC_OpKdoirpParser::createOpKdoirpParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas)
{
	LGC_OpKdoirpParser* pOpKdoirpParser = new LGC_OpKdoirpParser(pChange,lenArray,dataArray,datas);
	if(pOpKdoirpParser == NULL){
		lgc_errMsg("new failed \n");
		return NULL;
	}

	if(!pOpKdoirpParser->check()){
		lgc_errMsg("check failed \n");
		delete pOpKdoirpParser;
		pOpKdoirpParser = NULL;
		return NULL;
	}

	//success
	return pOpKdoirpParser;
}



//................................................
//class LGC_OpKdodrpParser
//删除数据块里的列数据时 产生的opcode;
//第一个data是log_opcode_kdodrp，
//接下来没有数据了;
//................................................

//constructor and desctructor

/*
*构造函数
*/
LGC_OpKdodrpParser::LGC_OpKdodrpParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas)
						:LGC_OpcodeParser( pChange, lenArray, dataArray, datas)
{
	return;
}

/*
*析构函数
*/
LGC_OpKdodrpParser::~LGC_OpKdodrpParser()
{
	return;
}

//virtual public member funcitos

/*
*解析出列数据， 并用pDmlRowsOutput输出
*/
int LGC_OpKdodrpParser::generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	log_opcode_kdodrp* pLogKdodrp = (log_opcode_kdodrp*)m_dataArray[0];

	//success 
	return 0;
}

/*
*OpKdodrpＰarser占用了多少个Data
*/
unsigned short LGC_OpKdodrpParser::getDatas()
{
	return 1;
}

//private functions

/*
*检验合法性
*/
bool LGC_OpKdodrpParser::check()
{
	return (m_lenArray[0] >= sizeof(log_opcode_kdodrp) 
		    && this->getDatas() <= m_datas );
}


//static member functions

/*
*创建OpKdoirpdrpParser
*/
LGC_OpKdodrpParser* 
LGC_OpKdodrpParser::createOpKdodrpParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas)
{
	LGC_OpKdodrpParser* pOpKdodrpParser = new LGC_OpKdodrpParser(pChange,lenArray,dataArray,datas);
	if(pOpKdodrpParser == NULL){
		lgc_errMsg("new failed \n");
		return NULL;
	}

	if(!pOpKdodrpParser->check() ){
		lgc_errMsg("check failed \n");
		delete pOpKdodrpParser;
		pOpKdodrpParser = NULL;
		return NULL;
	}

	//success
	return pOpKdodrpParser;
}

//................................................
//class LGC_OpKdourpParser
//更新数据块里几列数据 产生的操作码
//第一个data是log_opcode_kdourp
//第二个data是被更新的列的相对列号列表
//接下来的data主要是列数据，也有可能包含其他数据
//................................................

//constructor and desctructor

/*
*构造函数
*/
LGC_OpKdourpParser::LGC_OpKdourpParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas)
						:LGC_OpcodeParser( pChange, lenArray, dataArray, datas)
{
	return;
}

/*
*析构函数
*/
LGC_OpKdourpParser::~LGC_OpKdourpParser()
{
	return;
}

//virtual public member funcitos 

/*
*解析出列数据， 并用pDmlRowsOutput输出
*/
int LGC_OpKdourpParser::generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	const log_opcode_kdourp* pLogKdourp = (log_opcode_kdourp*)m_dataArray[0];
	BYTE newCols = pLogKdourp->nnew;
	const BYTE totalCols = pLogKdourp->ncol;

	if(newCols == 0){
		return 0;
	}

	const WORD* colNo_array = (const WORD*)m_dataArray[1];
	const WORD  colNoArrayLen = m_lenArray[1];

	const WORD startColNo_blk = colNo_array[0];
	const WORD minColNo_blk = startColNo_blk;
	const WORD maxColNo_blk = colNo_array[newCols-1];

	const bool bNeedData_inPrevBlk = ((minColNo_blk == 0) && (pLogKdourp->flag & 0x02));
	const bool bNeedData_inNextBlk = ((maxColNo_blk == totalCols-1) && (pLogKdourp->flag & 0x01));

	const unsigned short startColNo = this->getStartColNo();


	
	if(pLogKdourp->size == 0xffff){
		return 0;
	}

	if(pLogKdourp->xtype & 0x80 ){//所有列的内容放在了一起
		//暂不支持
		lgc_errMsg("pLogKdourp->xtype & 0x80 not support \n");
		return -1;
	}
	
	if( !LGC_OpKdourpParser::checkColNoArray(colNo_array,colNoArrayLen,newCols) ){
		lgc_errMsg("checkColNoArray failed \n");
		return -1;
	}

	newCols = LGC_OpKdourpParser::getIncrementalColsOfKdourp(colNo_array,newCols);
	
	unsigned short dataOffset = 0;
	if(pLogKdourp->opcode & 0x40){
		//add one data of dscn		
		dataOffset = 2+1;
	}else{
		dataOffset = 2;
	}

	unsigned short colNo = 0;
	unsigned int colLen = 0;
	char* colData = NULL;
	for(int i = 0; i < newCols; i++){
		colNo = (colNo_array[i] - startColNo_blk) + startColNo;
		colLen = m_lenArray[dataOffset+i];
		colData = m_dataArray[dataOffset+i];
		
		//write one column data 
		if( i == newCols -1 && bNeedData_inNextBlk ){//最后一列数据不完整
			if( 0 > pDmlRowsOutput->writeOneColumn(colData,colLen,colNo, COLDATA_NOTFINISH) ){
				lgc_errMsg("writeOneColumn failed\n");
				return -1;	
			}
			continue;
		}

		if( 0 > pDmlRowsOutput->writeOneColumn(colData,colLen,colNo, COLDATA_FINISH) ){
			lgc_errMsg("writeOneColumn failed \n");
			return -1;	
		}
	}

	//success 
	return 0;
}

/*
*OpKdourpParser包含多少data
*/
unsigned short LGC_OpKdourpParser::getDatas()
{
	unsigned short datas = 0;

	const log_opcode_kdourp* pLogKdourp = (log_opcode_kdourp*)m_dataArray[0];
	BYTE newCols = pLogKdourp->nnew;
	const BYTE totalCols = pLogKdourp->ncol;
	const WORD* colNo_array = (const WORD*)m_dataArray[1];
	const WORD  colNoArrayLen = m_lenArray[1];

	if(pLogKdourp->xtype & 0x80 ){//所有列的内容放在了一起
		//暂不支持
		lgc_errMsg("pLogKdourp->xtype & 0x80 not support \n");
		exit(1);
	}

	if(pLogKdourp->size == 0xffff){//有列数据，但是这些列数据无效
									//所以计算data的数量时，要考虑这些无效列数据
	}

	if( !LGC_OpKdourpParser::checkColNoArray(colNo_array,colNoArrayLen, newCols) ){
		lgc_errMsg("checkColNoArray failed \n");
		exit(1);
	}
	newCols = LGC_OpKdourpParser::getIncrementalColsOfKdourp(colNo_array,newCols);

	if(pLogKdourp->opcode & 0x40){
		//add one data of dscn		
		datas = 2+1+newCols;
	}else{
		datas = 2+newCols;
	}

	return datas;
}


//private functions

/*
*检验合法性
*/
bool LGC_OpKdourpParser::check()
{
	return (m_datas >= 2 
		    && m_lenArray[0] >= sizeof(log_opcode_kdourp)
		    && m_lenArray[1] % sizeof(unsigned short) == 0
			&& this->getDatas() <= m_datas);
}


//static functions

/*
*创建OpKdourpParser
*/
LGC_OpKdourpParser* 
LGC_OpKdourpParser::createOpKdourpParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas)
{
	LGC_OpKdourpParser* pOpKdourpParser = new LGC_OpKdourpParser(pChange,lenArray,dataArray,datas);
	if(pOpKdourpParser == NULL){
		return NULL;
	}

	if(!pOpKdourpParser->check()){
		delete pOpKdourpParser;
		pOpKdourpParser = NULL;
		return NULL;
	}

	//success
	return pOpKdourpParser;
}

/*
*检验colNoArray
* colNoArray里的列号严格递增然后严格递减
* 严格递增部分是有效列，严格递减部分是无效列
*/
bool LGC_OpKdourpParser::checkColNoArray(const WORD* colNo_array, const WORD colNoArrayLen, const BYTE newCols)
{
	int prev = 0;
	int i = 0;

	if(colNoArrayLen % sizeof(WORD) != 0 
		|| colNoArrayLen < sizeof(WORD)*newCols){
		lgc_errMsg("newCols invalid \n");
		return false;
	}

	if(newCols <=1){
		return true;
	}

	for(prev=0, i=1;
	    i<newCols && colNo_array[prev] < colNo_array[i]; 
		i++, prev++)
	{

	}

	
	if(i == newCols){
		return true;
	}else{
		if(colNo_array[prev] == colNo_array[i]){
			return false;
		}
	}

	for(;
	    i<newCols && colNo_array[prev] > colNo_array[i];
		i++,prev++)
	{
	}

	if(i != newCols){
		return false;
	}else{
		return true;
	}

	return true;
}

/*
* 计算列号严格递增的列的个数，列号应该严格递增然后严格递减
* 严格递增列的个数就是该kdourp操作的列的个数
*/
unsigned char LGC_OpKdourpParser::getIncrementalColsOfKdourp(const WORD* colNo_array, const BYTE newCols)
{
	int prev = 0;
	int i = 0;

	if(newCols <=1){
		return newCols;
	}

	for(prev=0, i=1;
	    i<newCols && colNo_array[prev] < colNo_array[i]; 
		i++, prev++)
	{

	}

	return i;

}


//................................................
//class LGC_OpKdoqmParser
//多行的数据直接插入到同一个数据块中时产生的操作码
//第一个data是log_opcode_kdoqm
//第二个data是rowLen_array
//第三个data是rowsData
//接下来没有了
//................................................

//constructor and desctructor
/*
*构造函数
*/
LGC_OpKdoqmParser::LGC_OpKdoqmParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas)
						:LGC_OpcodeParser( pChange, lenArray, dataArray, datas)
{
	return;
}

/*
*析构函数
*/
LGC_OpKdoqmParser::~LGC_OpKdoqmParser()
{
	return;
}

//virtual public member funcitos 

/*
*解析出列数据，并用pDmlRowsOutput输出
*/
int LGC_OpKdoqmParser::generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	//should never be called
	lgc_errMsg("should never be called \n");
	exit(1);
	return 0;
}

/*
*解析出行数据， 并用pDmlRowsOutput输出
*/
int LGC_OpKdoqmParser::generateRowsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	const log_opcode_kdoqm* pOpKdoqm = (log_opcode_kdoqm*)m_dataArray[0];
	const WORD* rowLen_array = (WORD*)m_dataArray[1];
	const BYTE rows = pOpKdoqm->nrow;
	const BYTE rowSlotHeaderLen = 3;
	const char* pRowData = m_dataArray[2];
	
	if(m_pChange->isUndoChange() ){
		//udno change不应该调用generateRowsData
		lgc_errMsg("udno change have no generateRowsData function\n");
		exit(1);
	}
	
	dml_header dmlHeader = pDmlRowsOutput->getCurDmlHeader();

	BYTE flg = 0;
	BYTE cols = 0;
	
	WORD colNo = 0;
	WORD colLen = 0;
	const char* colData = NULL;

	for(int i=0; i<rows; i++){
		flg = pRowData[0];
		cols = pRowData[2];
		if( (flg&0x10) || (flg&0x40) ){//the row deleted or is a cluster row,is not useful
			continue;
		}else{//row data is useful
		}

		pRowData += rowSlotHeaderLen;

		if(i != 0){//firt row's  dmlHeader have write
			if( sizeof(dmlHeader) != pDmlRowsOutput->writeDmlHeader(&dmlHeader,sizeof(dmlHeader)) ){
				lgc_errMsg("writeDmlHeader failed\n");
				return -1;
			}
			pDmlRowsOutput->switchToRedo();
		}
		
		for(int j=0; j < cols; j++){
			colLen = *((BYTE*)pRowData);
			pRowData += 1;//skip column len
			colNo = j+1;

			if(colLen == 255){//col len is zero
				//写一列的数据，该列数据为空
				if(0 > pDmlRowsOutput->writeOneColumn(NULL,0,colNo, COLDATA_FINISH) ){
					lgc_errMsg("writeOneColumn failed \n");
					return -1;
				}
				continue;
			}else if(colLen == 254){//one byte can't describe the column length
				colLen =*((WORD*)(pRowData));				
				pRowData += 2;//skip column len
			}else{//one byte can describe the column length		
			}

			if( (flg&0x08) && !(flg&0x20) ){//skip log_redo_rid, then col data
				pRowData += sizeof(log_redo_rid);
			}
			
			if( (pRowData + colLen - m_dataArray[2]) > m_lenArray[2]){//越界
				lgc_errMsg("pRowData yuejie: rowNo=%u colNo=%u colLen=%u m_lenArray[2]=%u \n",i+1, colNo, colLen, m_lenArray[2]);
				return -1;
			}
			colData = pRowData;
			if(0 > pDmlRowsOutput->writeOneColumn(colData,colLen,colNo,COLDATA_FINISH)){
				lgc_errMsg("writeOneColumn failed \n");
				return -1;
			}

			pRowData += colLen;
			
			if( (pRowData - m_dataArray[2]) > m_lenArray[2]){//越界
				lgc_errMsg("pRowData yuejie: colLen=%u m_lenArray[2]=%u \n",colLen, m_lenArray[2]);
				return -1;
			}


		}//end for inner

		if( 0 > pDmlRowsOutput->writeDmlEndInfo(NULL,0) ){
			lgc_errMsg("writeDmlEndInfo failed\n");
			return -1;
		}

	}//end for outer

	//success 
	return 0;
}

/*
*获取OpKdoqmParser占用的data数
*/
unsigned short LGC_OpKdoqmParser::getDatas()
{
	return 3;
}

//private functions

/*
*检验OpKdoqm的属性的合法性
*/
bool LGC_OpKdoqmParser::check()
{

	if(!lgc_check( m_pChange
		          && m_pChange->isUndoChange()  == false
		          && m_datas                    >= 2 
		          && m_lenArray[0]              >= sizeof(log_opcode_kdoqm)
		          && m_lenArray[1]%sizeof(WORD) == 0
		          )){
		lgc_errMsg("m_lenArray[0] < sizeof(log_opcode_kdoqm) \n");
		return false;
	}

	const log_opcode_kdoqm* pOpKdoqm = (log_opcode_kdoqm*)m_dataArray[0];
	const WORD* rowLen_array = (WORD*)m_dataArray[1];
	const BYTE rows = pOpKdoqm->nrow;
	
	if(!lgc_check(m_lenArray[1] >= sizeof(WORD)*rows 
		          && this->getDatas() <= m_datas)){
		lgc_errMsg("check failed \n");
		return false;
	}

	return true;
}

//static member functions

/*
*创建OpKdoqmParser
*/
LGC_OpKdoqmParser* 
LGC_OpKdoqmParser::createOpKdoqmParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas)
{
	LGC_OpKdoqmParser* pOpKdoqmParser = new LGC_OpKdoqmParser(pChange,lenArray,dataArray,datas);
	if(pOpKdoqmParser == NULL){
		lgc_errMsg("new failed \n");
		return NULL;
	}

	if( !pOpKdoqmParser->check() ){
		lgc_errMsg("check failed \n");
		delete pOpKdoqmParser;
		pOpKdoqmParser = NULL;
		return NULL;
	}
	
	//success
	return pOpKdoqmParser;
}


//................................................
//class LGC_OpKdomfcParser
//have only one col that have data in prev blk and have not data in next blk
//第一个data是log_opcode_kdomfc，但是这个结构未知
//第二个data是列数据
//接下来没有了
//................................................

//constructor and desctructor

/*
*构造函数
*/
LGC_OpKdomfcParser::LGC_OpKdomfcParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas)
						:LGC_OpcodeParser( pChange, lenArray, dataArray, datas)
{
	return;
}

/*
*析构函数
*/
LGC_OpKdomfcParser::~LGC_OpKdomfcParser()
{
	return;
}

//virtual public member funcitos 

/*
*分析出列数据，并用pDmlRowsOutput输出
*/
int LGC_OpKdomfcParser::generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	//have only one col that have data in prev blk and have not data in next blk

	const unsigned short colNo = this->getStartColNo();
	const unsigned int colLen = m_lenArray[1];
	const char* colData = m_dataArray[1];

	if(0 > pDmlRowsOutput->writeOneColumn(colData,colLen,colNo,COLDATA_FINISH) ){
		lgc_errMsg("writeOneColumn failed \n");
		return -1;
	}
	
	//success
	return 0;
}

/*
*获取OpKdomfcParser占用的data数
*/
unsigned short LGC_OpKdomfcParser::getDatas()
{
	return 2;
}

//private functions

/*
*检查OpKdomfc的属性的合法性
*/
bool LGC_OpKdomfcParser::check()
{
	return (m_lenArray[1] >= sizeof(log_opcode_kdo) 
		    && this->getDatas() <= m_datas);
}


//static member functions

/*
*创建OpKdomfcParser的合法性
*/
LGC_OpKdomfcParser* 
LGC_OpKdomfcParser::createOpKdomfcParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas)
{
	LGC_OpKdomfcParser* pOpKdomfcParser = new LGC_OpKdomfcParser(pChange,lenArray,dataArray,datas);
	if(pOpKdomfcParser == NULL){
		lgc_errMsg("new failed \n");
		return NULL;
	}

	if(!pOpKdomfcParser->check()){
		lgc_errMsg("check failed \n");
		delete pOpKdomfcParser;
		pOpKdomfcParser = NULL;
		return NULL;
	}

	//success
	return pOpKdomfcParser;
}


//................................................
//class LGC_OpKdolmnParser
//没有列数据，
//列数据在undo change vector 的supplemental log 中;
//第一个data是log_opcode_kdolmn, 但是这个结构未知
//接下来没有数据了
//................................................

//constructor and desctructor

/*
*构造函数
*/
LGC_OpKdolmnParser::LGC_OpKdolmnParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas)
						:LGC_OpcodeParser( pChange, lenArray, dataArray, datas)
{
	return;
}

/*
*析构函数
*/
LGC_OpKdolmnParser::~LGC_OpKdolmnParser()
{
	return;
}

//virtual public member funcitos 
/*
*解析出列数据，并用pDmlRowsOutput输出
*/
int LGC_OpKdolmnParser::generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{	
	//success
	return 0;
}

/*
*获取OpKdolmnParser占用的data输出
*/
unsigned short LGC_OpKdolmnParser::getDatas()
{
	return 1;
}

//private functions

/*
*检查OpKdolmnParser的属性的合法性
*/
bool LGC_OpKdolmnParser::check()
{
	return (m_lenArray[0] >= sizeof(log_opcode_kdo) 
		     && this->getDatas() <= m_datas);
}


//static member functions

/*
*创建OpKdolmnParser的合法性
*/
LGC_OpKdolmnParser* 
LGC_OpKdolmnParser::createOpKdolmnParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas)
{
	LGC_OpKdolmnParser* pOpKdolmnParser = new LGC_OpKdolmnParser(pChange,lenArray,dataArray,datas);
	if(pOpKdolmnParser == NULL){
		lgc_errMsg("new failed \n");
		return NULL;
	}

	if(!pOpKdolmnParser->check()){
		lgc_errMsg("check failed \n");
		delete pOpKdolmnParser;
		pOpKdolmnParser = NULL;
		return NULL;
	}

	//success
	return pOpKdolmnParser;
}

//................................................
//class LGC_SupplementalParser
//为了唯一确定更新的行，我们需要额外一些列的数据
//这些列的数据没有被跟新，但是这些列的数据合上更新列
//更新前的值，可以唯一确定哪一行被更新了;
//第一个data是log_supplemental
//第二个data是colNo_array
//第三个data是colLen_array
//接下来是列数据
//................................................

//constructor and desctructor

/*
*构造函数
*/
LGC_SupplementalParser::LGC_SupplementalParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas)
						:LGC_OpcodeParser( pChange, lenArray, dataArray, datas)
{
	return;
}

/*
*析构函数
*/
LGC_SupplementalParser::~LGC_SupplementalParser()
{
	return;
}


/*
*解析出列数据，并用pDmlRowsOutput输出
*/
int LGC_SupplementalParser::generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	if(m_datas < 3){//valid but have not data
		return 0;
	}

	const log_supplemental* pLogSupplemental = (log_supplemental*)m_dataArray[0];
	const WORD* colNo_array = (WORD*)m_dataArray[1];
	const WORD* colLen_array = (WORD*)m_dataArray[2];
	const WORD colNo_array_len = m_lenArray[1];
	const WORD cols = colNo_array_len/sizeof(WORD);
	
	const bool bNeedData_inPrevBlk = (pLogSupplemental->flag&0x02);
	const bool bNeedData_inNextBlk = (pLogSupplemental->flag&0x01);
	
	unsigned short colNo = 0;
	unsigned int colLen = 0;
	char* colData = NULL;
	for(int i=0; i< cols; i++){
		colNo = colNo_array[i];
		colLen = colLen_array[i];
		colData = m_dataArray[3+i];

		//write column data
		if( i == cols -1 && bNeedData_inNextBlk == true){//列数据不完整
			
			if(0 > pDmlRowsOutput->writeOneColumn(colData,colLen,colNo,COLDATA_NOTFINISH) ){
				lgc_errMsg("writeOneColumn failed \n");
				return -1;
			}

		}else{//列数据完整
			
			if(0 > pDmlRowsOutput->writeOneColumn(colData,colLen,colNo,COLDATA_FINISH) ){
				lgc_errMsg("writeOneColumn failed \n");
				return -1;
			}
		}//end if

	}//end for

	//success
	return 0;

}

/*
*获取SupplementalParser的data数
*/
unsigned short LGC_SupplementalParser::getDatas()
{

	if(m_datas < 3){//have not data
		return m_datas;
	}

	const log_supplemental* pLogSupplemental = (log_supplemental*)m_dataArray[0];
	const WORD* colNo_array = (WORD*)m_dataArray[1];
	const WORD* colLen_array = (WORD*)m_dataArray[2];
	const WORD colNo_array_len = m_lenArray[1];
	const WORD cols = colNo_array_len/sizeof(WORD);

	return cols+3;
}

//private functions

/*
*检验SupplementalParser的合法性
*/
bool LGC_SupplementalParser::check()
{
	if(!lgc_check(m_datas >= 1 
				   && m_lenArray[0] >= sizeof(log_supplemental)))
	{
		lgc_errMsg("check failed \n");
		return false;
	}

	if(m_datas < 3){
		return true;
	}

	const log_supplemental* pLogSupplemental = (log_supplemental*)m_dataArray[0];
	const WORD* colNo_array = (WORD*)m_dataArray[1];
	const WORD* colLen_array = (WORD*)m_dataArray[2];
	const WORD colNo_array_len = m_lenArray[1];
	const WORD cols = colNo_array_len/sizeof(WORD);

	if(!lgc_check(m_lenArray[1] % sizeof(WORD) == 0
		          && m_lenArray[2] % sizeof(WORD) == 0
				  && cols*sizeof(WORD) <= m_lenArray[1]
		          && cols*sizeof(WORD) <= m_lenArray[2]
		          && this->getDatas() <= m_datas))
	{
		lgc_errMsg("check failed \n");
		return false;
	}

	return true;
}

//SupplementalParser特有的接口

/*
*获取相对于表的开始列号
*/
unsigned short LGC_SupplementalParser::getStartColNo()
{
	unsigned short startColNo = 0;
	if(m_pChange->getChangeType() == DELETE_UNDOCHANGE 
//		|| m_pChange->getChangeType() == INSERT_UNDOCHANGE
	){
		startColNo = ((log_supplemental*)m_dataArray[0])->start_column_insert;
	}else{
		startColNo = ((log_supplemental*)m_dataArray[0])->start_column;
	}

	if(startColNo == 0){
		lgc_errMsg("startColNo is zero: %s \n", m_pChange->toString().data());
		exit(1);
	}
}

/*
*获取SupLogFlag
*/
unsigned char LGC_SupplementalParser::getSupLogFlag()
{
	return ((log_supplemental*)m_dataArray[0])->flag;
}

//static member functions

/*
*创建SupplementalParser
*/
LGC_SupplementalParser* 
LGC_SupplementalParser::createSupplementalParser(LGC_Change* pChange, unsigned short* lenArray, char** dataArray, unsigned short datas)
{
	LGC_SupplementalParser* pSupplementalParser = new LGC_SupplementalParser(pChange,lenArray,dataArray,datas);
	if(pSupplementalParser == NULL){
		lgc_errMsg("new failed \n");
		return NULL;
	}

	if(!pSupplementalParser->check() ){
		lgc_errMsg("check failed \n");
		delete pSupplementalParser;
		pSupplementalParser = NULL;
		return NULL;
	}

	//success
	return pSupplementalParser;
}


