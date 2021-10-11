#include "lgc_DmlChangeList.h"
#include "lgc_Transaction.h"
#include "lgc_Change.h"
#include "lgc_DmlRowsOutput.h"
#include "lgc_api.h"

#include "lgc_TransactionMgr.h"
#include "lgc_tableMeta.h"

#include "lgc_api.h"

//constructor and desctructor

/*
*构造函数
*/
LGC_DmlChangeList::LGC_DmlChangeList(LGC_Transaction* pTransaction)
{
	m_pTrsct = pTransaction;
	memset(&m_dmlHeader,0,sizeof(dml_header));
	m_objNum = 0;
	m_isDmlEnd = false;
	
	return;
}

/*
*析构函数
*/
LGC_DmlChangeList::~LGC_DmlChangeList()
{
	this->clear();
	return;
}

//public member functions

/*
*添加change到dmlChangeList
*/
void LGC_DmlChangeList::addChange(LGC_Change* pChange)
{
	if( m_isDmlEnd == true){
		lgc_errMsg("\n");
		exit(1);
	}
	
	if(pChange->getChangeType() == MULTIINSERT_REDOCHANGE && this->m_redoChange_list.empty() == false){
		lgc_errMsg("\n");
		exit(1);
	}

	if(pChange->isEndOfDml()){
		m_isDmlEnd = true;
	}

	if(pChange->isUndoChange()){//is UndoChange

		//add to undo change vec list
		this->addUndoChange(pChange);
		pChange = NULL;

	}else{// is redo change vec of dml
		
		//add to redo change vec list
		this->addRedoChange(pChange);
		pChange = NULL;

	}
	
	if(this->isEnd()){
		if( this->prepareForGenDmlData() != 0){
			lgc_errMsg("prepareForGenDmlData failed \n");
			exit(1);
		}
	}
	//success
	return;
}


/*
*解析出dml数据
*/
int LGC_DmlChangeList::generateDmlData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	if(!lgc_check( this->isEnd() == true 
		           && this->isNeedAnalyse() == true))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}

	//生成dmlHeader数据
	if ( this->generateDmlHeaderData(pDmlRowsOutput) < 0){
		lgc_errMsg("generateDmlHeaderData failed \n");
		return -1;
	}
	
	//生成dmlRowsData
	if( this->generateDmlRowsData(pDmlRowsOutput) < 0){
		lgc_errMsg("generateDmlRowsData failed \n");
		return -1;
	}
	
	//生成dmlEndData 
	if(this->generateDmlEndData(pDmlRowsOutput) < 0){
		lgc_errMsg("generateDmlEndData failed \n");
		return -1;
	}

	return 0;
}

//private member functions

/*
*为解析Dml数据做准备
*/
int LGC_DmlChangeList::prepareForGenDmlData()
{
	if(this->isEnd() == false){
		lgc_errMsg("\n");
		return -1;
	}

	m_objNum = this->calculateObjNum();
	if(!this->checkChangeList()){
		lgc_errMsg("checkChangeList failed \n");
		return -1;
	}

	//success
	return 0;
}

/*
*计算出object_id
*/
unsigned int LGC_DmlChangeList::calculateObjNum()
{
	LGC_UndoChange* pUndoChangeFront = (LGC_UndoChange* )m_undoChange_list.front();
	if(pUndoChangeFront == NULL ){
		lgc_errMsg("pUndoChangeFront is NULL \n");
		exit(1);
	}

	return pUndoChangeFront->getObjNum();

}

/*
*检查ChangeList
*/
bool LGC_DmlChangeList::checkChangeList()
{
	list<LGC_Change*>::iterator it;
	LGC_Change* pChange = NULL;
	for(it = m_redoChange_list.begin(); it != m_redoChange_list.end(); it++){
		pChange = *it;
		if(!lgc_check( pChange 
						&& m_objNum == pChange->getObjNum() ))
		{
			lgc_errMsg("check failed \n");
			return false;
		}
	}

	for(it = m_undoChange_list.begin(); it != m_undoChange_list.end(); it++){
		pChange = *it;
		if(!lgc_check(pChange
						&& m_objNum == pChange->getObjNum()))
		{
			lgc_errMsg("check failed \n");
			return false;
		}
	}

	//success
	return true;
}

/*
*添加UndoChange到changeList
*/
void LGC_DmlChangeList::addUndoChange(LGC_Change* pChange)
{
	m_undoChange_list.push_back(pChange);
	return;
}

