#include "lgc_DmlRowsOutput.h"
#include "lgc_Transaction.h"
#include "lgc_api.h"
#include "lgc_DmlRow.h"

//constructor and desctructor

/*
*���캯��
*/
LGC_DmlRowsOutput::LGC_DmlRowsOutput(LGC_Transaction* pTrsct)
{
	m_pTrsct = pTrsct;
	m_status = DML_NOTSTART;

	m_pCurDmlRow = NULL;
	m_rowCount = 0;
	return;
}

/*
*��������
*/
LGC_DmlRowsOutput::~LGC_DmlRowsOutput()
{
	if(m_pCurDmlRow){
		delete m_pCurDmlRow;
		m_pCurDmlRow = NULL;
	}
	
	while(m_dmlRow_list.empty() == false){
		LGC_DmlRow* pDmlRow = m_dmlRow_list.front();
		if(pDmlRow){
			delete pDmlRow;
			pDmlRow = NULL;
		}
		m_dmlRow_list.pop_front();
	}


	return;
}

//public member functions

/*
*дdmlHeader
*/
int LGC_DmlRowsOutput::writeDmlHeader(const void* dmlHeader, const unsigned int dmlHeaderLen)
{
	//check status
	if(!lgc_check(m_status == DML_NOTSTART || m_status == DML_END))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}
	if(!lgc_check(m_pCurDmlRow == NULL 
		          && dmlHeaderLen == sizeof(dml_header)))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}
	
	//write
	m_pCurDmlRow = new LGC_DmlRow((const dml_header*)dmlHeader);
	if(m_pCurDmlRow == NULL){
		lgc_errMsg("new failed \n");
		return -1;
	}

	//update status
	this->updateStatus(DML_START);
	
	//success
	return dmlHeaderLen;
}

/*
*дDmlEndInfo
*/
int LGC_DmlRowsOutput::writeDmlEndInfo(const void* dmlEndInfo, const unsigned int dmlEndInfoLen)
{
	//�Ѿ�д��������д
	if(m_status == DML_END){
		return 0;
	}
	
	//check status
	if(m_pCurDmlRow == NULL){
		lgc_errMsg("m_pCurDmlRow failed \n");
		return -1;
	}
	if( !lgc_check(m_status == DML_UNDOCOLUMNS || m_status == DML_REDOCOLUMNS) ){
		lgc_errMsg("m_status invalid \n");
		return -1;
	}
	
	//write
	if(m_pCurDmlRow->handleDmlRowEnd() < 0){
		lgc_errMsg("handleDmlRowEnd failed \n");
		return -1;
	}
	
	this->addDmlRow(m_pCurDmlRow);
	m_pCurDmlRow = NULL;

	//upate status
	this->updateStatus(DML_END);

	//success
	return dmlEndInfoLen;
}

/*
*дһ������
*/
int LGC_DmlRowsOutput::writeOneColumn(const void* colData, const unsigned int colLen, const unsigned short colNo, const bool isFinish)
{
	if(m_status == DML_REDOCOLUMNS){
		return this->writeOneRedoColumn(colData, colLen, colNo, isFinish);
	}else if(m_status == DML_UNDOCOLUMNS){
		return this->writeOneUndoColumn(colData, colLen, colNo, isFinish);
	}else{
		//fatal error
		lgc_errMsg("m_status invalid: %u \n",m_status);
		exit(1);
	}

	//never to here
	lgc_errMsg("never to here \n");
	return -1;
}

/*
*�л���undo
*/
void LGC_DmlRowsOutput::switchToUndo()
{
	if(m_status != DML_START && m_status != DML_REDOCOLUMNS){
		lgc_errMsg("check failed \n");
		exit(1);
	}

	//update status
	this->updateStatus(DML_UNDOCOLUMNS);
	
	//success
	return;
}

/*
*�л���redo
*/
void LGC_DmlRowsOutput::switchToRedo()
{
	if(m_status != DML_START){
		lgc_errMsg("check failed \n");
		exit(1);
	}

	//update status
	this->updateStatus(DML_REDOCOLUMNS);

	//success
	return;
}



//private functions

/*
*дredoColumn
*/
int LGC_DmlRowsOutput::writeOneRedoColumn(const void* colData, const unsigned int colLen, const unsigned short colNo, const bool isFinish)
{
	if(m_pCurDmlRow == NULL){
		lgc_errMsg("check failed \n");
		return -1;
	}

	return m_pCurDmlRow->writeRedoColumn(colData, colLen, colNo, isFinish);
}

/*
*дUndoColumn
*/
int LGC_DmlRowsOutput::writeOneUndoColumn(const void* colData, const unsigned int colLen, const unsigned short colNo, const bool isFinish)
{
	if(m_pCurDmlRow == NULL){
		lgc_errMsg("check failed \n");
		return -1;
	}
	
	return m_pCurDmlRow->writeUndoColumn(colData, colLen, colNo, isFinish);
}

/*
*����status
*/
void LGC_DmlRowsOutput::updateStatus(StatusType status)
{
	m_status = status;
}

/*
*���һ��
*/
void LGC_DmlRowsOutput::addDmlRow(LGC_DmlRow* pDmlRow)
{
	if(pDmlRow == NULL){
		lgc_errMsg("pDmlRow is NULL \n");
		exit(1);
	}

	m_dmlRow_list.push_back(pDmlRow);
	m_rowCount++;
	return;
}

//some get or set properties funtions
dml_header LGC_DmlRowsOutput::getCurDmlHeader()
{
	return m_pCurDmlRow->getDmlHeader();
}





