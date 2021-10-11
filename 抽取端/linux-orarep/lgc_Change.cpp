#include "lgc_Change.h"
#include "lgc_OpcodeParser.h"
#include "lgc_RedoRecord.h"
#include "lgc_DmlRowsOutput.h"
#include "lgc_api.h"


//..............................................................
//Abstract Class LGC_Change
//change的主要组成: changeHeader, lenArray, dataArray;
//changeHeader里记录了change的类型等信息;
//lenArray记录了dataArray里每个data的长度，
//但是lenArray[0]记录的是lenArray本身的长度;
//dataArray里的data是要解析的数据
//..............................................................

BYTE8 LGC_Change::s_createTimes = 0;
BYTE8 LGC_Change::s_freeTimes = 0;

//constructor and desctructor

/*
*构造函数
*/
LGC_Change::LGC_Change(LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign, 
                       char** dataArray, unsigned int changeNo)
{
	if(pRedoRecord == NULL || lenArray == NULL || dataArray == NULL){
		lgc_errMsg("\n");
		exit(1);
	}

	m_pRedoRecord		= pRedoRecord;
	
	m_addr.threadId		= pRedoRecord->getThreadId();
	m_addr.recordRBA	= pRedoRecord->getRBA();
	m_addr.changeNo		= changeNo;

	m_changeHeader		= changeHeader;
	m_lenArray			= lenArray;
	m_lenArrayAlign		= lenArrayAlign;
	m_datas				= m_lenArray[0]/2-1;
	m_dataArray			= dataArray;
	m_changeType		= UNKOWN_CHANGE;
	m_pUndoChange		= NULL;

	m_pOpcodeParser		= NULL;

	m_beFreed			= 0;

	s_createTimes++;

}

/*
*析构函数
*/
LGC_Change::~LGC_Change()
{
	if(m_lenArray){
		delete[] m_lenArray;
		m_lenArray = NULL;
	}

	if(m_lenArrayAlign){
		delete[] m_lenArrayAlign;
		m_lenArrayAlign = NULL;
	}

	for(int i = 0; i < m_datas; i++){
		delete[] m_dataArray[i];
		m_dataArray[i] = NULL;
	}
	delete[] m_dataArray;
	m_dataArray = NULL;

	if(m_pOpcodeParser){
		delete m_pOpcodeParser;
		m_pOpcodeParser = NULL;
	}

	if(m_pUndoChange){
		delete m_pUndoChange;
		m_pUndoChange = NULL;
	}
	
	m_beFreed = 1;
	s_freeTimes++;

	lgc_errMsg("s_createTimes=%lld\ts_freeTimes=%lld\tsub=%lld \n", s_createTimes,s_freeTimes,(s_createTimes - s_freeTimes));
	return;
}

//public virtual functions



//这些共有方法不应该被子类隐藏或者覆盖

/*
*根据changeHeader里记录的change类型信息判断是否是undoChange
*/
bool LGC_Change::isUndoChange() const
{
	return (m_changeHeader.op_major == 5 && m_changeHeader.op_minor == 1);
}

/*
*设置undoChange
*当前的change需要依赖的undoChange的一些信息
*/
void LGC_Change::setUndoChange(LGC_UndoChange* pUndoChange)
{
	if(pUndoChange == NULL || pUndoChange->isUndoChange() == false){
			lgc_errMsg("pUndoChange is NULL\n");
			exit(1);
	}

	m_pUndoChange = pUndoChange;
	m_pUndoChange->setRedoChange(this);
	return;
}

/*
*获取ThreadId
*rac环境有多个节点在工作，每个节点有自己的ThreadId
*/
unsigned short LGC_Change::getThreadId() const
{
	return m_addr.threadId;
}

/*
*获取change的scn号
*/
BYTE8 LGC_Change::getSCN() const
{
	BYTE8 scn = m_changeHeader.high_scn;
	scn <<=32;
	scn += m_changeHeader.low_scn;
	return scn;
}

/*
*获取主操作码
*操作码是个很重要的信息，
*他直接决定了dataArray里的数据是用来做什么的
*/
BYTE LGC_Change::getOpMajor() const
{
	BYTE opMajor = 0;
	BYTE opMinor = 0;

	if(this->isUndoChange()){//undo change vec
		if(m_changeType == UNKOWN_UNDOCHANGE){
			lgc_errMsg("\n");
			exit(1);
		}

		opMajor = ((log_opcode_51_second*)m_dataArray[1])->op_major;
		opMinor = (((log_opcode_kdo*)m_dataArray[3])->opcode&0x1F);
		
	}else{//redo change vec
		opMajor = m_changeHeader.op_major;
		opMinor = m_changeHeader.op_minor;
	}
	
	return opMajor;
}

/*
*获取次操作码
*/
BYTE LGC_Change::getOpMinor() const
{
	BYTE opMajor = 0;
	BYTE opMinor = 0;

	if(this->isUndoChange()){//undo change vec
		if(m_changeType == UNKOWN_UNDOCHANGE){
			lgc_errMsg("\n");
			exit(1);
		}

		opMajor = ((log_opcode_51_second*)m_dataArray[1])->op_major;
		opMinor = (((log_opcode_kdo*)m_dataArray[3])->opcode&0x1F);
		
	}else{//redo change vec
		opMajor = m_changeHeader.op_major;
		opMinor = m_changeHeader.op_minor;
	}
	
	return opMinor;
}

/*
*将change的某些属性转化为string
*/
string LGC_Change::toString()
{
	string changeTypeStr = this->changeTypeToString();
	string changeAddrStr = this->changeAddrToString();
	string trsctIdStr    = this->changeTIdToString();
	string objNumStr     = this->objNumToString();
	string dumpText;
	
	dumpText += changeTypeStr;
	dumpText += "\t";
	dumpText += changeAddrStr;	
	dumpText += "\t";
	dumpText += trsctIdStr;
	dumpText += "\t";
	dumpText += objNumStr;

	return dumpText;
}

string LGC_Change::changeAddrToString()
{
	string addrStr;

	char rbaStr[126]={0};
	sprintf(rbaStr,"0x%06x.%08x.%04x ", m_addr.recordRBA.sequence, m_addr.recordRBA.blockNo, m_addr.recordRBA.offsetInBlk);

	char numStr[50] = {0};
	
	addrStr +="threadId:";
	sprintf(numStr, "%u",m_addr.threadId);
	addrStr += numStr;
	addrStr +="\t";

	addrStr +="rba:";
	addrStr +=rbaStr;
	addrStr +="\t";

	addrStr +="changeNo:";
	sprintf(numStr, "%u",m_addr.changeNo);
	addrStr += numStr;

	return addrStr;
}

