#include "lgc_DmlRow.h"
#include "lgc_api.h"

//..........................................
//class LGC_DmlRow
//存储了一行的原始数据
//主要组成: dml_header、 
//          redo column list、 undo column list
//提供了向里面添加column数据的方法
//..........................................

BYTE8 LGC_DmlRow::s_createTimes = 0;
BYTE8 LGC_DmlRow::s_freeTimes = 0;

//constructor and desctructor
/*
*构造函数
*/
LGC_DmlRow::LGC_DmlRow(const dml_header* pDmlHeader)
{
	m_dmlHeader = *pDmlHeader;
	m_pCurDmlCol = NULL;
	
	++s_createTimes;
	return;

	fprintf(stdout, "new row: %u\n", pDmlHeader->objNum);
}

/*
*析构函数
*/
LGC_DmlRow::~LGC_DmlRow()
{
	if(m_pCurDmlCol){
		delete m_pCurDmlCol;
		m_pCurDmlCol = NULL;
	}
	
	LGC_DmlColumn* pDmlCol = NULL;

	while(m_redoColList.empty() == false){
		pDmlCol = m_redoColList.front();
		if(pDmlCol){
			delete pDmlCol;
			pDmlCol = NULL;
		}
		m_redoColList.pop_front();
	}

	while(m_undoColList.empty() == false){
		pDmlCol = m_undoColList.front();
		if(pDmlCol){
			delete pDmlCol;
			pDmlCol = NULL;
		}
		m_undoColList.pop_front();
	}
	
	++s_freeTimes;
	lgc_errMsg("LGC_DmlRow: %u\t %u\t %u \n", s_createTimes, s_freeTimes, (s_createTimes-s_freeTimes));

	if((s_createTimes-s_freeTimes) > 1000){
		lgc_errMsg("\n");
	//	exit(1);
	}
	return;
}


//public member functions

/*
*写redo列数据
*/
int LGC_DmlRow::writeRedoColumn(const void* colData, const unsigned int colLen, const unsigned short colNo, const bool isFinish)
{
	
	LGC_DmlColumn* pDmlCol = this->getCurDmlCol();
	if(pDmlCol->writeColData(colData, colLen, colNo) < 0 ){
		lgc_errMsg("writeColData failed \n");
		return -1;
	}
	
	
	if(isFinish){//redo列数据已经完整
		this->addToRedoColList(pDmlCol);
		pDmlCol = NULL;
		this->detachCurDmlCol();
	}
	
	//success
	return 0;
}

/*
*写undo列数据
*/
int LGC_DmlRow::writeUndoColumn(const void* colData, const unsigned int colLen, const unsigned short colNo, const bool isFinish)
{
	LGC_DmlColumn* pDmlCol = this->getCurDmlCol();
	if(pDmlCol->writeColData(colData, colLen, colNo) < 0){
		lgc_errMsg("writeColData failed \n");
		return -1;
	}

	if(isFinish){//redo列数据已经完整
		this->addToUndoColList(pDmlCol);
		pDmlCol = NULL;
		this->detachCurDmlCol();
	}

	//success
	return 0;
}

/*
*dmlRow的所有列的数据已经写完，处理它
*/
int LGC_DmlRow::handleDmlRowEnd()
{
	if(!lgc_check(m_pCurDmlCol == NULL 
		          && m_dmlHeader.redoCols == m_redoColList.size()
		          && m_dmlHeader.undoCols == m_undoColList.size()
		          && this->checkColNoOrder()))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}

	m_dmlHeader.dmlTotalLen = m_dmlHeader.dmlHeaderLen + m_dmlHeader.redoCols_dataLen + m_dmlHeader.undoCols_dataLen;

	//success
	return 0;
}

//private member functions
LGC_DmlColumn* LGC_DmlRow::getCurDmlCol()
{
	if(m_pCurDmlCol == NULL){
		m_pCurDmlCol = new LGC_DmlColumn;
		if(!m_pCurDmlCol){
			exit(1);
		}
	}

	//success
	return m_pCurDmlCol;
}

void LGC_DmlRow::detachCurDmlCol()
{
	m_pCurDmlCol = NULL;
	return;
}

