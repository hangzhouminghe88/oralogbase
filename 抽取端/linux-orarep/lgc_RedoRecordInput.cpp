#include "lgc_RedoRecordInput.h"
#include "lgc_RedoFile.h"
#include "lgc_RedoRecord.h"

/*
*������ȡ��RedoRecord
*
*/

//...............................
//constructor and desctructor
//...............................

LGC_RedoRecordInput::LGC_RedoRecordInput(LGC_RedoFile* pRedoFile)
{
	m_pRedoFile				= pRedoFile;
	m_pFirstRedoRecord		= NULL;
	m_pLastIncompRedoRecord = NULL;
}

LGC_RedoRecordInput::~LGC_RedoRecordInput()
{
	if(m_pFirstRedoRecord != NULL ){
		delete m_pFirstRedoRecord;
		m_pFirstRedoRecord = NULL;
	}

	if( m_pLastIncompRedoRecord != NULL ){
		delete m_pLastIncompRedoRecord;
		m_pLastIncompRedoRecord = NULL;
	}

	return;
}

//...............................
//public member functions
//...............................

/*
*����:		������ȡ��RedoRecord
*���:		ȡ����RecordRecord��ֵ��ppRedoRecord(���ڲ�new��)
*����ֵ:	< 0 -- ʧ�� 0--û���� 1--�ɹ�
*/
int LGC_RedoRecordInput::getNextRedoRecord(LGC_RedoRecord** ppRedoRecord)
{
	
	if(m_pFirstRedoRecord){//��һ��RedoRecord��û������
		
		//������һ��RedoRecord
		*ppRedoRecord		= m_pFirstRedoRecord;
		m_pFirstRedoRecord	= NULL;
		return 1;
	}else{//��һ��RedoRecord�Ѿ�������

		//��RedoFile�ж���RedoRecord
		return this->readNextRedoRecordFromFile(ppRedoRecord);
	}
}

//............................
//private member functions
//............................