string LGC_Change::changeTypeToString()
{
	string changeType;
	switch(m_changeType){
		case UNKOWN_UNDOCHANGE: 
		{
			changeType="UNKOWN_UNDOCHANGE";
		}
		break;
		case	INSERT_UNDOCHANGE: 
		{
			changeType="INSERT_UNDOCHANGE";
		}
		break;
		case DELETE_UNDOCHANGE:
		{
			changeType="DELETE_UNDOCHANGE";
		}
		break;
		case UPDATE_UNDOCHANGE: 
		{
			changeType="UPDATE_UNDOCHANGE";
		}
		break;
		case ROWCHAIN_UNDOCHANGE:
		{
			changeType="ROWCHAIN_UNDOCHANGE";
		}
		break;
		case MFC_UNDOCHANGE:
		{
			changeType="MFC_UNDOCHANGE";
		}
		break;
		case LMN_UNDOCHANGE:
		{
			changeType="LMN_UNDOCHANGE";
		}
		break;	
		case MULTIINSERT_UNDOCHANGE:
		{
			changeType="MULTIINSERT_UNDOCHANGE";
		}
		break;
		case MULTIDELETE_UNDOCHANGE:
		{
			changeType="MULTIDELETE_UNDOCHANGE";
		}
		break;
		case INSERT_REDOCHANGE:
		{
			changeType="INSERT_REDOCHANGE";
		}
		break;
		case DELETE_REDOCHANGE:
		{
			changeType="DELETE_REDOCHANGE";
		}
		break;
		case UPDATE_REDOCHANGE:
		{
			changeType="UPDATE_REDOCHANGE";
		}
		break;
		case MULTIINSERT_REDOCHANGE:
		{
			changeType="MULTIINSERT_REDOCHANGE";
		}
		break;
		case ROWCHAIN_REDOCHANGE:
		{
			changeType="ROWCHAIN_REDOCHANGE";
		}
		break;
		case BEGINT_CHANGE:
		{
			changeType="BEGINT_CHANGE";
		}
		break;
		case ENDT_CHANGE:
		{
			changeType="ENDT_CHANGE";
		}
		break;
		case UNKOWN_CHANGE:
		{
			changeType="UNKOWN_CHANGE";
		}
		break;
		default:
		{
			lgc_errMsg("changeType invalid\n");
			exit(1);
		}
	}//end switch
	
	string resultStr;
	resultStr += "changeType:"; 
	resultStr += changeType;
	return resultStr;
}

string LGC_Change::changeTIdToString()
{
	string out;
	char iStr[26] = {0};
	LGC_TransactionId tid = this->getTransactionId();

	out += "xidSlot=";
	sprintf(iStr, "%u", tid.xidSlot);
	out += iStr;
	
	out += "\t";

	out += "xidSqn=";
	sprintf(iStr, "%u", tid.xidSqn);
	out += iStr;
	
	return out;
}

string LGC_Change::objNumToString()
{
	string out;
	char iStr[26] = {0};
	sprintf(iStr, "%u", this->getObjNum());
	
	out += "obj:";
	out += iStr;

	return out;
}
//static member functions

/*
*根据changeHeader里记录的类型信息创建change相应的change子类
*/
LGC_Change* 
LGC_Change::createChange(
  LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, 
  unsigned short** pLenArray, unsigned short** pLenArrayAlign, char*** pDataArray,
  unsigned int changeNo )
{
	LGC_Change* pChange = NULL;
	BYTE opcode_major	= changeHeader.op_major;
	BYTE opcode_minor	= changeHeader.op_minor;

	if( opcode_major == 5 && opcode_minor == 1){//undo
		pChange = LGC_UndoChange::createUndoChange(pRedoRecord,changeHeader, pLenArray,pLenArrayAlign,pDataArray,changeNo);

	}else if(opcode_major == 11 && opcode_minor == 2){//insert
		pChange = LGC_InsertRedoChange::createInsertRedoChange(pRedoRecord,changeHeader, pLenArray,pLenArrayAlign,pDataArray,changeNo);

	}else if(opcode_major == 11 && opcode_minor == 3){ //delete
		pChange = LGC_DeleteRedoChange::createDeleteRedoChange(pRedoRecord,changeHeader, pLenArray,pLenArrayAlign,pDataArray,changeNo);

	}else if( opcode_major == 11 && opcode_minor == 5){//update
		pChange = LGC_UpdateRedoChange::createUpdateRedoChange(pRedoRecord,changeHeader, pLenArray,pLenArrayAlign,pDataArray,changeNo);

	}else if(opcode_major == 11 && opcode_minor == 11){//multi insert
		pChange = LGC_MultiInsertRedoChange::createMultiInsertRedoChange(pRedoRecord,changeHeader, pLenArray,pLenArrayAlign,pDataArray,changeNo);

	}else if(opcode_major == 11 && opcode_minor == 6){//row chain for updata
		pChange = LGC_RowChainRedoChange::createRowChainRedoChange(pRedoRecord,changeHeader, pLenArray,pLenArrayAlign,pDataArray,changeNo);

	}else if(opcode_major == 5  && opcode_minor == 2){//begin transaction
		pChange = LGC_BeginTChange::createBeginTChange(pRedoRecord,changeHeader, pLenArray,pLenArrayAlign,pDataArray,changeNo);

	}else if(opcode_major == 5  && opcode_minor == 4){//commit
		pChange = LGC_EndTChange::createEndTChange(pRedoRecord,changeHeader, pLenArray,pLenArrayAlign,pDataArray,changeNo);

	}else{
		pChange = LGC_UnkownChange::createUnkownChange(pRedoRecord,changeHeader, pLenArray,pLenArrayAlign,pDataArray,changeNo);
	}
	
	//return
	if(pChange == NULL){
		return NULL;
	}else{
		*pLenArray		= NULL;
		*pLenArrayAlign = NULL;
		*pDataArray		= NULL;
		return pChange;
	}
}

/*
*释放change
*所有的change对象都应该通过这个静态函数释放
*不应该用delete直接释放
*/
void LGC_Change::freeChange(LGC_Change* pChange)
{
	if(pChange == NULL){
		return;	
	}

	if(pChange->isUndoChange()){
		LGC_UndoChange::freeUndoChange((LGC_UndoChange*)pChange);
		pChange = NULL;
	}else{
		delete pChange;
		pChange = NULL;
	}

	return;
}

//.......................................
//class LGC_UndoChange
//UndoChange的dataArray主要由三部分组成:
//undoInfo用的subDataArray、opcodeParser用的subDataArray, 
//supplementalParser用的subDataArray;
//undoInfo用的是第一个和第二个data;
//opcodeParser用的是中间的data;
//supplementalParser用的是最后几个data;
//.......................................

//constructor and desctructor

/*
*构造函数
*/
LGC_UndoChange::LGC_UndoChange(
LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign, char** dataArray,unsigned int changeNo
):LGC_Change(pRedoRecord, changeHeader,lenArray, lenArrayAlign, dataArray,changeNo)
{
	m_changeType = UNKOWN_UNDOCHANGE;
	memset(&m_logOp51,0,sizeof(log_opcode_51));
	memset(&m_logOp51Second,0,sizeof(log_opcode_51_second));
	memset(&m_logOp51Kdo,0,sizeof(log_opcode_kdo));
	
	m_supLogOff = 0;
	m_pSupplementalParser = NULL;
	//指示哪一个redoChange依赖于当前的
	//undoChange, 并把删除当前undo的责任交给他
	m_pRedoChange = NULL;

	return;
}


/*
*析构函数
*/
LGC_UndoChange::~LGC_UndoChange()
{
	if(m_pSupplementalParser){
		delete m_pSupplementalParser;
		m_pSupplementalParser = NULL;
	}

	return;
}

//public virtual functions