/*
*添加RedoChange到ChangeList
*/
void LGC_DmlChangeList::addRedoChange(LGC_Change* pChange)
{
	m_redoChange_list.push_back(pChange);
	return;
}

/*
*构建DmlHeaderData，并用pDmlRowsOutput输出
*/
int LGC_DmlChangeList::generateDmlHeaderData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	LGC_UndoChange* pFirstUndoChange = (LGC_UndoChange*)m_undoChange_list.front();	
	LGC_TransactionId transactionId = pFirstUndoChange->getTransactionId();

	memset(&m_dmlHeader,0,sizeof(dml_header));

	m_dmlHeader.xidSlot = transactionId.xidSlot;
	m_dmlHeader.xidSqn  = transactionId.xidSqn;
	m_dmlHeader.scn     = pFirstUndoChange->getSCN();
	m_dmlHeader.objNum  = m_objNum;
	m_dmlHeader.dmlType = this->calculateDmlType();
	m_dmlHeader.dmlHeaderLen = sizeof(dml_header);

	if (sizeof(m_dmlHeader) != pDmlRowsOutput->writeDmlHeader(&m_dmlHeader,sizeof(m_dmlHeader)) ){
		lgc_errMsg("writeDmlHeader failed \n");
		return -1;
	}

	//success
	return 0;

}

/*
*解析出dmlrowsdata， 并用pDmlRowsOutput输出
*/
int LGC_DmlChangeList::generateDmlRowsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	LGC_UndoChange* pUndoChange_back = (LGC_UndoChange* )m_undoChange_list.back();
	unsigned char opMajor_back = pUndoChange_back->getOpMajor();
	unsigned char opMinor_back = pUndoChange_back->getOpMinor();
	

	if(opMajor_back == 11 && opMinor_back == 2 ){//delete dml
		
		return generateDeleteDmlRowData(pDmlRowsOutput);
	
	}else if(opMajor_back == 11 && opMinor_back == 3){//insert dml
	
		return generateInsertDmlRowData(pDmlRowsOutput);
	
	}else if(opMajor_back == 11 && (opMinor_back == 5 || opMinor_back == 7 || opMinor_back == 16 || opMinor_back==6) ){//update dml
		
		return generateUpdateDmlRowData(pDmlRowsOutput);
	
	}else if(opMajor_back == 11 && opMinor_back ==12 ){//multiInsert
	
		return generateMultiInsertDmlRowsData(pDmlRowsOutput);
	
	}else{//unkown opcode, should not exist
		lgc_errMsg("unkown opcode \n");
		return -1;
	}

	//never to here
	lgc_errMsg("never to here\n");
	return -1;

}

/*
*解析出InsertDmlRow, 并用pDmlRowsOutput输出
*/
int LGC_DmlChangeList::generateInsertDmlRowData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	list<LGC_Change*>::reverse_iterator rit;
	LGC_Change* pRedoChange = NULL;

	pDmlRowsOutput->switchToRedo();
	for(rit=m_redoChange_list.rbegin(); rit != m_redoChange_list.rend(); rit++){
		pRedoChange = *rit;
		if(pRedoChange == NULL){
			lgc_errMsg("pRedoChagne is NULL");
			return -1;
		}

		if( pRedoChange->generateColumnsData(pDmlRowsOutput) < 0){
			lgc_errMsg("generateColumnsData failed \n");
			return -1;
		}
	}

	//success
	return 0;
}

/*
*解析出DeleteDmlRow, 并用pDmlRowsOutput输出
*/
int LGC_DmlChangeList::generateDeleteDmlRowData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	list<LGC_Change*>::iterator it;
	LGC_UndoChange* pUndoChange = NULL;
	
	pDmlRowsOutput->switchToUndo();
	for(it=m_undoChange_list.begin(); it != m_undoChange_list.end(); it++){
		pUndoChange = (LGC_UndoChange*)*it;
		if(pUndoChange == NULL){
			lgc_errMsg("pUndoChange is NULL\n");		
			return -1;
		}

		if(pUndoChange->generateColumnsData(pDmlRowsOutput) < 0){
			lgc_errMsg("generateColumnsData failed \n");
			return -1;
		}
	}

	//success
	return 0;
}