/*
*����:		��redoFile�ж�ȡ��RecordRecord
*ǰ������:	��ȡָ���Ѿ�ָ����RedoRecord����ʼλ��
*���:		������RedoRecord��ֵ��ppRedoRecord (���ڲ�new��)
*����ֵ:	<0--ʧ�� 0--û���� 1--�ɹ�
*/
int LGC_RedoRecordInput::readNextRedoRecordFromFile(LGC_RedoRecord** ppRedoRecord)
{
	int iRet		= 0;
	int bytesReaded = 0;

	log_record_header_major		logRecordHeader;
	unsigned int				logRecordHeaderLen	= sizeof(logRecordHeader);
	char*						recordBody			= NULL;
	unsigned int				recordBodyLen		= 0;
	LGC_RedoRecord*				pRedoRecord			= NULL;
	
	//��ȡ��ǰ�Ķ�ȡָ���λ��
	RBA rba = m_pRedoFile->getCurRBA();

	//��redofile��ȡ��redoRecordHeader
	bytesReaded = this->readRedoRecordHeaderFromFile(&logRecordHeader,&logRecordHeaderLen);
	if(!(bytesReaded == 0 
		  || (bytesReaded > 0 && bytesReaded == logRecordHeaderLen)))
	{//Ҫôʲô��û���� Ҫô�������� �����쳣

		lgc_errMsg("readRedoRecordHeaderFromFile failed \n");
		goto errOut;
	}

	if(bytesReaded == 0){//Record�Ѿ���������
		iRet = 0;
		goto successOut;
	}
	
	//recordHeader�ĳ���ֻ��Ϊsizeof(log_record_header_minor) 
	//����sizeof(log_record_header_major)
	if(!lgc_check(	   logRecordHeaderLen == sizeof(log_record_header_minor) 
					|| logRecordHeaderLen == sizeof(log_record_header_major) ))
	{
		lgc_errMsg("check failed \n");
		goto errOut;
	}

	//��redofile�ж���recordBody������
	bytesReaded = this->readRedoRecordBody((const log_record_header_minor*)&logRecordHeader,logRecordHeaderLen,&recordBody,&recordBodyLen);
	if(!(   (bytesReaded == 0 && bytesReaded == recordBodyLen && recordBody == NULL ) 
		 || (bytesReaded >  0 && bytesReaded == recordBodyLen && recordBody != NULL ) ))
	{//û�ж������ݱ���û�����ݷ���
	 //�������ݱ��������ݷ���

		lgc_errMsg("readRedoRecordBody failed \n");
		goto errOut;
	}

	
	//����recordheader��recordbody������RedoRecord
	pRedoRecord = LGC_RedoRecord::createRedoRecord(m_pRedoFile, rba,&logRecordHeader, logRecordHeaderLen, recordBody, recordBodyLen);
	if(!(pRedoRecord != NULL)){

		lgc_errMsg("createRedoRecord failed \n");
		goto errOut;
	}
	
	//���record�����ݻ�û�б����꣬�������������
	//�Ա����һ��RedoFile��ȡʣ�µ�����
	if ( pRedoRecord->recordBodyFinish() == false){

		//��recordbody���ݲ�������redorecord��������
		//ʣ�µ����ݿ��Դ���һ��redofile��ȡ
		this->saveLastIncompleteRecord(pRedoRecord);
		pRedoRecord = NULL;
		iRet = 0;
		goto successOut;
	}
	
	//������һ��redoRecordλ�ã��Ա��´ζ�ȡ�µ�redorecord
	if ( this->skipToNextRecord() < 0){
		lgc_errMsg("skipToNextRecord failed \n");
		goto errOut;
	}


	//success 
	iRet = 1;
	*ppRedoRecord	= pRedoRecord;
	pRedoRecord		= NULL;
	goto successOut;

successOut:
    if(recordBody){
		delete[] recordBody;
		recordBody = NULL;
	}
	if(pRedoRecord){
		delete pRedoRecord;
		pRedoRecord = NULL;
	}
	return iRet;

errOut:
		
    if(recordBody){
		delete[] recordBody;
		recordBody = NULL;
	}
	if(pRedoRecord){
		delete pRedoRecord;
		pRedoRecord = NULL;
	}
	
	return -1;
}

/*
*����:		��redofile�ж�ȡ��recordHeader
*ǰ������:	��ȡָ���ѽ�λ��record����ʼλ��
*���룺		pRecordHeaderLen -- recordHeadBuf������С
*���:		recordHeaderBuf  -- ���recordheader������
*			pRecordHeaderLen -- recordHeader����ʵ��С
*����:		<0 -- ʧ�� 0--�ļ����� >0 --�ɹ�
*/
int LGC_RedoRecordInput::readRedoRecordHeaderFromFile(void* recordHeaderBuf, unsigned int* pRecordHeaderLen)
{
	int	bytesReaded	= 0;

	//recordHeaderBuf����ΪNULL
	//��*pRecordHeaderLen���ڵ���RecordHeader����С��С
	if(!lgc_check(   recordHeaderBuf 
		          && *pRecordHeaderLen >= sizeof(log_record_header_minor) ))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}
	
	//���Դ�redofile�ж�ȡ��minor recordheader, 
	//���Ƕ�ȡλ�ò����
	bytesReaded = m_pRedoFile->tryReadRecordHeader(recordHeaderBuf, sizeof(log_record_header_minor));
	if(!(  bytesReaded == 0 
		|| bytesReaded == sizeof(log_record_header_minor) ))
	{//Ҫôʲô��û������Ҫô��������� �����쳣

		lgc_errMsg("tryReadRecordHeader failed \n");
		return -1;
	}
	
	//���ʲô��û�������������
	if(bytesReaded == 0){
		return 0;
	}

	//����recordheaderlen
	const unsigned  char vld				= ((log_record_header_minor*)recordHeaderBuf)->valid;
	const unsigned  int	 recordHeaderLen	= LGC_RedoRecordInput::calRecordHeaderLen(vld);

	
	//�������record�ĳ��� Ӧ��С�ڵ��� record������С
	if(!(*pRecordHeaderLen >= recordHeaderLen)){
		lgc_errMsg("check failed \n");
		return -1;
	}
	
	//��redo file�ж�ȡ��recordheader
	bytesReaded = m_pRedoFile->readRecordHeader(recordHeaderBuf, recordHeaderLen);
	if(!(    bytesReaded == 0 
		  || bytesReaded == recordHeaderLen ))
	{//Ҫôʲô��û���� Ҫô��������� �����쳣

		lgc_errMsg("readRecordHeader failed \n");
		return 1;
	}

	//ʲô��û��������û��record��
	if(bytesReaded == 0){
		return 0;
	}

	//success
	*pRecordHeaderLen = recordHeaderLen;
	return *pRecordHeaderLen;
}