/*
*初始化
*主要做了: 检查合法，计算出changeType 
*          创建opcodeParser,  创建supplementalParser
*undoChange初始化后，他提供的共有接口才能被调用
*/
int LGC_UndoChange::init()
{
	if(!lgc_check(m_datas >= 2 
		          && m_lenArray[1] >= sizeof(m_logOp51) 
		          && m_lenArray[2] >= sizeof(m_logOp51Second)))
	{
		lgc_errMsg("m_lenArray invalid: datas=%u  m_lenArray[1]=%u m_lenArray[2]=%u sizeof(m_logOp51)=%u sizeof(m_logOp51Second)=%u \n", 
							m_datas,  m_lenArray[1], m_lenArray[2], sizeof(m_logOp51), sizeof(m_logOp51Second));
		return -1;
	}

	memcpy(&m_logOp51,m_dataArray[0],sizeof(m_logOp51));
	memcpy(&m_logOp51Second,m_dataArray[1],sizeof(m_logOp51Second));
	memset(&m_logOp51Kdo,0,sizeof(log_opcode_kdo));

	//如果(m_logOp51Second.op_major,m_logOp51Second.op_minor) not in [(11,1),(5,1)], 
	//则这个UndoChange对我们来说是无用的，因为他不包含dml数据
	if ( !lgc_check((m_logOp51Second.op_major == 11 || m_logOp51Second.op_major == 5) 
		             && m_logOp51Second.op_minor == 1))
	{// not dml data
		return 0;
	}
	
	//如果m_datas < 4 则这个UndoChange对我们来说是无用的
	if(m_datas < 4){
		if(m_logOp51Second.objn != 0){//have not kdo info and objNum != 0
			//lgc_errMsg("have not kdo info and objNum != 0: m_lenVec_changes=%u m_logOp51Second.objn=%u \n", m_lenVec_changes, m_logOp51Second.objn);
			//return -1
			return 0;
		}
		//have not kdo info but objNum == 0 valid
		return 0;
	}
	
	const short lenOfLogOp51Kdo = *(m_lenArray+3+1);
	if( lenOfLogOp51Kdo < 8){
		lgc_errMsg("change length < 8 invalid \n");
		return -1;

	}else if( lenOfLogOp51Kdo < 12){//change length invalid
		memcpy(&m_logOp51Kdo, m_dataArray[3], 8);

	}else if(lenOfLogOp51Kdo < sizeof(log_opcode_kdo)){
		memcpy(&m_logOp51Kdo, m_dataArray[3], 12);

	}else{
		memcpy(&m_logOp51Kdo, m_dataArray[3], sizeof(log_opcode_kdo));
	}
	
	//计算出changeType
	ChangeType changeType = this->calculateUndoChangeType();
	m_changeType = changeType;
	
	//创建opcodeParser,  创建supplementalParser
	if(this->buildParsers() < 0){
		lgc_errMsg("this->buildParsers failed \n");
		return -1;
	}

	//success
	return 0;
}

/*
*是否需要添加给事务
*只有被提交给事务的change才会被解析
*/
bool LGC_UndoChange::isNeedAddToTrsct()
{
	if(m_pRedoChange){
		//把请求传给对应的RedoChange
		return m_pRedoChange->isNeedAddToTrsct();
	}else{
		if(m_changeType == UNKOWN_UNDOCHANGE){
			return false;
		}else{
			if(m_pRedoChange == NULL && m_changeType != MFC_UNDOCHANGE && m_changeType != LMN_UNDOCHANGE){
				return false;
			}else{
				return true;
			}	
		}
	}

	//should never to here
	lgc_errMsg("\n");
	exit(1);
}

/*
*获取事务Id
*/
LGC_TransactionId LGC_UndoChange::getTransactionId()
{
	LGC_TransactionId transactionId;
	transactionId.xidSlot = this->getXidSlot();
	transactionId.xidSqn  = this->getXidSqn();

	return transactionId;
}

/*
*是否是dml的最后一个Change
*/
bool LGC_UndoChange::isEndOfDml()
{	
	if(m_changeType == UNKOWN_UNDOCHANGE){

		lgc_errMsg("m_changeType invalid \n");
		exit(1);

	}else if(   m_changeType == MFC_UNDOCHANGE 
		     || m_changeType == LMN_UNDOCHANGE)
	{//这些UndoChange没有对应的redo change,
	 //所以需要判断isEndOfDml

		if(m_pRedoChange != NULL){
			lgc_errMsg("\n");
			exit(1);
		}
		
		return (this->getSupLogFlag() & 0x04);

	}else if(      m_changeType == MULTIINSERT_UNDOCHANGE
				|| m_changeType == MULTIDELETE_UNDOCHANGE)
	{
		
		if(m_pRedoChange == NULL){
			return true;
		}else{
			return false;
		}
	
	}else{//这些UndoChange有对应的redo change,
	      //所以不需要判断isEndOfDml
		  //但是经过测试好像有事后的确没有对应的redoChange
		
		//return (this->getSupLogFlag() & 0x04); 
		if(m_pRedoChange == NULL){
			return (this->getSupLogFlag() & 0x04); 
		}else{
			return false;
		}
	}

	//should never to here
	lgc_errMsg("\n");
	exit(1);
}

/*
*获取ObjNum
*每个表都有对应的Object_id, 可以从dba_objects表中查到
*/
unsigned int LGC_UndoChange::getObjNum()
{
	if(m_changeType == UNKOWN_UNDOCHANGE){
		lgc_errMsg("change type invalid\n");
		exit(1);
	}

	return m_logOp51Second.objn;
}

/*
*获取相对于表的起始列号
*m_dataArray里有多列的数据，列号递增，
*最小的列号是相对于数据块的, 而不是相对于表的
*/
unsigned short LGC_UndoChange::getStartColNo()
{
	if(m_pSupplementalParser == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}

	return m_pSupplementalParser->getStartColNo();
}

/*
*解析出dataArray里的列数据，并用pDmlRowsOutput输出
*/
int LGC_UndoChange::generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	if(!lgc_check(m_pOpcodeParser && m_pSupplementalParser)){
		lgc_errMsg("\n");
		return -1;
	}

	if(m_pOpcodeParser->generateColumnsData(pDmlRowsOutput) < 0){
		lgc_errMsg("\n");
		return -1;
	}
	
	if(m_pSupplementalParser->generateColumnsData(pDmlRowsOutput) < 0){
		lgc_errMsg("\n");
		return 0;
	}

	//success
	return 0;
}

//private member functions

/*
*构建UndoChange所有的解析对象
*/
int LGC_UndoChange::buildParsers()
{	
	if(m_changeType != UNKOWN_UNDOCHANGE 
		&& m_changeType != MULTIINSERT_UNDOCHANGE
		&& m_changeType != MULTIDELETE_UNDOCHANGE)
	{
		//创建opcodeParser
		LGC_OpcodeParser* pOpcodeParser = this->createOpcodeParser();
		if(pOpcodeParser == NULL){
			lgc_errMsg("createOpcodeParser failed \n");
			return -1;
		}
		m_pOpcodeParser = pOpcodeParser;
		pOpcodeParser = NULL;
		
		//计算出undoChange的补充日志的起始位置
		//这个位置也是supplementParser开始解析的位置
		unsigned short supLogOff = this->calculateSupLogOff();
		if(!(supLogOff > 3)){
			lgc_errMsg("calculateSupLogOff failed \n");
			return -1;
		}
		m_supLogOff = supLogOff;
		
		//创建supplementaParser
		LGC_SupplementalParser* pSupplementalParser = this->createSupplementalParser();
		if(pSupplementalParser == NULL){
			lgc_errMsg("createSupplementalParser failed \n");
			return -1;
		}
		m_pSupplementalParser = pSupplementalParser;
		pSupplementalParser = NULL;
	}
	
	//检查m_datas是否合法
	if(!this->checkDataNum()){
		lgc_errMsg("checkDataNum failed \n");
		return -1;
	}

	//success
	return 0;
}

/*
*检查m_datas是否合法
*/
bool LGC_UndoChange::checkDataNum()
{
	if(m_pSupplementalParser != NULL && m_pOpcodeParser != NULL){
		return ( m_datas == ( 3 + m_pOpcodeParser->getDatas() + m_pSupplementalParser->getDatas() ) ) ;	
	}else{
		return m_datas >= 2;
	}
}