void LGC_DmlRow::addToRedoColList(LGC_DmlColumn* pDmlCol)
{
	fprintf(stdout, "redo colNo: %u \n", pDmlCol->getColNo() );
	m_redoColList.push_back(pDmlCol);

	m_dmlHeader.redoCols++;
	m_dmlHeader.redoCols_dataLen += pDmlCol->getColDataLen();

	//success
	return;
}

void LGC_DmlRow::addToUndoColList(LGC_DmlColumn* pDmlCol)
{
	fprintf(stdout, "undo colNo: %u \n", pDmlCol->getColNo() );
	m_undoColList.push_back(pDmlCol);

	m_dmlHeader.undoCols++;
	m_dmlHeader.undoCols_dataLen += pDmlCol->getColDataLen();

	//success
	return;
}

bool LGC_DmlRow::checkColNoOrder()
{
	list<LGC_DmlColumn*>::iterator it;
	LGC_DmlColumn* pPrevCol = NULL;
	LGC_DmlColumn* pCol = NULL;

	if(m_redoColList.size() > 1){
		it = m_redoColList.begin();
		pPrevCol = *it;
		if(pPrevCol == NULL){
			lgc_errMsg("pPrevCol is NULL \n");
			return false;
		}
		it++;
		for(;it != m_redoColList.end(); it++){
			pCol = *it;
			if(pCol == NULL){
				lgc_errMsg("pCol is NULL \n");
				return false;
			}

			if(pPrevCol->getColNo() >= pCol->getColNo() ){
				lgc_errMsg("colNo order invalid: %u %u \n", pPrevCol->getColNo(), pCol->getColNo());
				return false;
			}

			pPrevCol = *it;
		}
	}

	if(m_undoColList.size() > 1){
		it = m_undoColList.begin();
		pPrevCol = *it;
		if(pPrevCol == NULL){
			lgc_errMsg("pPrevCol is NULL \n");
			return false;
		}
		it++;
		for(; it != m_undoColList.end(); pPrevCol = *it, it++){
			pCol = *it;
			if(pCol == NULL){
				lgc_errMsg("pCol is Null \n");
				return false;
			}

			if(pPrevCol->getColNo() >= pCol->getColNo() ){
				//lgc_errMsg("colNo order invalid: %u %u \n", pPrevCol->getColNo(), pCol->getColNo());
				//return false;
			}
			pPrevCol = *it;
		}
	}
	return true;
}
//...............................................
//class LGC_DmlColumn
//管理一列的数据
//列数据组成: 列号，列长， 数据
//...............................................

//constructor and destructor
LGC_DmlColumn::LGC_DmlColumn()
{
	memset(&m_colHeader,0,sizeof(m_colHeader));
	m_colData = NULL;
	return;
}

LGC_DmlColumn::~LGC_DmlColumn()
{
	if(m_colData){
		delete[] m_colData;
		m_colData = NULL;
	}

	return;
}

//public meber functions

/*
*写列数据
*/
int LGC_DmlColumn::writeColData(const void* colData, const unsigned int colLen, const unsigned short colNo)
{
//	printf("colNo=%u\tcolLen=%u\n", colNo, colLen);
	if( !this->checkColNo(colNo) ){
		return -1;
	}

	this->setColNo(colNo);

	if( this->writeColData(colData, colLen) != colLen){
		return -1;
	}


	//success
	return 0;
}


//private member functions

bool LGC_DmlColumn::checkColNo(const unsigned short colNo)
{
	return (m_colData == NULL || colNo == m_colHeader.colNo );
}


int LGC_DmlColumn::writeColData(const void* colData, const unsigned int colLen)
{
	unsigned int newColLen = 0;

	if(m_colData != NULL){
		newColLen = m_colHeader.colLen + colLen;
	}else{
		newColLen = colLen;
	}

	char* buf = new char[newColLen];
	if(buf == NULL){
		return -1;
	}
	memset(buf,0,newColLen);

	char* writePos = buf;

	if(m_colData != NULL){
		memcpy(writePos, m_colData, m_colHeader.colLen);
		writePos += m_colHeader.colLen;
	}

	memcpy(writePos,colData,colLen);
	writePos += colLen;

	if(m_colData){
		delete[] m_colData;
		m_colData = NULL;
	}

	m_colData = buf;
	buf = NULL;
	m_colHeader.colLen = newColLen;

	//success
	return colLen;

}

void LGC_DmlColumn::setColNo(const unsigned short colNo)
{
	m_colHeader.colNo = colNo;
}


