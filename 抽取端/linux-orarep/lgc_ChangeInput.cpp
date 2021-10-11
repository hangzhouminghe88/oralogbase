#include "lgc_ChangeInput.h"
#include "lgc_Change.h"
#include "lgc_RedoRecord.h"
/*
*�����ػ�ȡchange vector
*������Դ��RedoRecord
*/

//...................................................
//constructor and desctructor
//...................................................

LGC_ChangeInput::LGC_ChangeInput(LGC_RedoRecord* pRedoRecord)
{
	m_changeNo = 1;
	m_pRedoRecord = pRedoRecord;
	m_changeList.clear();

	return;
}

LGC_ChangeInput::~LGC_ChangeInput()
{
	list<LGC_Change*>::iterator it;
	LGC_Change* pChange = NULL;
	while(!m_changeList.empty()){
		pChange = m_changeList.front();
		
		if(pChange){
			LGC_Change::freeChange(pChange);
			pChange = NULL;
		}

		m_changeList.pop_front();
	}
	return;
}


//.....................................................
//public member functions
//.....................................................

/*
*��ȡ��һ��change vector
*��ȡ�����change vector,���ڲ������� 
*����ɾ�������ν������, ��Ϊ�ڲ�ά�ֵ����û�Ͽ�
*/
int LGC_ChangeInput::getNextChange(LGC_Change** ppChange)
{
	*ppChange = this->popFromChangeList();
	if(*ppChange == NULL){
		return 0;
	}else{
		return 1;
	}
}



//.......................................................
//private member functions
//.......................................................

/*
*��changeListͷ����Changeȡ�����������ظ�������
*���changeListΪ�գ��򷵻�NULL
*/

LGC_Change* 
LGC_ChangeInput::popFromChangeList()
{
	if(m_changeList.empty()){
		return NULL;
	}else{
		LGC_Change* pChange = m_changeList.front();
		m_changeList.pop_front();
		return pChange;
	}
}

//LGC_Change* 
//LGC_ChangeInput::popFromChangeList()
//{
//	if(m_changeList.empty()){
//		return NULL;
//	}else{
//		LGC_Change* pChange = m_changeList.front();
//		m_changeList.pop_front();
//		return pChange;
//	}
//}



/*
*���δ�RedoRecord��ȡ��change vector, Ȼ����װ��changeList
*
*/
bool
LGC_ChangeInput::generateChangeList()
{
	//check: �տ�ʼʱchangeList����Ϊ��
	if(!lgc_check( m_changeList.empty() )){
		lgc_errMsg("m_changeList is not empty\n");
		return false;
	}

	int changesReaded = 0;
	LGC_Change* pChange = NULL;


	while(1 == (changesReaded = this->readChangeFromRecord(&pChange)) ){
		if(pChange){
			this->addChangeToList(pChange);
			pChange = NULL;
			++m_changeNo;
		}else{
			lgc_errMsg("pChange is null\n");
			goto errOut;
		}
	}

	
	if(!lgc_check(changesReaded ==0 && pChange == NULL)){
		lgc_errMsg("readChangeFromRecord  failed \n");
		goto errOut;
	}

	//success
	return true;

errOut:
	if(pChange){
		LGC_Change::freeChange(pChange);
		pChange = NULL;
	}
	return false;
}