/*
*根据现有信息计算出UndoChangeType
*/
ChangeType LGC_UndoChange::calculateUndoChangeType()
{
	// if (m_logOp51Second.op_major, m_logOp51Second.op_minor) not in [(11,1),(5,1)] or !(m_datas >= 4) or !(m_lenArray[1+3] >= 12) 
	// then undoChange's type is unkown
	// 我们只关心(m_logOp51Second.op_major, m_logOp51Second.op_minor)为[(11,1), (5,1)]的undo change vector; 
	// 我们需要m_dataArray的第四个结构m_logOp51Kdo 所以m_datas >= 4;
	// 我们需要m_logOp51Kdo里的opcode, 所以len(m_data[3]) >= sizeof(m_logOpKdo);

	if( !( (m_logOp51Second.op_major == 11 && m_logOp51Second.op_minor == 1) 
		   || (m_logOp51Second.op_major == 5 && m_logOp51Second.op_minor == 1))){
		//unkown undo change
		return UNKOWN_UNDOCHANGE;

	}else if(!(m_datas >= 4 && m_lenArray[1+3] >= sizeof(m_logOp51Kdo))){
		return UNKOWN_UNDOCHANGE;

	}else if(m_logOp51Second.op_major == 11 && m_logOp51Second.op_minor == 1 && (m_logOp51Kdo.opcode&0x1F) == 3){
		//delete undo change, it's redo change is insert redo change
		return DELETE_UNDOCHANGE;

	}else if(m_logOp51Second.op_major == 11 && m_logOp51Second.op_minor == 1 && (m_logOp51Kdo.opcode&0x1F) == 2){
		//insert undo change, it's redo change is delete redo change
		return INSERT_UNDOCHANGE;

	}else if(m_logOp51Second.op_major == 11 && m_logOp51Second.op_minor == 1 && (m_logOp51Kdo.opcode&0x1F) == 6){
		//row chain undo change, it's redo change is row chain redo change
		return ROWCHAIN_UNDOCHANGE;

	}else if(m_logOp51Second.op_major == 11 && m_logOp51Second.op_minor == 1 && (m_logOp51Kdo.opcode&0x1F) == 5){
		//update undo change, it's redo change is update redo change
		return UPDATE_UNDOCHANGE;

	}else if(m_logOp51Second.op_major == 11 && m_logOp51Second.op_minor == 1 && (m_logOp51Kdo.opcode&0x1F) == 7){
		//mfc undo change, it's redo change is mfc redo change
		return MFC_UNDOCHANGE;

	}else if(m_logOp51Second.op_major == 11 && m_logOp51Second.op_minor == 1 && (m_logOp51Kdo.opcode&0x1F) == 16){
		//LMN undo change, it's redo change is lmn redo change
		return LMN_UNDOCHANGE;

	}else if(m_logOp51Second.op_major == 11 && m_logOp51Second.op_minor == 1 && (m_logOp51Kdo.opcode&0x1F) == 11){
		//不需要解析里面的column数据
		return MULTIINSERT_UNDOCHANGE;

	}else if(m_logOp51Second.op_major == 11 && m_logOp51Second.op_minor == 1 && (m_logOp51Kdo.opcode&0x1F) == 12){
		//不需要解析里面的column数据
		return MULTIDELETE_UNDOCHANGE;

	}else{
		//undo undo change
		//这些undo change 我不知道怎么计算他们的补充日志的位置
		return UNKOWN_UNDOCHANGE;
	}
}

/*
*计算undoChange的补充日志位置
*/
unsigned short LGC_UndoChange::calculateSupLogOff()
{
	if( m_pOpcodeParser == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}

	unsigned short supLogOff = 3+m_pOpcodeParser->getDatas();
	return supLogOff;
}

/*
*创建opcodeParser
*/
LGC_OpcodeParser* LGC_UndoChange::createOpcodeParser()
{
	return LGC_OpcodeParser::createOpcodeParser(this, &m_lenArray[3+1], &m_dataArray[3],m_datas-3);
}

/*
*创建supplementalParser
*/
LGC_SupplementalParser* LGC_UndoChange::createSupplementalParser()
{
	unsigned short supLogOff = m_supLogOff;
	return LGC_SupplementalParser::createSupplementalParser(this, &m_lenArray[1+supLogOff],&m_dataArray[supLogOff],m_datas-supLogOff); 	
}

//some get or set properties functions
//PUBLIC
unsigned char LGC_UndoChange::getSupLogFlag()
{
	if( m_pSupplementalParser == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}

	return m_pSupplementalParser->getSupLogFlag();

}

void LGC_UndoChange::setRedoChange(LGC_Change* pRedoChange)
{
	m_pRedoChange = pRedoChange;
	return;
}

//static member functions

/*
*创建UndoChange
*/
LGC_UndoChange* 
LGC_UndoChange::createUndoChange(
LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short** pLenArray, unsigned short** pLenArrayAlign, 
char*** pDataArray, unsigned int changeNo)
{
	LGC_UndoChange* pUndoChange = new LGC_UndoChange(pRedoRecord,changeHeader,*pLenArray,*pLenArrayAlign,*pDataArray,changeNo);
	if(pUndoChange == NULL){
		lgc_errMsg("new failed \n");
		return NULL;
	}
	*pLenArray = NULL;
	*pLenArrayAlign = NULL;
	*pDataArray = NULL;

	//success
	return pUndoChange;
}

/*
*释放UndoChange
*/
void LGC_UndoChange::freeUndoChange(LGC_UndoChange* pUndoChange)
{
	if(pUndoChange == NULL || pUndoChange->isUndoChange() == false){
		lgc_errMsg("\n");
		exit(1);
	}
	
	if(pUndoChange->isNeedFree()){
		delete pUndoChange;
		pUndoChange = NULL;
	}

	return;
}

//.......................................
//class LGC_InsertRedoChange
//向一个数据块中插入列数据产生的change
//.......................................


//constructor and desctructor

/*
*构造函数
*/
LGC_InsertRedoChange::LGC_InsertRedoChange(
LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign, char** dataArray, unsigned int changeNo
):LGC_Change(pRedoRecord, changeHeader,lenArray, lenArrayAlign, dataArray, changeNo)
{
	m_changeType = INSERT_REDOCHANGE;
	memset(&m_logOpKdoirp,0,sizeof(log_opcode_kdoirp));
	return;
}

/*
*析构函数
*/
LGC_InsertRedoChange::~LGC_InsertRedoChange()
{
	return;
}

//public virtual functions

/*
*初始化
*/
int
LGC_InsertRedoChange::init()
{
	if(!lgc_check(m_changeHeader.op_major == 11 
		          && m_changeHeader.op_minor == 2 
		          && m_datas >=2 
		          && m_lenArray[1+1] >= sizeof(log_opcode_kdoirp)))
	{
		lgc_errMsg("\n");
		return -1;
	}

	memcpy(&m_logOpKdoirp,m_dataArray[1],sizeof(m_logOpKdoirp));
	
	if(m_pUndoChange == NULL){
		//lgc_errMsg("warning: m_pUndoChange is NULL: changeNo=%u fileId=%u m_datas=%u cc=%u \n", 
		//	                        m_addr.changeNo, m_changeHeader.file_id, m_datas, m_logOpKdoirp.cc);
		return 0;
	}

	if(!lgc_check(m_pUndoChange 
		         && m_pUndoChange->getChangeType() != UNKOWN_UNDOCHANGE))
	{
		lgc_errMsg("m_pUndoChange invalid \n");
		return -1;
	}
	
	//创建OpcodeParser
	m_pOpcodeParser = LGC_OpcodeParser::createOpcodeParser(this,&m_lenArray[1+1],&m_dataArray[1],m_datas-1);
	if(m_pOpcodeParser == NULL){
		lgc_errMsg("\n");
		return -1;
	}

	//检查m_datas
	if(!lgc_check(m_pOpcodeParser->getDatas()  == m_datas -1 )){
		lgc_errMsg("\n");
		return -1;
	}

	return 0;
}

/*
*是否需要交给事务来处理
*/
bool LGC_InsertRedoChange::isNeedAddToTrsct()
{
	if(m_pUndoChange == NULL){
		return false;
	}else{
		return true;
	}
	
	//should never to here
	lgc_errMsg("\n");
	exit(1);
}