/*
*����:		��readFile�ж�ȡ��recordbody������
*���:		ppRecordBody   -- ָ��new���Ĵ���recordbody���ݵ�һ���ڴ�
*			pRecordBodyLen -- recordBody���������ݵĳ���
*����:		<0-- ʧ�� 0-- �ļ����� >0-- �ɹ�
*����˵��:	��������RecordBody�ĳ��ȣ�����recordBody
*/
int 
LGC_RedoRecordInput::readRedoRecordBody(const log_record_header_minor* pRecordHeader, int recordHeaderLen, char** ppRecordBody, unsigned int* pRecordBodyLen)
{
	int bytesReaded = 0;

	//pRecordHeader����Ϊ��
	//��recordHeader�ĳ���ֻ����sizeof(log_record_header_minor) ����sizeof(log_record_header_major) 
	if(!(    pRecordHeader 
		  && (    recordHeaderLen == sizeof(log_record_header_minor) 
		       || recordHeaderLen == sizeof(log_record_header_major) 
		     ) ))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}
	
	const unsigned int recordLen     = pRecordHeader->record_length;
	const unsigned int recordBodyLen = recordLen - recordHeaderLen;

	//record�ĳ��ȴ���recordHeader�ĳ���
	if(!(recordLen > recordHeaderLen)){
		lgc_errMsg("check failed \n");
		return -1;
	}
	
	//new recordBody
	char* pRecordBody = new char[recordBodyLen];
	if(pRecordBody == NULL){
		lgc_errMsg("new failed \n");
		return -1;
	}
	memset(pRecordBody,0,recordBodyLen);
	
	//��redoFile�ж�ȡRecordBody
	bytesReaded = m_pRedoFile->readRecordBody(pRecordBody,recordBodyLen);
	if( !(    bytesReaded >= 0 
		   && bytesReaded <= recordBodyLen) )
	{//��ȡ�ĳ��Ȳ���Խ�� �����쳣

		lgc_errMsg("readRecordBody failed \n");
		delete[] pRecordBody;
		pRecordBody = NULL;
		return -1;
	}

	//success
	*ppRecordBody = pRecordBody;
	pRecordBody =NULL;
	*pRecordBodyLen = recordBodyLen;
	if(*pRecordBodyLen == 0 && *ppRecordBody != NULL){
		delete[] *ppRecordBody;
		*ppRecordBody=NULL;
	}
	return *pRecordBodyLen;
}

/*
*����: ���������ݲ�������redorecord��������
*����: pRedoRecord -- Ҫ���ֵ�record
*/
void 
LGC_RedoRecordInput::saveLastIncompleteRecord(const LGC_RedoRecord* pRedoRecord)
{
	//Ҫ�����Record��Ϊ��
	//�����ݻ�û����
	if(!(   pRedoRecord != NULL 
		 && pRedoRecord->recordBodyFinish() == false))
	{
		lgc_errMsg("check failed \n");
		exit(1);
	}
	
	//����
	m_pLastIncompRedoRecord = (LGC_RedoRecord*)pRedoRecord;
	return;
}