/*
*��RedoRecord�ж�ȡchange vector;
*��RedoRecord�ж�ȡchange vector�����ݣ�������Щ���ݴ���change vector��ʵ����
*��ͨ������������ظ�������;
*/
int 
LGC_ChangeInput::readChangeFromRecord(LGC_Change** ppChange)
{
	//check: *ppChange ָ����ڴ滹û���䣬����ΪNULL
	if(!lgc_check(*ppChange == NULL)){
		lgc_errMsg("*ppChange is not null \n");
		return -1;
	}
	
	int bytesReaded = 0;
	
	LGC_Change* pChange = NULL;
	log_changeVector_header changeHeader;
	memset(&changeHeader,0,sizeof(changeHeader));
	unsigned short lenOfChangeLenArray = 0;
	unsigned short* lenArray = NULL;
	unsigned short* lenArrayAlign = NULL;
	char** dataArray = NULL;
	
	//read changeHeader from record
	bytesReaded = this->readChangeHeaderFromRecord(&changeHeader,sizeof(changeHeader));
	if(!(bytesReaded == 0 || bytesReaded == sizeof(changeHeader))){
		lgc_errMsg("readChangeHeaderFromRecord failed \n");
		return -1;
	}

	if(bytesReaded == 0){//have no more change vector
		return 0;
	}
	
	//read changeVector's lenArray from Record 
	bytesReaded = this->readChangeLenArrayFromRecord(&lenArray, &lenOfChangeLenArray);
	if(!(bytesReaded > 0 && bytesReaded == lenOfChangeLenArray) ){
		lgc_errMsg("readChangeLenArrayFromRecord failed \n");
		goto errOut;
	}
	
	//�������ֽڶ����lenArray of change vector
	lenArrayAlign = LGC_ChangeInput::createAlignChangeLenArray(lenArray, lenOfChangeLenArray);
	if(!(lenArrayAlign && lenArrayAlign[0] == lenOfChangeLenArray)){
		lgc_errMsg("createAlignChangeLenArray failed \n");
		goto errOut;
	}
	
	//create dataArray of change vector
	dataArray = LGC_ChangeInput::createDataArray(lenArray,lenArrayAlign,lenOfChangeLenArray);
	if(dataArray == NULL){
		lgc_errMsg("createDataArray failed \n");
		goto errOut;
	}
	
	//read changeVector's dataArray from record
	bytesReaded = this->readDataArrayFromRecord(lenArray,lenArrayAlign,lenOfChangeLenArray,dataArray);
	if(!(bytesReaded >= 0)){
		lgc_errMsg("readDataArrayFromRecord \n");
		goto errOut;
	}
	
	//���ݴ�Record�ж�����changeHeader�� lenArray��dataArray����change vector
	pChange = LGC_Change::createChange(m_pRedoRecord, changeHeader, &lenArray, &lenArrayAlign,&dataArray,m_changeNo);
	if(pChange == NULL){
		lgc_errMsg("createchange failed \n");
		goto errOut;
	}

	//success
	*ppChange = pChange;
	pChange = NULL;
	return 1;

errOut:
	if(pChange){
		LGC_Change::freeChange(pChange);
		pChange = NULL;
	}

	if(dataArray){
		LGC_ChangeInput::freeDataArray(lenArray,lenArrayAlign,lenOfChangeLenArray, dataArray);
		dataArray = NULL;
	}

	if(lenArrayAlign){
		delete lenArrayAlign;
		lenArrayAlign = NULL;
	}

	if(lenArray){
		delete lenArray;
		lenArray = NULL;
	}
	
	return -1;
}

/*
*���change vector��changeList��β��
*
*/
void 
LGC_ChangeInput::addChangeToList(const LGC_Change* pChange)
{
	if(NULL == pChange){
		lgc_errMsg("pChange is null");
		exit(1);
	}

	m_changeList.push_back((LGC_Change*)pChange);
}

/*
*��redo record�ж�ȡchangeVector's header 
*����: changeHeaderLen
*���: changeHeader --��Ŷ�ȡ������
*/
int 
LGC_ChangeInput::readChangeHeaderFromRecord(void* changeHeader, const unsigned int changeHeaderLen)
{
	if(!lgc_check(changeHeaderLen == sizeof(log_changeVector_header))){
		lgc_errMsg("changeHeaderLen invalid \n");
		return -1;
	}

	return m_pRedoRecord->readChangeHeader(changeHeader,changeHeaderLen);
}