/*
*获取事务Id
*/
LGC_TransactionId 
LGC_InsertRedoChange::getTransactionId()
{
	if(m_pUndoChange == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}

	return m_pUndoChange->getTransactionId();
}

/*
*change里是否有dmlEnd的标志
*/
bool LGC_InsertRedoChange::isEndOfDml()
{
	if(m_pUndoChange == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}
	
	return (m_pUndoChange->getSupLogFlag() & 0x04);//update 20150515 
}

/*
*获取object_id
*/
unsigned int LGC_InsertRedoChange::getObjNum()
{
	if(m_pUndoChange == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}

	return m_pUndoChange->getObjNum();
}

/*
*获取相对于表的startColNo
*/
unsigned short LGC_InsertRedoChange::getStartColNo()
{
	if(m_pUndoChange == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}

	return m_pUndoChange->getStartColNo();
}

/*
*解析出列数据，并用pDmlRowsOutput输出
*/
int LGC_InsertRedoChange::generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	if(m_pUndoChange == NULL || m_pOpcodeParser == NULL){
		lgc_errMsg("\n");
		return -1;
	}

	if(m_pOpcodeParser->generateColumnsData(pDmlRowsOutput) < 0){
		lgc_errMsg("\n");
		return -1;
	}

	//success
	return 0;
}

//private member functions


//static member functions

/*
*创建InsertRedoChange
*/
LGC_InsertRedoChange* 
LGC_InsertRedoChange::createInsertRedoChange(
LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short** pLenArray, unsigned short** pLenArrayAlign, char*** pDataArray,unsigned int changeNo)
{
	LGC_InsertRedoChange* pInsertRedoChange = NULL;
	pInsertRedoChange = new LGC_InsertRedoChange(pRedoRecord,changeHeader,*pLenArray,*pLenArrayAlign,*pDataArray,changeNo);
	if(pInsertRedoChange == NULL){
		lgc_errMsg("new failed \n");
		return NULL;
	}
	*pLenArray = NULL;
	*pLenArrayAlign = NULL;
	*pDataArray = NULL;

	//success
	return pInsertRedoChange;
}




//.......................................
//class LGC_DeleteRedoChange
//第一个data内容未知；
//剩下的data用于opParser解析；
//但是如果 m_UndoChange == NULL, 
//则认为这个DeleteRedoChange 无效，
//这种情况有可能发生在执行了一个delete操作但是没有任何行被删除
//.......................................


//constructor and desctructor

/*
*构造函数
*/
LGC_DeleteRedoChange::LGC_DeleteRedoChange(
LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign, char** dataArray,unsigned int changeNo
):LGC_Change(pRedoRecord, changeHeader,lenArray, lenArrayAlign, dataArray,changeNo)
{
	m_changeType = DELETE_REDOCHANGE;
	memset(&m_logOpKdodrp,0,sizeof(log_opcode_kdodrp));
	return;
}

/*
*析构函数
*/
LGC_DeleteRedoChange::~LGC_DeleteRedoChange()
{
	return;
}

//public virtual functions

/*
*初始化
*/
int
LGC_DeleteRedoChange::init()
{

	if(!lgc_check(m_changeHeader.op_major == 11 
				&& m_changeHeader.op_minor == 3 
				&& m_datas >= 2
				&& m_lenArray[1+1] >= sizeof(log_opcode_kdodrp)))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}

	memcpy(&m_logOpKdodrp,m_dataArray[1],sizeof(m_logOpKdodrp));

	if(m_pUndoChange == NULL){
		//但是如果 m_UndoChange == NULL, 
		//则认为这个DeleteRedoChange 无效，
		//这种情况有可能发生在执行了一个delete操作但是没有任何行被删除
		//lgc_errMsg("warning: m_pUndoChange is NULL: changeNo=%u file_id=%u m_datas=%u \n", m_addr.changeNo, m_changeHeader.file_id, m_datas);
		return 0;
	}

	if(!lgc_check(m_pUndoChange 
		         && m_pUndoChange->getChangeType() != UNKOWN_UNDOCHANGE))
	{
		lgc_errMsg("m_pUndoChange invalid: %s \n", m_pUndoChange == NULL? "NULL": m_pUndoChange->toString().data());
		return -1;
	}
	
	//创建opcodeParser
	m_pOpcodeParser = LGC_OpcodeParser::createOpcodeParser(this,&m_lenArray[1+1],&m_dataArray[1],m_datas-1);
	if(m_pOpcodeParser == NULL){
		lgc_errMsg("createOpcodeParser \n");
		return -1;
	}

	//检查m_datas
	if(!lgc_check(m_pOpcodeParser->getDatas() == m_datas -1 )){
		lgc_errMsg("check failed \n");
		return -1;
	}

	return 0;
}

/*
*是否需要添加到事务中
*/
bool LGC_DeleteRedoChange::isNeedAddToTrsct()
{
	if(m_pUndoChange == NULL){
		return false;
	}

	return true;
}

/*
*获取事务Id
*/
LGC_TransactionId 
LGC_DeleteRedoChange::getTransactionId()
{
	if(m_pUndoChange == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}

	return m_pUndoChange->getTransactionId();
}

/*
*是否标志一个dml结束
*/
bool LGC_DeleteRedoChange::isEndOfDml()
{
	if(m_pUndoChange == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}
	
	return (m_pUndoChange->getSupLogFlag() & 0x04);//update 20150515 
}

/*
*获取object_id
*/
unsigned int LGC_DeleteRedoChange::getObjNum()
{
	if(m_pUndoChange == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}

	return m_pUndoChange->getObjNum();
}

/*
*获取相对于表的startColNo
*/
unsigned short LGC_DeleteRedoChange::getStartColNo()
{
	//deleteRedoChange里没有列数据
	//所以不存在startColNo信息

	//should never be called 
	lgc_errMsg("error\n");
	exit(1);
	return 0;
}

/*
*解析出列数据，并用pDmlRowsOutput输出
*/
int LGC_DeleteRedoChange::generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	if(m_pUndoChange == NULL || m_pOpcodeParser == NULL){
		lgc_errMsg("check failed \n");
		return -1;
	}

	if(m_pOpcodeParser->generateColumnsData(pDmlRowsOutput) < 0){
		lgc_errMsg("generateColumnsData failed \n");
		return -1;
	}

	//success
	return 0;
}

//private member functions

//static member functions

/*
*创建DeleteRedoChange
*/
LGC_DeleteRedoChange* 
LGC_DeleteRedoChange::createDeleteRedoChange(
LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short** pLenArray, unsigned short** pLenArrayAlign, char*** pDataArray,
unsigned int changeNo)
{
	LGC_DeleteRedoChange* pDeleteRedoChange = NULL;
	pDeleteRedoChange = new LGC_DeleteRedoChange(pRedoRecord,changeHeader,*pLenArray,*pLenArrayAlign,*pDataArray,changeNo);
	if(pDeleteRedoChange == NULL){
		lgc_errMsg("new LGC_DeleteRedoChange failed \n");
		return NULL;
	}
	*pLenArray = NULL;
	*pLenArrayAlign = NULL;
	*pDataArray = NULL;

	//success
	return pDeleteRedoChange;
}



//...................................................
//class LGC_UpdateRedoChange
//第一个data的内容未知 不解析;
//从第二个data开始，连续几个data有效
//被opParser用于解析列数据；
//接下来可能有几个data, 但是他们的内容未知, 不解析
//但是如果 m_UndoChange == NULL, 
//则认为这个UpdateRedoChange 无效，
//这种情况有可能发生在执行了一个update操作
//但是没有任何行被更新
//....................................................


