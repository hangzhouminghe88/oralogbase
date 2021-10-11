#include "lgc_ChangeInput.h"
#include "lgc_Change.h"
#include "lgc_RedoRecord.h"
/*
*迭代地获取change vector
*数据来源是RedoRecord
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
*获取下一个change vector
*获取的这个change vector,在内部创建的 
*但是删除的责任交给外界, 因为内部维持的引用会断开
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
*将changeList头部的Change取出来，并返回个调用者
*如果changeList为空，则返回NULL
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
*依次从RedoRecord中取出change vector, 然后组装成changeList
*
*/
bool
LGC_ChangeInput::generateChangeList()
{
	//check: 刚开始时changeList必须为空
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
*从RedoRecord中读取change vector;
*从RedoRecord中读取change vector的数据，根据这些数据创建change vector的实例，
*并通过输出参数返回给调用者;
*/
int 
LGC_ChangeInput::readChangeFromRecord(LGC_Change** ppChange)
{
	//check: *ppChange 指向的内存还没分配，所以为NULL
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
	
	//创建四字节对齐的lenArray of change vector
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
	
	//根据从Record中读出的changeHeader、 lenArray、dataArray创建change vector
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
*添加change vector到changeList的尾部
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
*从redo record中读取changeVector's header 
*输入: changeHeaderLen
*输出: changeHeader --存放读取的数据
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
*从redoRecord中读取changeVector’s lenArray
*输出: pLenOfChangeLenArray --指向的位置记录了changeVector‘s lenArray的长度
*      pLenArray            --指向的位置时读取的changeVector's lenArray的数据，
*                             这块内存是新分配的
*/
int 
LGC_ChangeInput::readChangeLenArrayFromRecord(unsigned short** pLenArray, unsigned short* pLenOfChangeLenArray)
{
	//check: pLenArray和pLenOfChangeLenArray为输出参数，初始值为空
	if(!lgc_check(*pLenArray == NULL && *pLenOfChangeLenArray == 0)){
		lgc_errMsg("pLenArray和pLenOfChangeLenArray为输出参数，初始值不为空 \n");
		return -1;
	}

	//LenArray里的数据元素的数据类型是unsigned short;
	//LenArray[0] 代表LenArray的有效数据的长度，
	//所以LenArray[0] >= 2 && LenArray[0] % 2 == 0;
	//LenArray接下来的元素指示dataArray里每个data的有效数据的长度;
	//LenArray里的每个元素按照4字节对齐后，指示的dataArray里data的真实长度
	//例如: LenArray[0] == 6 说明LenArray有3个数据元素，LenArray占用的空间却为8
	//      LenArray[1] == 11 说明dataArray[0]有效数据的长度为1,但是dataArray[0]占用的空间为12

	int bytesReaded = 0;
	unsigned short lenOfChangeLenArray = 0;
	unsigned short lenOfChangeLenArrayAlign = 0;
	
	//tryRead length of changeVector's lenArray
	//这个长度是lenArray的有效数据的长度，并不是lenArray的内存容量
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
	
	//lenArray的有效数据的长度，经过4字节对齐后得到lenArray的真实长度(占用内存的大小)
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
*读取dataArray的数据
*输出: dataArray --用于存放读取的dataArray的数据
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
*生成undo依赖关系;
*change vector按是否是undo change vector分为: undo change vector 和 redo change vector;
*其中redo change vector有些依赖于某个undo change vector，另外一些没有依赖于任何一个undo change vector;
*形成undo依赖的redo change vector 和 undo change vector 一定是在同一个redorecord里的，
*且redo change vector的上一个change vector就是他对应的undo change vector;
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
*创建changeInput的实例;
*外界不能直接通过new来创建，因为构造函数是私有的
*
*/
LGC_ChangeInput* 
LGC_ChangeInput::createChangeInput(LGC_RedoRecord* pRedoRecord)
{
	
	if(!lgc_check(pRedoRecord 
		          && pRedoRecord->recordBodyFinish() ))
	{
		lgc_errMsg("pRedoRecord‘s data is not finish \n");
		return NULL;
	}
	
	//new an instance
	LGC_ChangeInput* pChangeInput = NULL;
	pChangeInput = new LGC_ChangeInput(pRedoRecord);
	if(pChangeInput == NULL){
		lgc_errMsg("new failed \n");
		return NULL;
	}
	
	//构建changeList
	if(false == pChangeInput->generateChangeList() ){
		lgc_errMsg("generateChangeList failed ");
		goto errOut;
	}
	
	//构建undo依赖
	if(false == pChangeInput->generateUndoReferences() ){
		lgc_errMsg("generateUndoReferences failed \n");
		goto errOut;
	}

	//初始化changeList里的change
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
*根据没有4四字节对齐的lenArray，创建一个四字节对齐的lenArray
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
	
	//对齐
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
*创建dataArray
*输入: lenArray --记录了dataArray中每个data的有效数据长度
*      lenArrayAlign --记录了dataArray中每个data的真实长度
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
*释放dataArray
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
*计算value对应的四字节对齐的值
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