/*
*��redoRecord�ж�ȡchangeVector��s lenArray
*���: pLenOfChangeLenArray --ָ���λ�ü�¼��changeVector��s lenArray�ĳ���
*      pLenArray            --ָ���λ��ʱ��ȡ��changeVector's lenArray�����ݣ�
*                             ����ڴ����·����
*/
int 
LGC_ChangeInput::readChangeLenArrayFromRecord(unsigned short** pLenArray, unsigned short* pLenOfChangeLenArray)
{
	//check: pLenArray��pLenOfChangeLenArrayΪ�����������ʼֵΪ��
	if(!lgc_check(*pLenArray == NULL && *pLenOfChangeLenArray == 0)){
		lgc_errMsg("pLenArray��pLenOfChangeLenArrayΪ�����������ʼֵ��Ϊ�� \n");
		return -1;
	}

	//LenArray�������Ԫ�ص�����������unsigned short;
	//LenArray[0] ����LenArray����Ч���ݵĳ��ȣ�
	//����LenArray[0] >= 2 && LenArray[0] % 2 == 0;
	//LenArray��������Ԫ��ָʾdataArray��ÿ��data����Ч���ݵĳ���;
	//LenArray���ÿ��Ԫ�ذ���4�ֽڶ����ָʾ��dataArray��data����ʵ����
	//����: LenArray[0] == 6 ˵��LenArray��3������Ԫ�أ�LenArrayռ�õĿռ�ȴΪ8
	//      LenArray[1] == 11 ˵��dataArray[0]��Ч���ݵĳ���Ϊ1,����dataArray[0]ռ�õĿռ�Ϊ12

	int bytesReaded = 0;
	unsigned short lenOfChangeLenArray = 0;
	unsigned short lenOfChangeLenArrayAlign = 0;
	
	//tryRead length of changeVector's lenArray
	//���������lenArray����Ч���ݵĳ��ȣ�������lenArray���ڴ�����
	bytesReaded = m_pRedoRecord->tryReadLenOfChangeLenArray(&lenOfChangeLenArray,sizeof(unsigned short));
	if(!(bytesReaded > 0 && bytesReaded == sizeof(unsigned short) 
		&& lenOfChangeLenArray >= 2 && (lenOfChangeLenArray % sizeof(unsigned short)) == 0)){
		lgc_errMsg("tryReadLenOfChangeLenovoArray failed \n");
		return -1;
	}

	if(!lgc_check(lenOfChangeLenArray >= 2 && lenOfChangeLenArray%2 == 0)){
		lgc_errMsg("lenOfChangeLenArray invalid \n");
		return -1;
	}
	
	//lenArray����Ч���ݵĳ��ȣ�����4�ֽڶ����õ�lenArray����ʵ����(ռ���ڴ�Ĵ�С)
	lenOfChangeLenArrayAlign = LGC_ChangeInput::getAlignValue(lenOfChangeLenArray);
	if(!(lenOfChangeLenArrayAlign >= 4 && (lenOfChangeLenArrayAlign % 4) == 0)){
		lgc_errMsg("getAignValue failed \n");
		return -1;
	}
	
	//new lenArray
	unsigned short* lenArray = (unsigned short*)new char[lenOfChangeLenArrayAlign];
	if(lenArray == NULL){
		lgc_errMsg("new failed \n");
		goto errOut;
	}
	memset(lenArray,0,lenOfChangeLenArrayAlign);
	
	//read lenArray's data from record
	bytesReaded = m_pRedoRecord->readChangeLenArray(lenArray,lenOfChangeLenArrayAlign);
	if(!(bytesReaded > 0 && bytesReaded == lenOfChangeLenArrayAlign 
		&& lenArray[0] == lenOfChangeLenArray)){
		lgc_errMsg("readChangeLenArray failed \n");
		goto errOut;
	}

	//success
	*pLenArray = lenArray;
	lenArray = NULL;
	*pLenOfChangeLenArray = lenOfChangeLenArrayAlign;

	return lenOfChangeLenArrayAlign;

errOut:
	if(lenArray){
		delete[] lenArray;
		lenArray = NULL;
	}
	return -1;
}