//constructor and desctructor
/*
*构造函数
*/
LGC_UpdateRedoChange::LGC_UpdateRedoChange(
LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign, char** dataArray,unsigned int changeNo
):LGC_Change(pRedoRecord, changeHeader,lenArray, lenArrayAlign, dataArray,changeNo)
{
	m_changeType = UPDATE_REDOCHANGE;
	memset(&m_logOpKdourp,0,sizeof(log_opcode_kdourp));
	return;
}

/*
*析构函数
*/
LGC_UpdateRedoChange::~LGC_UpdateRedoChange()
{
	return;
}

//public virtual functions

/*
*初始化
*主要做的事情: 检查成员属性的合法性，创建OpcodeParser, 检查m_datas的合法性
*/
int LGC_UpdateRedoChange::init()
{
	if(!(m_changeHeader.op_major == 11 && m_changeHeader.op_minor == 5
		  && m_datas >= 2 && m_lenArray[1+1] >= sizeof(log_opcode_kdourp) )){
		lgc_errMsg("check failed \n");
		return -1;
	}
	
	memcpy(&m_logOpKdourp,m_dataArray[1],sizeof(log_opcode_kdourp));

	if(m_pUndoChange == NULL){
		//但是如果 m_UndoChange == NULL, 
		//则认为这个UpdateRedoChange 无效，
		//这种情况有可能发生在执行了一个update操作
		//但是没有任何行被更新
		//lgc_errMsg("warning: m_pUndoChange is NULL: changeNo=%u fileId=%u m_datas=%u, ncol=%u nnew=%u size=%u\n",
		//				m_addr.changeNo, m_changeHeader.file_id, m_datas, m_logOpKdourp.ncol, m_logOpKdourp.nnew, m_logOpKdourp.size);
		return 0;	
	}

	if(!lgc_check(m_pUndoChange 
		          && m_pUndoChange->getChangeType() != UNKOWN_UNDOCHANGE))
	{
		lgc_errMsg("m_pUndoChange invalid: %s \n", m_pUndoChange == NULL? "undoChange is null":"changeType is not Undo");
		return -1;
	}
	
	//创建OpcodeParser
	m_pOpcodeParser = LGC_OpcodeParser::createOpcodeParser(this,&m_lenArray[1+1],&m_dataArray[1],m_datas-1);
	if(m_pOpcodeParser == NULL){
		lgc_errMsg("createOpcodeParser failed \n");
		return -1;
	}
	
	//如果最后有未知data，不解析
	if(!lgc_check(m_pOpcodeParser->getDatas() <= m_datas -1 )){
		lgc_errMsg("check failed: m_pOpcodeParser->getDatas()=%u m_datas=%u \n",
							m_pOpcodeParser->getDatas(), m_datas);
		return -1;
	}

	return 0;


}

/*
*是否需要添加到事务中
*/
bool LGC_UpdateRedoChange::isNeedAddToTrsct()
{
	if(m_pUndoChange == NULL){
		return false;
	}

	return true;
}

/*
*获取事务Id
*/
LGC_TransactionId 
LGC_UpdateRedoChange::getTransactionId()
{
	
	if(m_pUndoChange == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}

	return m_pUndoChange->getTransactionId();
}

/*
*是否有dml结束的标记
*/
bool LGC_UpdateRedoChange::isEndOfDml()
{
	if(m_pUndoChange == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}

//	return false;
	return (m_pUndoChange->getSupLogFlag() & 0x04);//update 20150515 
}

/*
*获取object_id
*/
unsigned int LGC_UpdateRedoChange::getObjNum()
{
	if(m_pUndoChange == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}

	return m_pUndoChange->getObjNum();
}

/*
*获取相对于表的startColNo
*/
unsigned short LGC_UpdateRedoChange::getStartColNo()
{
	if(m_pUndoChange == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}

	return m_pUndoChange->getStartColNo();
}

/*
*解析出列数据，并用pDmlRowsOutputRowsOutput输出
*/
int LGC_UpdateRedoChange::generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	if(m_pUndoChange == NULL || m_pOpcodeParser == NULL){
		lgc_errMsg("check failed \n");
		return -1;
	}

	if(m_pOpcodeParser->generateColumnsData(pDmlRowsOutput) < 0 ){
		lgc_errMsg("generateColumnsData failed \n");
		return -1;
	}

	//success
	return 0;
}

//private member functions 


//static member functions

/*
*创建UpdateRedoChange
*/
LGC_UpdateRedoChange* 
LGC_UpdateRedoChange::createUpdateRedoChange(
LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short** pLenArray, unsigned short** pLenArrayAlign, char*** pDataArray,unsigned int changeNo)
{
	LGC_UpdateRedoChange* pUpdateRedoChange = NULL;
	pUpdateRedoChange = new LGC_UpdateRedoChange(pRedoRecord,changeHeader,*pLenArray,*pLenArrayAlign,*pDataArray,changeNo);
	if(pUpdateRedoChange == NULL){
		return NULL;
	}
	*pLenArray = NULL;
	*pLenArrayAlign = NULL;
	*pDataArray = NULL;

	//success
	return pUpdateRedoChange;
}


//
//.......................................
//class LGC_MultiInsertRedoChange
//第一个data未知
//第二个data放的是log_opcode_kdoqm
//第三个data放的是多行的数据
//一次向数据库中插入多行数据
//.......................................


//constructor and desctructor

/*
*构造函数
*/
LGC_MultiInsertRedoChange::LGC_MultiInsertRedoChange(
LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign, char** dataArray,unsigned int changeNo
):LGC_Change(pRedoRecord, changeHeader,lenArray, lenArrayAlign, dataArray,changeNo)
{
	m_changeType = MULTIINSERT_REDOCHANGE;
	memset(&m_logOpKdoqm,0,sizeof(log_opcode_kdoqm));
	return;
}

/*
*析构函数
*/
LGC_MultiInsertRedoChange::~LGC_MultiInsertRedoChange()
{
	return;
}

//public virtual functions

/*
*初始化
*/
int LGC_MultiInsertRedoChange::init()
{
	if(!lgc_check(m_changeHeader.op_major == 11 
		         && m_changeHeader.op_minor == 11
		         && m_datas >= 2 
		         && m_lenArray[1+1] >= sizeof(log_opcode_kdoqm)))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}

	memcpy(&m_logOpKdoqm,m_dataArray[1], sizeof(log_opcode_kdoqm));
	
	if(!lgc_check(m_pUndoChange 
		         && m_pUndoChange->getChangeType() != UNKOWN_UNDOCHANGE))
	{
		lgc_errMsg("m_pUndoChange invalid \n");
		return -1;
	}
	
	//创建OpcodeParser
	m_pOpcodeParser = LGC_OpcodeParser::createOpcodeParser(this,&m_lenArray[1+1],&m_dataArray[1],m_datas-1);
	if(m_pOpcodeParser == NULL){
		lgc_errMsg("createOpcodeParser failed \n");
		return -1;
	}
	
	//check: 
	if(!lgc_check(m_pOpcodeParser->getDatas() == m_datas -1 )){
		lgc_errMsg("check failed \n");
		return -1;
	}

	//success
	return 0;
}

/*
*是否需要添加到事务中
*/
bool LGC_MultiInsertRedoChange::isNeedAddToTrsct()
{
	return true;
}

/*
*获取事务Id
*/
LGC_TransactionId 
LGC_MultiInsertRedoChange::getTransactionId()
{
	if(m_pUndoChange == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}

	return m_pUndoChange->getTransactionId();
}

/*
*是否包含dml结束的标志
*/
bool LGC_MultiInsertRedoChange::isEndOfDml()
{
	return true;
}

/*
*获取object_id
*/
unsigned int LGC_MultiInsertRedoChange::getObjNum()
{
	if(m_pUndoChange == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}

	return m_pUndoChange->getObjNum();
}

/*
*获取相对于表的startColNo
*不提供该接口
*/
unsigned short LGC_MultiInsertRedoChange::getStartColNo()
{
	//should never be called
	lgc_errMsg("error\n");
	exit(1);
	return 0;
}

