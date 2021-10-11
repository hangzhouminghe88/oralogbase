#include "lgc_Change.h"
#include "lgc_OpcodeParser.h"
#include "lgc_RedoRecord.h"
#include "lgc_DmlRowsOutput.h"
#include "lgc_api.h"


//..............................................................
//Abstract Class LGC_Change
//change����Ҫ���: changeHeader, lenArray, dataArray;
//changeHeader���¼��change�����͵���Ϣ;
//lenArray��¼��dataArray��ÿ��data�ĳ��ȣ�
//����lenArray[0]��¼����lenArray����ĳ���;
//dataArray���data��Ҫ����������
//..............................................................

BYTE8 LGC_Change::s_createTimes = 0;
BYTE8 LGC_Change::s_freeTimes = 0;

//constructor and desctructor

/*
*���캯��
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
*��������
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



//��Щ���з�����Ӧ�ñ��������ػ��߸���

/*
*����changeHeader���¼��change������Ϣ�ж��Ƿ���undoChange
*/
bool LGC_Change::isUndoChange() const
{
	return (m_changeHeader.op_major == 5 && m_changeHeader.op_minor == 1);
}

/*
*����undoChange
*��ǰ��change��Ҫ������undoChange��һЩ��Ϣ
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
*��ȡThreadId
*rac�����ж���ڵ��ڹ�����ÿ���ڵ����Լ���ThreadId
*/
unsigned short LGC_Change::getThreadId() const
{
	return m_addr.threadId;
}

/*
*��ȡchange��scn��
*/
BYTE8 LGC_Change::getSCN() const
{
	BYTE8 scn = m_changeHeader.high_scn;
	scn <<=32;
	scn += m_changeHeader.low_scn;
	return scn;
}