/*
*解析出UpdateDmlRow， 并用pDmlRowsOutput输出
*/
int LGC_DmlChangeList::generateUpdateDmlRowData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	list<LGC_Change*>::iterator it;
	LGC_Change* pRedoChange = NULL;
	LGC_UndoChange* pUndoChange = NULL;
	
	pDmlRowsOutput->switchToRedo();
	for(it = m_redoChange_list.begin(); it != m_redoChange_list.end(); it++){
		pRedoChange = *it;
		if(pRedoChange == NULL ){
			lgc_errMsg("pRedoChange is NULL\n");
			return -1;
		}
		
		if(pRedoChange->generateColumnsData(pDmlRowsOutput) < 0){
			lgc_errMsg("generateColumnsData failed \n");
			return -1;
		}
	}
	
	pDmlRowsOutput->switchToUndo();
	for(it = m_undoChange_list.begin(); it != m_undoChange_list.end(); it++){
		pUndoChange = (LGC_UndoChange*)*it;
		if(pUndoChange == NULL){
			lgc_errMsg("pUndoChange is NULL \n");
			return -1;
		}

		if(pUndoChange->generateColumnsData(pDmlRowsOutput) < 0){
			lgc_errMsg("generateColumnsData failed \n");
			return -1;
		}
	}
	
	//success
	return 0;
}

/*
*解析出MultiInsertRowsData, 并用pDmlRowsOutput输出
*/
int LGC_DmlChangeList::generateMultiInsertDmlRowsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	if(m_redoChange_list.empty()){
		return 0;
	}

	if(m_redoChange_list.size() != 1){
		lgc_errMsg("err\n");
		return -1;
	}

	LGC_MultiInsertRedoChange* pMultiRedoChange = (LGC_MultiInsertRedoChange*)m_redoChange_list.front();
	
	pDmlRowsOutput->switchToRedo();

	if (pMultiRedoChange->generateRowsData(pDmlRowsOutput) < 0 ){
		lgc_errMsg("generateRowsData failed \n");
		return -1;
	}
	
	//success
	return 0;
}

/*
*生成dmlEndInfo， 并用pDmlRowsOutput输出
*/
int LGC_DmlChangeList::generateDmlEndData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	if ( 0 != pDmlRowsOutput->writeDmlEndInfo(NULL,0) ){
		lgc_errMsg("writeDmlEndInfo failed \n");
		return -1;
	}

	//success
	return 0;
}


/*
*计算出dmlType
*/
unsigned char 
LGC_DmlChangeList::calculateDmlType()
{
	LGC_UndoChange* pUndoChange_back = (LGC_UndoChange* )m_undoChange_list.back();
	unsigned char opMajor_back = pUndoChange_back->getOpMajor();
	unsigned char opMinor_back = pUndoChange_back->getOpMinor();


	if(opMajor_back == 11 && opMinor_back == 2 ){//delete dml
		
		return DML_DELETE;
	
	}else if(opMajor_back == 11 && opMinor_back == 3){//insert dml
	
		return DML_INSERT;
	
	}else if(opMajor_back == 11 && (opMinor_back == 5 || opMinor_back == 7 || opMinor_back == 16 || opMinor_back==6) ){//update dml
		
		return DML_UPDATE;
	
	}else if(opMajor_back == 11 && opMinor_back ==12 ){//multiInsert
	
		return DML_INSERT;
	
	}else{//unkown opcode, should not exist
		lgc_errMsg("unkown opcode \n");
		exit(1);
	
	}

	//never to here
	lgc_errMsg("never to here \n");
	return -1;
}

//some get or set properties functions 

/*
*changeList是否结束
*/
bool LGC_DmlChangeList::isEnd()
{
	return m_isDmlEnd;
}

/*
*DmlChangeList是否需要解析
*/
bool LGC_DmlChangeList::isNeedAnalyse()
{
	//return true;
	const bool bFind = LGC_TblsMeta_Mgr::getInstance()->isFindOfObj(m_objNum);

	fprintf(stdout, "object:%u\tfind:%u\n", m_objNum, bFind);
	return bFind;
}

/*
*清空dmlChangeList
*/
void LGC_DmlChangeList::clear()
{
	LGC_Change* pChange = NULL;

	while(!m_undoChange_list.empty()){
		pChange = m_undoChange_list.front();
		
		if(pChange){
			LGC_Change::freeChange(pChange);
			pChange = NULL;
		}else{
			lgc_errMsg("pChange is NULL\n");
			exit(1);
		}

		m_undoChange_list.pop_front();
	}

	while(!m_redoChange_list.empty()){
		pChange = m_redoChange_list.front();
		
		if(pChange){
			LGC_Change::freeChange(pChange);
			pChange = NULL;
		}else{
			lgc_errMsg("pChange is NULL \n");
			exit(1);
		}

		m_redoChange_list.pop_front();
	}

	m_isDmlEnd = false;
	m_objNum = 0;

	return;
}