/*
*解析出里面的列数据，并用pDmlRowsOutput输出
*不提供该接口
*/
int LGC_MultiInsertRedoChange::generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	//shold never be called 
	lgc_errMsg("error\n");
	exit(1);
	return -1;
}

//public member functions

/*
*解析出里面的行数据(多行), 并用pDmlRowsOutput输出
*/
int LGC_MultiInsertRedoChange::generateRowsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
fprintf(stdout, "LGC_MultiInsertRedoChange:%s\n", this->toString().data());

	if(m_pUndoChange == NULL || m_pOpcodeParser == NULL){
		lgc_errMsg("check failed \n");
		return -1;
	}
	
	if(m_changeType != MULTIINSERT_REDOCHANGE){
		lgc_errMsg("%s \n", this->toString().data());
		return -1;
	}

	LGC_OpKdoqmParser* pOpKdoqmParser = (LGC_OpKdoqmParser*)m_pOpcodeParser;
	if(pOpKdoqmParser->generateRowsData(pDmlRowsOutput) < 0){
		lgc_errMsg("generateRowsData failed \n");
		return -1;
	}
	
	//success
	return 0;
}

//private member functions


//static member functions

/*
*创建MultiInsertRedoChange
*/
LGC_MultiInsertRedoChange* 
LGC_MultiInsertRedoChange::createMultiInsertRedoChange(
LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short** pLenArray, unsigned short** pLenArrayAlign, char*** pDataArray,unsigned int changeNo)
{
	LGC_MultiInsertRedoChange* pMultiInsertRedoChange =NULL;
	pMultiInsertRedoChange = new LGC_MultiInsertRedoChange(pRedoRecord,changeHeader,*pLenArray,*pLenArrayAlign,*pDataArray,changeNo);
	if(pMultiInsertRedoChange == NULL){
		return NULL;
	}
	*pLenArray = NULL;
	*pLenArrayAlign = NULL;
	*pDataArray = NULL;

	//success
	return pMultiInsertRedoChange;
}


//.......................................
//class LGC_RowChainRedoChange
//行连接的时候产生change
//第一个data未知
//第二个data是log_opcode_kdoirp
//接下来的就是列数据 
//.......................................


//constructor and desctructor

/*
*构造函数
*/
LGC_RowChainRedoChange::LGC_RowChainRedoChange(
LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign, char** dataArray,unsigned int changeNo
):LGC_Change(pRedoRecord, changeHeader,lenArray, lenArrayAlign, dataArray, changeNo)
{
	m_changeType = ROWCHAIN_REDOCHANGE;
	memset(&m_logOpKdoirp,0,sizeof(m_logOpKdoirp));
	return;
}

/*
*析构函数
*/
LGC_RowChainRedoChange::~LGC_RowChainRedoChange()
{
	return;
}

//public virtual functions

/*
*初始化
*主要做的事情: 检查成员属性的合法性，
*              创建OpcodeParser,
*              检查OpcodeParser的合法性
*/
int LGC_RowChainRedoChange::init()
{
	if(!lgc_check(m_changeHeader.op_major == 11 
		          && m_changeHeader.op_minor == 6
		          && m_datas >= 2 
		          && m_lenArray[1+1] >= sizeof(m_logOpKdoirp)))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}
	
	if(!lgc_check(m_pUndoChange 
		         && m_pUndoChange->getChangeType() != UNKOWN_UNDOCHANGE))
	{
		lgc_errMsg("m_pUndoChange invalid \n");
		return -1;
	}

	memcpy(&m_logOpKdoirp,m_dataArray[1],sizeof(m_logOpKdoirp));
	
	//创建OpecodeParser
	m_pOpcodeParser = LGC_OpcodeParser::createOpcodeParser(this,&m_lenArray[1+1],&m_dataArray[1],m_datas-1);
	if(m_pOpcodeParser == NULL){
		lgc_errMsg("createOpcodeParser failed \n");
		return -1;
	}

	//check m_datas
	if(!lgc_check(m_pOpcodeParser->getDatas() == m_datas -1 )){
		lgc_errMsg("check failed \n");
		return -1;
	}

	//success
	return 0;
}

/*
*是否需要添加给事务
*/
bool LGC_RowChainRedoChange::isNeedAddToTrsct()
{
	return true;
}

/*
*获取事务Id
*/
LGC_TransactionId 
LGC_RowChainRedoChange::getTransactionId()
{
	
	if(m_pUndoChange == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}

	return m_pUndoChange->getTransactionId();
}

/*
*是否包含Dml结束的标记
*/
bool LGC_RowChainRedoChange::isEndOfDml()
{
	if(m_pUndoChange == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}

	return (m_pUndoChange->getSupLogFlag() & 0x04);//update 20150515 
}

/*
*获取object_id
*/
unsigned int LGC_RowChainRedoChange::getObjNum()
{
	if(m_pUndoChange == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}

	return m_pUndoChange->getObjNum();
}

/*
*获取相对于表的startColNo
*/
unsigned short LGC_RowChainRedoChange::getStartColNo()
{
	if(m_pUndoChange == NULL){
		lgc_errMsg("error\n");
		exit(1);
	}

	return m_pUndoChange->getStartColNo();
}

/*
*解析出列数据， 并用pDmlRowsOutput输出
*/
int LGC_RowChainRedoChange::generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	if(m_pUndoChange == NULL || m_pOpcodeParser == NULL){
		lgc_errMsg("check failed \n");
		return -1;
	}

	if(m_pOpcodeParser->generateColumnsData(pDmlRowsOutput)){
		lgc_errMsg("generateColumnsData failed \n");
		return -1;
	}

	//success
	return 0;
}

//private member functions


//static member functions

/*
*创建RowChainRedoChange
*/
LGC_RowChainRedoChange* 
LGC_RowChainRedoChange::createRowChainRedoChange(
LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short** pLenArray, unsigned short** pLenArrayAlign, char*** pDataArray,unsigned int changeNo)
{
	LGC_RowChainRedoChange* pRowChainRedoChange = NULL;
	pRowChainRedoChange = new LGC_RowChainRedoChange(pRedoRecord,changeHeader,*pLenArray,*pLenArrayAlign,*pDataArray,changeNo);
	if(!pRowChainRedoChange){
		return NULL;
	}
	*pLenArray = NULL;
	*pLenArrayAlign = NULL;
	*pDataArray = NULL;

	//success
	return pRowChainRedoChange;
}

//.......................................
//class LGC_BeginTChange
//开始一个新的事务时，产生该类型的change
//第一个data未知
//第二个data是log_opcode_52
//.......................................

//constructor and desctructor

/*
*构造函数
*/
LGC_BeginTChange::LGC_BeginTChange(
LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign, char** dataArray, unsigned int changeNo
):LGC_Change(pRedoRecord, changeHeader,lenArray, lenArrayAlign, dataArray, changeNo)
{
	m_changeType = BEGINT_CHANGE;
	memset(&m_logOp52,0,sizeof(m_logOp52));
	m_beginSCN = pRedoRecord->getRecordSCN();
	return;
}

/*
*析构函数
*/
LGC_BeginTChange::~LGC_BeginTChange()
{
	return;
}

//public virtual functions

/*
*初始化
*/
int LGC_BeginTChange::init()
{
	if(!lgc_check(m_changeHeader.op_major == 5 
		         && m_changeHeader.op_minor == 2
		         && m_datas >= 1 
		         && m_lenArray[1+0] >= sizeof(m_logOp52)))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}
	memcpy(&m_logOp52,m_dataArray[0],sizeof(m_logOp52));

	//success
	return 0;
}

/*
*是否需要添加给事务
*/
bool LGC_BeginTChange::isNeedAddToTrsct()
{
	return (m_logOp52.seq != 0);
}