/*
*����: ������һ��redorecord
*      Ҳ���Ƕ�ȡλ��������һ��Record����ʼλ��
*
*/
int  LGC_RedoRecordInput::skipToNextRecord()
{
	return m_pRedoFile->skipToNextRecordPos();
}


//.............................
//static member functions
//.............................

/*
*����: ����RedoRecordInput����
*����: pPrevRedoRecordInput -- ��һ��RedoFile��Ӧ��RedoRecordInput����
*      pRedoFile            -- ��ǰredoFile
*/
LGC_RedoRecordInput* 
LGC_RedoRecordInput::createRedoRecordInput(LGC_RedoRecordInput* pPrevRedoRecordInput, LGC_RedoFile* pRedoFile)
{
	int iFuncRet = 0;

	LGC_RedoRecordInput* pRedoRecordInput = NULL;
	LGC_RedoRecord* pPrevLastIncompRedoRecord = NULL;
	LGC_IncompleteRedoRecordData *pIncompleteRedoRecordData = NULL;

	//����RedoRecordInput
	pRedoRecordInput = new LGC_RedoRecordInput(pRedoFile);
	if(pRedoRecordInput == NULL){
		lgc_errMsg("new failed \n");
		return NULL;
	}
	
	//�����һ��redoRecordInput�б��������ݲ�������redorecord
	//����������redofile�м���ʣ�������
	if(pPrevRedoRecordInput != NULL
	        && pPrevRedoRecordInput->isLeftIncompleteRedoRecord() )
	{
		//�ӵ�ǰRedoFile�л�ȡ��һ��RedoRecordData����recorddataû��ͷֻ������
		//��Ҫ����һ��redoFile�����һ��RedoRecord�ϲ�
		pIncompleteRedoRecordData = pRedoFile->getFirstIncompleteRedoRecordData();
		if(pIncompleteRedoRecordData == NULL ){
			lgc_errMsg("pIncompleteRedoRecordData is NULL \n");
			return NULL;
		}

		//����һ��RedoFile��Ӧ��RedoRecordInputz�л�ȥ���һ��RedoRecord
		//��RedoRecord������û�ж���
		pPrevLastIncompRedoRecord = pPrevRedoRecordInput->getLastIncompleteRedoRecord();
		if(pPrevLastIncompRedoRecord == NULL ){
			lgc_errMsg("pPrevLastIncompRedoRecord is NULL \n");
			return NULL;
		}

		//�ϲ���һ��RedoFile�����һ��RedoRecord�͵�ǰRedoFile�ĵ�һ��RedoRecord
		iFuncRet = pPrevLastIncompRedoRecord->loadLeftRedoRecordData(pIncompleteRedoRecordData);
		if(iFuncRet < 0)
		{
			lgc_errMsg("loadLeftRedoRecordData failed \n");
			return NULL;
		}
		
		//���ϲ����RedoRecord����һ��RedoRecordInputת�Ƶ���ǰ��RedoRecordInput
		pPrevRedoRecordInput->detachLastIncompleteRedoRecord();
		pRedoRecordInput->setFirstRedoRecord(pPrevLastIncompRedoRecord);
		pPrevLastIncompRedoRecord = NULL;
	}
	
	//success
	return pRedoRecordInput;
}


/*
*����: ����recordHeader�ĳ���
*����: recordHeader�ĳ���
*/
unsigned int 
LGC_RedoRecordInput::calRecordHeaderLen(unsigned char vld)
{
	unsigned int recordHeaderLen = 0;

	switch( vld )
	{
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x0d:
		case 0x14://tw add 20150718
			recordHeaderLen = sizeof(log_record_header_major);
			break;
		case 0x01:
		case 0x02:
		case 0x09:
			recordHeaderLen = sizeof(log_record_header_minor);
			break;
		case 0x00:
		case 0x03:
			recordHeaderLen = sizeof(log_record_header_minor);
			break;
		default:
			recordHeaderLen = sizeof(log_record_header_minor);
			break;
	}

	return recordHeaderLen;
}