/*
*��ȡdataArray������
*���: dataArray --���ڴ�Ŷ�ȡ��dataArray������
*/
int 
LGC_ChangeInput::readDataArrayFromRecord(const unsigned short* lenArray,const unsigned short* lenArrayAlign,
                                         const unsigned short lenOfChangeLenArray, char** dataArray)
{

	if(!lgc_check(lenArray[0] >= 2 && (lenArray[0] % 2) == 0
		  && lenArrayAlign[0] >= 4 && (lenArrayAlign[0] % 4) == 0 
		  && lenArrayAlign[0] == lenOfChangeLenArray))
	{
		lgc_errMsg("lenArray or lenArrayAlign invlid\n");
		return -1;
	}

	int bytesReaded = 0;
	unsigned short datas = lenArray[0]/2 - 1;
	unsigned int bytesTotalReaded = 0;
	for(int i=0; i<datas; i++){
		bytesReaded = m_pRedoRecord->readChangeData(dataArray[i], lenArrayAlign[i+1]);
		if(!(bytesReaded >= 0 && bytesReaded == lenArrayAlign[i+1] && (lenArrayAlign[i+1] % 4) == 0 )){
			lgc_errMsg("readChangedata failed \n");
			return -1;
		}
		bytesTotalReaded += bytesReaded;
	}

	//success
	return bytesTotalReaded;

}

/*
*����undo������ϵ;
*change vector���Ƿ���undo change vector��Ϊ: undo change vector �� redo change vector;
*����redo change vector��Щ������ĳ��undo change vector������һЩû���������κ�һ��undo change vector;
*�γ�undo������redo change vector �� undo change vector һ������ͬһ��redorecord��ģ�
*��redo change vector����һ��change vector��������Ӧ��undo change vector;
*/
bool
LGC_ChangeInput::generateUndoReferences()
{
	list<LGC_Change*>::reverse_iterator rit = m_changeList.rbegin();
	LGC_Change* pPrevChange = NULL;
	LGC_Change* pCurChange = NULL;
	
	
	pPrevChange = *rit;
	rit++;
	
	for(;rit != m_changeList.rend(); rit++){
		pCurChange = *rit;
		
		if(!lgc_check(pPrevChange != NULL 
			          && pCurChange != NULL))
		{
			lgc_errMsg("pPrevChange or pCurChange is NULL \n");
			return false;
		}
	
		if(pCurChange->isUndoChange()){
			pPrevChange->setUndoChange((LGC_UndoChange*)pCurChange);
		}
		
		pPrevChange = pCurChange;
	}

	//success 
	return true;
}

bool LGC_ChangeInput::initChangesInChangeList()
{
	int iFuncRet = 0;

	list<LGC_Change*>::iterator it;
	LGC_Change* pChange = NULL;

	for (it = m_changeList.begin();it != m_changeList.end(); it++){
		pChange = *it;

		if(pChange == NULL){
			lgc_errMsg("change is null");
			return false;
		}
		
		iFuncRet =pChange->init();
		if( iFuncRet < 0){
			lgc_errMsg("init change failed:%s \n", pChange->toString().data() );
			return false;
		}
	}
	
	//success
	return true;

}

//.......................................................
//static member functions
//.......................................................

/*
*����changeInput��ʵ��;
*��粻��ֱ��ͨ��new����������Ϊ���캯����˽�е�
*
*/
LGC_ChangeInput* 
LGC_ChangeInput::createChangeInput(LGC_RedoRecord* pRedoRecord)
{
	
	if(!lgc_check(pRedoRecord 
		          && pRedoRecord->recordBodyFinish() ))
	{
		lgc_errMsg("pRedoRecord��s data is not finish \n");
		return NULL;
	}
	
	//new an instance
	LGC_ChangeInput* pChangeInput = NULL;
	pChangeInput = new LGC_ChangeInput(pRedoRecord);
	if(pChangeInput == NULL){
		lgc_errMsg("new failed \n");
		return NULL;
	}
	
	//����changeList
	if(false == pChangeInput->generateChangeList() ){
		lgc_errMsg("generateChangeList failed ");
		goto errOut;
	}
	
	//����undo����
	if(false == pChangeInput->generateUndoReferences() ){
		lgc_errMsg("generateUndoReferences failed \n");
		goto errOut;
	}

	//��ʼ��changeList���change
	if(false == pChangeInput->initChangesInChangeList()){
		lgc_errMsg("initChangesInChangeList failed \n");
		goto errOut;
	}

	//success
	return pChangeInput;

errOut:
	if(pChangeInput){
		delete pChangeInput;
		pChangeInput = NULL;
	}
	return NULL;
}