/*
*获取事务Id
*/
LGC_TransactionId 
LGC_BeginTChange::getTransactionId()
{
	LGC_TransactionId transactionId;
	transactionId.xidSlot = this->getXidSlot();
	transactionId.xidSqn = this->getXidSqn();

	return transactionId;
}

bool LGC_BeginTChange::isEndOfDml()
{
	//shoult never be called 
	lgc_errMsg("error\n");
	exit(1);
	return false;
}

unsigned int LGC_BeginTChange::getObjNum()
{
	//should never be called
	return 0;
	lgc_errMsg("error\n");
	exit(1);
	return 0;
}

unsigned short LGC_BeginTChange::getStartColNo()
{
	//should never be called 
	lgc_errMsg("error\n");
	exit(1);
	return 0;

}
int LGC_BeginTChange::generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	//should never be called 
	lgc_errMsg("error\n");
	exit(1);
	return 0;

}
//private member functions

//some get or set functions
//public

/*
*获取事务的开始scn
*/
BYTE8 LGC_BeginTChange::getTrsctBeginSCN() const
{
	return m_beginSCN;

}

//static member functions
/*
*创建BeginTChange
*/
LGC_BeginTChange* 
LGC_BeginTChange::createBeginTChange(
LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short** pLenArray, unsigned short** pLenArrayAlign, char*** pDataArray,unsigned int changeNo)
{
	LGC_BeginTChange* pBeginTChange = NULL;
	pBeginTChange = new LGC_BeginTChange(pRedoRecord,changeHeader,*pLenArray,*pLenArrayAlign,*pDataArray,changeNo);
	if(!pBeginTChange){
		return NULL;
	}
	*pLenArray = NULL;
	*pLenArrayAlign = NULL;
	*pDataArray = NULL;

	//success
	return pBeginTChange;

}


//.......................................
//class LGC_EndTChange
//.......................................

//constructor and desctructor

/*
*构造函数
*/
LGC_EndTChange::LGC_EndTChange(
LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign, char** dataArray,unsigned int changeNo
):LGC_Change(pRedoRecord, changeHeader,lenArray, lenArrayAlign, dataArray,changeNo)
{
	m_changeType = ENDT_CHANGE;
	memset(&m_logOp54,0,sizeof(m_logOp54));
	m_commitSCN = pRedoRecord->getRecordSCN();

	return;
}

/*
*析构函数
*/
LGC_EndTChange::~LGC_EndTChange()
{
	return;
}


//public virtual functions

/*
*初始化
*/
int LGC_EndTChange::init()
{
	if(!lgc_check(m_changeHeader.op_major == 5 
		         && m_changeHeader.op_minor == 4
		         && m_datas >= 1 
		         && m_lenArray[1+0] >= sizeof(m_logOp54)))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}

	memcpy(&m_logOp54,m_dataArray[0],sizeof(m_logOp54));
	
	//success
	return 0;

}

/*
*是否需要添加到事务
*/
bool LGC_EndTChange::isNeedAddToTrsct()
{
	return true;
}

/*
*获取事务Id
*/
LGC_TransactionId 
LGC_EndTChange::getTransactionId()
{
	LGC_TransactionId transactionId;
	transactionId.xidSlot = this->getXidSlot();
	transactionId.xidSqn = this->getXidSqn();

	return transactionId;
}

bool LGC_EndTChange::isEndOfDml()
{
	//should never be called 
	lgc_errMsg("error\n");
	exit(1);
	return false;
}

unsigned int LGC_EndTChange::getObjNum()
{
	//should never be called
	return 0;
	lgc_errMsg("error\n");
	exit(1);
	return 0;
}

unsigned short LGC_EndTChange::getStartColNo()
{
	//should never be called 
	lgc_errMsg("error\n");
	exit(1);
	return 0;
}

int LGC_EndTChange::generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	//should never be called
	lgc_errMsg("error\n");
	exit(1);
	return 0;
}

//private member functions

//some get or set functions
//public

/*
*获取事务提交scn
*/
BYTE8 LGC_EndTChange::getCommitSCN() const
{
	return m_commitSCN;

}

/*
*事务是否回滚
*/
bool LGC_EndTChange::isTrsctRollback() const
{
	return (m_logOp54.flg&4);
}


//static member functions
/*
*创建EndTChange
*/
LGC_EndTChange* 
LGC_EndTChange::createEndTChange(
LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short** pLenArray, unsigned short** pLenArrayAlign, char*** pDataArray,unsigned int changeNo)
{
	LGC_EndTChange* pEndTChange = NULL;
	pEndTChange = new LGC_EndTChange(pRedoRecord,changeHeader,*pLenArray,*pLenArrayAlign,*pDataArray,changeNo);
	if(!pEndTChange){
		return NULL;
	}
	*pLenArray = NULL;
	*pLenArrayAlign = NULL;
	*pDataArray = NULL;

	//success
	return pEndTChange;
}


//.......................................
//class LGC_UnkownChange
//.......................................

//constructor and desctructor
LGC_UnkownChange::LGC_UnkownChange(
LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short* lenArray, unsigned short* lenArrayAlign, char** dataArray,unsigned int changeNo
):LGC_Change(pRedoRecord, changeHeader,lenArray, lenArrayAlign, dataArray,changeNo)
{
	m_changeType = UNKOWN_CHANGE;
	return;
}

LGC_UnkownChange::~LGC_UnkownChange()
{
	return;
}


//public virtual functions
int LGC_UnkownChange::init()
{
	return 0;
}

bool LGC_UnkownChange::isNeedAddToTrsct()
{
	return false;
}

LGC_TransactionId 
LGC_UnkownChange::getTransactionId()
{
	//short never be called 
	lgc_errMsg("error\n");
	exit(1);

	return *(new LGC_TransactionId);
}

bool LGC_UnkownChange::isEndOfDml()
{
	//should never be called 
	lgc_errMsg("error\n");
	exit(1); 
	return false;
}

unsigned int LGC_UnkownChange::getObjNum()
{
	//should never be called
	lgc_errMsg("error\n");
	exit(1);
	return 0;
}

unsigned short LGC_UnkownChange::getStartColNo()
{
	//should never be called 
	lgc_errMsg("error\n");
	exit(1);
	return 0;
}

int LGC_UnkownChange::generateColumnsData(LGC_DmlRowsOutput* pDmlRowsOutput)
{
	//should never be called
	lgc_errMsg("error\n");
	exit(1);
	return 0;
}

//static member functions
LGC_UnkownChange* 
LGC_UnkownChange::createUnkownChange(
LGC_RedoRecord* pRedoRecord, const log_changeVector_header& changeHeader, unsigned short** pLenArray, unsigned short** pLenArrayAlign, char*** pDataArray,unsigned int changeNo)
{
	LGC_UnkownChange* pUnkownChange = NULL;
	pUnkownChange = new LGC_UnkownChange(pRedoRecord,changeHeader,*pLenArray,*pLenArrayAlign,*pDataArray,changeNo);
	if(!pUnkownChange){
		return NULL;
	}
	*pLenArray = NULL;
	*pLenArrayAlign = NULL;
	*pDataArray = NULL;

	//success
	return pUnkownChange;
}



//helper class

//....................................................
//class LGC_DataArrayDump
//用来间change的dataArray以十六进制文本形式导出
//....................................................
LGC_DataArrayDump::LGC_DataArrayDump(char** dataArray, unsigned short* lenArray, unsigned short datas)
{
	m_dataArray = dataArray;
	m_lenArray = lenArray;
	m_datas = datas;
	return;
}

LGC_DataArrayDump::~LGC_DataArrayDump()
{
	return;
}


char* LGC_DataArrayDump::getHexDump()
{
	return NULL;
}