/*
*��ȡ��������
*�������Ǹ�����Ҫ����Ϣ��
*��ֱ�Ӿ�����dataArray���������������ʲô��
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
*��ȡ�β�����
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
*��change��ĳЩ����ת��Ϊstring
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
*����changeHeader���¼��������Ϣ����change��Ӧ��change����
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
*�ͷ�change
*���е�change����Ӧ��ͨ�������̬�����ͷ�
*��Ӧ����deleteֱ���ͷ�
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
//UndoChange��dataArray��Ҫ�����������:
//undoInfo�õ�subDataArray��opcodeParser�õ�subDataArray, 
//supplementalParser�õ�subDataArray;
//undoInfo�õ��ǵ�һ���͵ڶ���data;
//opcodeParser�õ����м��data;
//supplementalParser�õ�����󼸸�data;
//.......................................

//constructor and desctructor

/*
*���캯��
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
	//ָʾ��һ��redoChange�����ڵ�ǰ��
	//undoChange, ����ɾ����ǰundo�����ν�����
	m_pRedoChange = NULL;

	return;
}


/*
*��������
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
*��ʼ��
*��Ҫ����: ���Ϸ��������changeType 
*          ����opcodeParser,  ����supplementalParser
*undoChange��ʼ�������ṩ�Ĺ��нӿڲ��ܱ�����
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

	//���(m_logOp51Second.op_major,m_logOp51Second.op_minor) not in [(11,1),(5,1)], 
	//�����UndoChange��������˵�����õģ���Ϊ��������dml����
	if ( !lgc_check((m_logOp51Second.op_major == 11 || m_logOp51Second.op_major == 5) 
		             && m_logOp51Second.op_minor == 1))
	{// not dml data
		return 0;
	}
	
	//���m_datas < 4 �����UndoChange��������˵�����õ�
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
	
	//�����changeType
	ChangeType changeType = this->calculateUndoChangeType();
	m_changeType = changeType;
	
	//����opcodeParser,  ����supplementalParser
	if(this->buildParsers() < 0){
		lgc_errMsg("this->buildParsers failed \n");
		return -1;
	}

	//success
	return 0;
}

/*
*�Ƿ���Ҫ��Ӹ�����
*ֻ�б��ύ�������change�Żᱻ����
*/
bool LGC_UndoChange::isNeedAddToTrsct()
{
	if(m_pRedoChange){
		//�����󴫸���Ӧ��RedoChange
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
*��ȡ����Id
*/
LGC_TransactionId LGC_UndoChange::getTransactionId()
{
	LGC_TransactionId transactionId;
	transactionId.xidSlot = this->getXidSlot();
	transactionId.xidSqn  = this->getXidSqn();

	return transactionId;
}

/*
*�Ƿ���dml�����һ��Change
*/
bool LGC_UndoChange::isEndOfDml()
{	
	if(m_changeType == UNKOWN_UNDOCHANGE){

		lgc_errMsg("m_changeType invalid \n");
		exit(1);

	}else if(   m_changeType == MFC_UNDOCHANGE 
		     || m_changeType == LMN_UNDOCHANGE)
	{//��ЩUndoChangeû�ж�Ӧ��redo change,
	 //������Ҫ�ж�isEndOfDml

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
	
	}else{//��ЩUndoChange�ж�Ӧ��redo change,
	      //���Բ���Ҫ�ж�isEndOfDml
		  //���Ǿ������Ժ������º��ȷû�ж�Ӧ��redoChange
		
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
*��ȡObjNum
*ÿ�����ж�Ӧ��Object_id, ���Դ�dba_objects���в鵽
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
*��ȡ����ڱ����ʼ�к�
*m_dataArray���ж��е����ݣ��кŵ�����
*��С���к�����������ݿ��, ����������ڱ��
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
*������dataArray��������ݣ�����pDmlRowsOutput���
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
*����UndoChange���еĽ�������
*/
int LGC_UndoChange::buildParsers()
{	
	if(m_changeType != UNKOWN_UNDOCHANGE 
		&& m_changeType != MULTIINSERT_UNDOCHANGE
		&& m_changeType != MULTIDELETE_UNDOCHANGE)
	{
		//����opcodeParser
		LGC_OpcodeParser* pOpcodeParser = this->createOpcodeParser();
		if(pOpcodeParser == NULL){
			lgc_errMsg("createOpcodeParser failed \n");
			return -1;
		}
		m_pOpcodeParser = pOpcodeParser;
		pOpcodeParser = NULL;
		
		//�����undoChange�Ĳ�����־����ʼλ��
		//���λ��Ҳ��supplementParser��ʼ������λ��
		unsigned short supLogOff = this->calculateSupLogOff();
		if(!(supLogOff > 3)){
			lgc_errMsg("calculateSupLogOff failed \n");
			return -1;
		}
		m_supLogOff = supLogOff;
		
		//����supplementaParser
		LGC_SupplementalParser* pSupplementalParser = this->createSupplementalParser();
		if(pSupplementalParser == NULL){
			lgc_errMsg("createSupplementalParser failed \n");
			return -1;
		}
		m_pSupplementalParser = pSupplementalParser;
		pSupplementalParser = NULL;
	}
	
	//���m_datas�Ƿ�Ϸ�
	if(!this->checkDataNum()){
		lgc_errMsg("checkDataNum failed \n");
		return -1;
	}

	//success
	return 0;
}

/*
*���m_datas�Ƿ�Ϸ�
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
*����������Ϣ�����UndoChangeType
*/
ChangeType LGC_UndoChange::calculateUndoChangeType()
{
	// if (m_logOp51Second.op_major, m_logOp51Second.op_minor) not in [(11,1),(5,1)] or !(m_datas >= 4) or !(m_lenArray[1+3] >= 12) 
	// then undoChange's type is unkown
	// ����ֻ����(m_logOp51Second.op_major, m_logOp51Second.op_minor)Ϊ[(11,1), (5,1)]��undo change vector; 
	// ������Ҫm_dataArray�ĵ��ĸ��ṹm_logOp51Kdo ����m_datas >= 4;
	// ������Ҫm_logOp51Kdo���opcode, ����len(m_data[3]) >= sizeof(m_logOpKdo);

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
		//����Ҫ���������column����
		return MULTIINSERT_UNDOCHANGE;

	}else if(m_logOp51Second.op_major == 11 && m_logOp51Second.op_minor == 1 && (m_logOp51Kdo.opcode&0x1F) == 12){
		//����Ҫ���������column����
		return MULTIDELETE_UNDOCHANGE;

	}else{
		//undo undo change
		//��Щundo change �Ҳ�֪����ô�������ǵĲ�����־��λ��
		return UNKOWN_UNDOCHANGE;
	}
}

/*
*����undoChange�Ĳ�����־λ��
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
*����opcodeParser
*/
LGC_OpcodeParser* LGC_UndoChange::createOpcodeParser()
{
	return LGC_OpcodeParser::createOpcodeParser(this, &m_lenArray[3+1], &m_dataArray[3],m_datas-3);
}

/*
*����supplementalParser
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
*����UndoChange
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
*�ͷ�UndoChange
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
//��һ�����ݿ��в��������ݲ�����change
//.......................................


//constructor and desctructor

/*
*���캯��
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
*��������
*/
LGC_InsertRedoChange::~LGC_InsertRedoChange()
{
	return;
}

//public virtual functions

/*
*��ʼ��
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
	
	//����OpcodeParser
	m_pOpcodeParser = LGC_OpcodeParser::createOpcodeParser(this,&m_lenArray[1+1],&m_dataArray[1],m_datas-1);
	if(m_pOpcodeParser == NULL){
		lgc_errMsg("\n");
		return -1;
	}

	//���m_datas
	if(!lgc_check(m_pOpcodeParser->getDatas()  == m_datas -1 )){
		lgc_errMsg("\n");
		return -1;
	}

	return 0;
}

/*
*�Ƿ���Ҫ��������������
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
*��ȡ����Id
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
*change���Ƿ���dmlEnd�ı�־
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
*��ȡobject_id
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
*��ȡ����ڱ��startColNo
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
*�����������ݣ�����pDmlRowsOutput���
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
*����InsertRedoChange
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
//��һ��data����δ֪��
//ʣ�µ�data����opParser������
//������� m_UndoChange == NULL, 
//����Ϊ���DeleteRedoChange ��Ч��
//��������п��ܷ�����ִ����һ��delete��������û���κ��б�ɾ��
//.......................................


//constructor and desctructor

/*
*���캯��
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
*��������
*/
LGC_DeleteRedoChange::~LGC_DeleteRedoChange()
{
	return;
}

//public virtual functions

/*
*��ʼ��
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
		//������� m_UndoChange == NULL, 
		//����Ϊ���DeleteRedoChange ��Ч��
		//��������п��ܷ�����ִ����һ��delete��������û���κ��б�ɾ��
		//lgc_errMsg("warning: m_pUndoChange is NULL: changeNo=%u file_id=%u m_datas=%u \n", m_addr.changeNo, m_changeHeader.file_id, m_datas);
		return 0;
	}

	if(!lgc_check(m_pUndoChange 
		         && m_pUndoChange->getChangeType() != UNKOWN_UNDOCHANGE))
	{
		lgc_errMsg("m_pUndoChange invalid: %s \n", m_pUndoChange == NULL? "NULL": m_pUndoChange->toString().data());
		return -1;
	}
	
	//����opcodeParser
	m_pOpcodeParser = LGC_OpcodeParser::createOpcodeParser(this,&m_lenArray[1+1],&m_dataArray[1],m_datas-1);
	if(m_pOpcodeParser == NULL){
		lgc_errMsg("createOpcodeParser \n");
		return -1;
	}

	//���m_datas
	if(!lgc_check(m_pOpcodeParser->getDatas() == m_datas -1 )){
		lgc_errMsg("check failed \n");
		return -1;
	}

	return 0;
}

/*
*�Ƿ���Ҫ��ӵ�������
*/
bool LGC_DeleteRedoChange::isNeedAddToTrsct()
{
	if(m_pUndoChange == NULL){
		return false;
	}

	return true;
}

/*
*��ȡ����Id
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
*�Ƿ��־һ��dml����
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
*��ȡobject_id
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
*��ȡ����ڱ��startColNo
*/
unsigned short LGC_DeleteRedoChange::getStartColNo()
{
	//deleteRedoChange��û��������
	//���Բ�����startColNo��Ϣ

	//should never be called 
	lgc_errMsg("error\n");
	exit(1);
	return 0;
}

/*
*�����������ݣ�����pDmlRowsOutput���
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
*����DeleteRedoChange
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
//��һ��data������δ֪ ������;
//�ӵڶ���data��ʼ����������data��Ч
//��opParser���ڽ��������ݣ�
//�����������м���data, �������ǵ�����δ֪, ������
//������� m_UndoChange == NULL, 
//����Ϊ���UpdateRedoChange ��Ч��
//��������п��ܷ�����ִ����һ��update����
//����û���κ��б�����
//....................................................


//constructor and desctructor
/*
*���캯��
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
*��������
*/
LGC_UpdateRedoChange::~LGC_UpdateRedoChange()
{
	return;
}

//public virtual functions

/*
*��ʼ��
*��Ҫ��������: ����Ա���ԵĺϷ��ԣ�����OpcodeParser, ���m_datas�ĺϷ���
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
		//������� m_UndoChange == NULL, 
		//����Ϊ���UpdateRedoChange ��Ч��
		//��������п��ܷ�����ִ����һ��update����
		//����û���κ��б�����
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
	
	//����OpcodeParser
	m_pOpcodeParser = LGC_OpcodeParser::createOpcodeParser(this,&m_lenArray[1+1],&m_dataArray[1],m_datas-1);
	if(m_pOpcodeParser == NULL){
		lgc_errMsg("createOpcodeParser failed \n");
		return -1;
	}
	
	//��������δ֪data��������
	if(!lgc_check(m_pOpcodeParser->getDatas() <= m_datas -1 )){
		lgc_errMsg("check failed: m_pOpcodeParser->getDatas()=%u m_datas=%u \n",
							m_pOpcodeParser->getDatas(), m_datas);
		return -1;
	}

	return 0;


}

/*
*�Ƿ���Ҫ��ӵ�������
*/
bool LGC_UpdateRedoChange::isNeedAddToTrsct()
{
	if(m_pUndoChange == NULL){
		return false;
	}

	return true;
}

/*
*��ȡ����Id
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
*�Ƿ���dml�����ı��
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
*��ȡobject_id
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
*��ȡ����ڱ��startColNo
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
*�����������ݣ�����pDmlRowsOutputRowsOutput���
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
*����UpdateRedoChange
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
//��һ��dataδ֪
//�ڶ���data�ŵ���log_opcode_kdoqm
//������data�ŵ��Ƕ��е�����
//һ�������ݿ��в����������
//.......................................


//constructor and desctructor

/*
*���캯��
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
*��������
*/
LGC_MultiInsertRedoChange::~LGC_MultiInsertRedoChange()
{
	return;
}

//public virtual functions

/*
*��ʼ��
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
	
	//����OpcodeParser
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
*�Ƿ���Ҫ��ӵ�������
*/
bool LGC_MultiInsertRedoChange::isNeedAddToTrsct()
{
	return true;
}

/*
*��ȡ����Id
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
*�Ƿ����dml�����ı�־
*/
bool LGC_MultiInsertRedoChange::isEndOfDml()
{
	return true;
}

/*
*��ȡobject_id
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
*��ȡ����ڱ��startColNo
*���ṩ�ýӿ�
*/
unsigned short LGC_MultiInsertRedoChange::getStartColNo()
{
	//should never be called
	lgc_errMsg("error\n");
	exit(1);
	return 0;
}

/*
*����������������ݣ�����pDmlRowsOutput���
*���ṩ�ýӿ�
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
*�����������������(����), ����pDmlRowsOutput���
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
*����MultiInsertRedoChange
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
//�����ӵ�ʱ�����change
//��һ��dataδ֪
//�ڶ���data��log_opcode_kdoirp
//�������ľ��������� 
//.......................................


//constructor and desctructor

/*
*���캯��
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
*��������
*/
LGC_RowChainRedoChange::~LGC_RowChainRedoChange()
{
	return;
}

//public virtual functions

/*
*��ʼ��
*��Ҫ��������: ����Ա���ԵĺϷ��ԣ�
*              ����OpcodeParser,
*              ���OpcodeParser�ĺϷ���
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
	
	//����OpecodeParser
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
*�Ƿ���Ҫ��Ӹ�����
*/
bool LGC_RowChainRedoChange::isNeedAddToTrsct()
{
	return true;
}

/*
*��ȡ����Id
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
*�Ƿ����Dml�����ı��
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
*��ȡobject_id
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
*��ȡ����ڱ��startColNo
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
*�����������ݣ� ����pDmlRowsOutput���
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
*����RowChainRedoChange
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
//��ʼһ���µ�����ʱ�����������͵�change
//��һ��dataδ֪
//�ڶ���data��log_opcode_52
//.......................................

//constructor and desctructor

/*
*���캯��
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
*��������
*/
LGC_BeginTChange::~LGC_BeginTChange()
{
	return;
}

//public virtual functions

/*
*��ʼ��
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
*�Ƿ���Ҫ��Ӹ�����
*/
bool LGC_BeginTChange::isNeedAddToTrsct()
{
	return (m_logOp52.seq != 0);
}

/*
*��ȡ����Id
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
*��ȡ����Ŀ�ʼscn
*/
BYTE8 LGC_BeginTChange::getTrsctBeginSCN() const
{
	return m_beginSCN;

}

//static member functions
/*
*����BeginTChange
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
*���캯��
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
*��������
*/
LGC_EndTChange::~LGC_EndTChange()
{
	return;
}


//public virtual functions

/*
*��ʼ��
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
*�Ƿ���Ҫ��ӵ�����
*/
bool LGC_EndTChange::isNeedAddToTrsct()
{
	return true;
}

/*
*��ȡ����Id
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
*��ȡ�����ύscn
*/
BYTE8 LGC_EndTChange::getCommitSCN() const
{
	return m_commitSCN;

}

/*
*�����Ƿ�ع�
*/
bool LGC_EndTChange::isTrsctRollback() const
{
	return (m_logOp54.flg&4);
}


//static member functions
/*
*����EndTChange
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
//������change��dataArray��ʮ�������ı���ʽ����
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