/*
*����û��4���ֽڶ����lenArray������һ�����ֽڶ����lenArray
*/
unsigned short* 
LGC_ChangeInput::createAlignChangeLenArray(const unsigned short* lenArray, const unsigned short lenOfChangeLenArray)
{
	if(!lgc_check(lenArray[0] >= 2 && lenArray[0] % 2 == 0
		          && lenOfChangeLenArray >= 4 && (lenOfChangeLenArray % 4) == 0))
	{
		lgc_errMsg("lenArray or lenOfChangeLenArray invalid\n");
		exit(1);
	}
	
	//new 
	const unsigned short items = lenArray[0]/2;
	unsigned short* lenArrayAlign = (unsigned short*)new char[lenOfChangeLenArray];
	if(lenArrayAlign == NULL){
		lgc_errMsg("new failed \n");
		exit(1);
	}
	memset(lenArrayAlign,0,lenOfChangeLenArray);
	
	//����
	for(int i=0; i < items; i++){
		lenArrayAlign[i] = LGC_ChangeInput::getAlignValue(lenArray[i]);
		if(!( lenArrayAlign[i] >= lenArray[i] 
			  &&(lenArrayAlign[i] % 4) == 0))
		{
			lgc_errMsg("getAlignValue failed \n");
			exit(1);
		}
	}

	//success
	return lenArrayAlign;
}

/*
*����dataArray
*����: lenArray --��¼��dataArray��ÿ��data����Ч���ݳ���
*      lenArrayAlign --��¼��dataArray��ÿ��data����ʵ����
*/
char** 
LGC_ChangeInput::createDataArray(const unsigned short* lenArray, const unsigned short* lenArrayAlign, const unsigned short lenOfChangeLenArray)
{
	if(!lgc_check(lenArray[0] >= 2 && (lenArray[0] % 2) == 0
		          && lenArrayAlign[0] >= 4 && (lenArrayAlign[0] % 4) == 0 
		          && lenArrayAlign[0] == lenOfChangeLenArray))
	{
		lgc_errMsg("lenArray or lenArrayAlign invalid \n");
		exit(1);
	}
	
	//new dataArray
	unsigned short datas = lenArray[0]/2 - 1;
	char** dataArray = new char*[datas];
	if(dataArray == NULL){
		lgc_errMsg("new failed \n");
		exit(1);
	}
	memset(dataArray,0, sizeof(char*)*datas);
	
	//new dataArray's datas
	for(int i=0; i < datas; i++){

		dataArray[i] = new char[lenArrayAlign[i+1]];
		if(dataArray[i] == NULL){
			lgc_errMsg("new failed \n");
			exit(1);
		}
		memset(dataArray[i],0,lenArrayAlign[i+1]);
	}

	//success
	return dataArray;

}

/*
*�ͷ�dataArray
*
*/
void
LGC_ChangeInput::freeDataArray(const unsigned short* lenArray, const unsigned short* lenArrayAlign, const unsigned short lenOfChangeLenArray, char** dataArray)
{

	if(!lgc_check(lenArray[0] >= 2 && (lenArray[0] % 2) == 0
			     && lenArrayAlign[0] >= 4 && (lenArrayAlign[0] % 4) == 0 
		         && lenArrayAlign[0] == lenOfChangeLenArray))
	{
		lgc_errMsg("lenArray or  lenArrayAlign invalid, may have been free\n");
		exit(1);
	}

	unsigned short datas = lenArray[0]/2 - 1;
	
	//delete dataArray's datas
	for(int i=0; i < datas; i++){
		delete[] dataArray[i];
		dataArray[i] = NULL;
	}

	//delete dataArray
	delete[] dataArray;

	return;
}

/*
*����value��Ӧ�����ֽڶ����ֵ
*/
unsigned short 
LGC_ChangeInput::getAlignValue(const unsigned short value)
{
	unsigned short ret = value;
	if( (ret&0x03) != 0 ){
		ret = ((ret>>2)+1)<<2;
	}

	return ret;
}

