#include "lgc_RedoRecord.h"
#include "lgc_RedoFile.h"

/*
*redoRecord ����������ɣ�redoRecordHeader �� redoRecordBody;
*����RedoRecordBody�ɶ��change vector��ɣ�
*����redoRecord�ṩ��һЩ��recordBody�ж���change vector���ݵĽӿ�
*/

//.............................
//constructor and desctructor
//.............................

/*
*���캯��
*
*/
LGC_RedoRecord::LGC_RedoRecord(LGC_RedoFile* pRedoFile, 
                               const RBA&    rba, 
							   const void*   recordHeader, 
							   unsigned int  recordHeaderLen, 
							   const void*   recordBody, 
							   unsigned int  recordBodyLen)
{
	m_pRedoFile = pRedoFile;
	m_rba       = rba;

	
	if(recordHeaderLen > sizeof(m_recordHeader)){
		lgc_errMsg("recordHeaderLen failed \n");
		exit(1);
	}

	memset(&m_recordHeader, 0,            sizeof(m_recordHeader));
	memcpy(&m_recordHeader, recordHeader, recordHeaderLen);
	
	m_recordHeaderLen = recordHeaderLen;
	
	if(recordBodyLen == 0){
		m_recordBody      = NULL;
		m_recordBodyLen   = 0;
		m_recordBodyRdPos = m_recordBody;
	}else{

		m_recordBody = new char[recordBodyLen];
		if(m_recordBody == NULL){
			lgc_errMsg("new failed \n");
			exit(1);
		}
		memset(m_recordBody,0,recordBodyLen);
		memcpy(m_recordBody,recordBody,recordBodyLen);

		m_recordBodyLen   = recordBodyLen;
		m_recordBodyRdPos = m_recordBody;
	}
	m_dumpText = NULL;
	return;
}

LGC_RedoRecord::~LGC_RedoRecord()
{
	if(m_recordBody){
		delete[] m_recordBody;
		m_recordBody = NULL;
	}
	
	if(m_dumpText){
		delete[] m_dumpText;
		m_dumpText = NULL;
	}

	return;
}


//..................................
//public member functions
//..................................

/*
*redoRecord�����ݲ�����������ʣ�µ����ݣ��Թ���������redoRecord
*����: <0--ʧ��; >=0 --���ص����ݵĳ���
*/
int LGC_RedoRecord::loadLeftRedoRecordData(LGC_IncompleteRedoRecordData* pIncompleteRedoRecordData)
{
	unsigned int recordLen					= m_recordHeader.record_length;
	unsigned int prevRecordBodyLen			= m_recordBodyLen;
	unsigned int incompleteRecordBodyLen	= pIncompleteRedoRecordData->getDataLen();
	unsigned int recordHeaderLen			= m_recordHeaderLen;
	unsigned int completeRecordBodyLen		= recordLen - recordHeaderLen;

	//���recordLen�ĺϷ���
	if(!lgc_check(    recordLen >  (recordHeaderLen + prevRecordBodyLen)
		           && recordLen <= (recordHeaderLen + prevRecordBodyLen + incompleteRecordBodyLen)
		         ))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}
	
	//Ϊ������recordBody����ռ�
	char* pRecordBody = new char[completeRecordBodyLen];
	if(pRecordBody == NULL){
		lgc_errMsg("new failed \n");
		return -1;
	}
	memset(pRecordBody,0,completeRecordBodyLen);
	
	//��������recordBody��ǰ��������
	if(m_recordBodyLen > 0){
		memcpy(pRecordBody,m_recordBody,m_recordBodyLen);
	}

	//��������recordBody�ĺ󲿷�����
	memcpy(pRecordBody+m_recordBodyLen, pIncompleteRedoRecordData->getDataBuf(), (completeRecordBodyLen-m_recordBodyLen));

	if(m_recordBody){
		delete[] m_recordBody;
		m_recordBody = NULL;
	}
	
	//����RedoRecord������RedoFileΪ��һ��RedoFile
	m_pRedoFile			= pIncompleteRedoRecordData->getRedoFile();

	m_recordBody		= pRecordBody;
	pRecordBody			= NULL;
	m_recordBodyLen		= completeRecordBodyLen;
	m_recordBodyRdPos	= m_recordBody;

	return (completeRecordBodyLen-prevRecordBodyLen);
}

/*
*��redoBody�ж���change vector header������
*/
int LGC_RedoRecord::readChangeHeader(void* changeHeader, unsigned int size)
{
	if(m_recordBodyLen == (m_recordBodyRdPos-m_recordBody)){
		return 0;
	}

	return this->read(changeHeader,size);
}

/*
*��redoBody�ж���change vector�� dataLenArray�ĳ��ȵ�����, ���Ƕ�ȡָ�벻��
*/
int LGC_RedoRecord::tryReadLenOfChangeLenArray(void* lenOfChangeLenArray, unsigned int size)
{
	//check: size == sizeof(unsigned short)
	if(!(size == sizeof(unsigned short))){
		lgc_errMsg("check failed \n");
	}
	return this->tryRead(lenOfChangeLenArray,size);
}

/*
*��redoBody�ж���change vector��lenArray������
*/
int LGC_RedoRecord::readChangeLenArray(void* changeLenArray, unsigned int size)
{
	return this->read(changeLenArray,size);
}

/*
*��redoBody�ж���change vector��dataArray��ĳ��data������
*/
int LGC_RedoRecord::readChangeData(void* changeData, unsigned int size)
{	
	return this->read(changeData,size);
}


//........................................
//private member functions
//........................................

/*
*��recordBody�ж�������
*/
int LGC_RedoRecord::read(void* buf, unsigned int size)
{
	//check: m_recordBodyLen >= ((m_recordBodyRdPos-m_recordBody)+size)
	if(!(m_recordBodyLen >= ((m_recordBodyRdPos-m_recordBody)+size))){
		lgc_errMsg("check failed \n");
	}
	memcpy(buf,m_recordBodyRdPos,size);
	m_recordBodyRdPos += size;

	return size;
}

/*
*��recordBody�ж������� 
*���ǲ��ƶ���ȡλ��
*/
int LGC_RedoRecord::tryRead(void* buf, unsigned int size) const
{
	//check: m_recordBodyLen >= ((m_recordBodyRdPos-m_recordBody)+size)
	if(!(m_recordBodyLen >= ((m_recordBodyRdPos-m_recordBody)+size))){
		lgc_errMsg("check failed \n");
	}
	memcpy(buf,m_recordBodyRdPos,size);
	return size;
}

//............................................
//some get or set properties functions
//............................................

unsigned int LGC_RedoRecord::getThreadId() const
{
	return m_pRedoFile->getThreadId();
}
unsigned int LGC_RedoRecord::getRecordLen() const 
{
	return m_recordHeader.record_length;
}

unsigned int LGC_RedoRecord::getRecordHeaderLen() const 
{
	return m_recordHeaderLen;
}

unsigned int LGC_RedoRecord::getRecordBodyLen() const
{
	return m_recordBodyLen;
}

bool LGC_RedoRecord::recordBodyFinish() const
{
	//check this->getRecordLen() >= (this->getRecordHeaderLen() + this->getRecordBodyLen())
	if(!(this->getRecordLen() >= (this->getRecordHeaderLen() + this->getRecordBodyLen()))){
		lgc_errMsg("check failed \n");
		exit(1);
	}
	
	return (this->getRecordLen() == (this->getRecordHeaderLen() + this->getRecordBodyLen()));
}

BYTE8 LGC_RedoRecord::getRecordSCN() const
{
	BYTE8 scn = m_recordHeader.scn_major;
	scn <<= 32;
	scn += m_recordHeader.scn_minor;

	return scn;

}

char* LGC_RedoRecord::getClearedDumpTextBuf()
{
	if(m_dumpText == NULL){
		m_dumpText = new char[512];
		if(m_dumpText == NULL){
			lgc_errMsg("new failed \n");
			exit(1);
		}
	}

	memset(m_dumpText,0,512);
	return m_dumpText;
}

//...............................................
//functions that convert properties to string 
//...............................................

const char* LGC_RedoRecord::recordHeaderToString() 
{	
	char* dumpText = this->getClearedDumpTextBuf();
	sprintf(dumpText+strlen(dumpText), "REDO RECORD - Thread:%01u ", this->getThreadId());
	sprintf(dumpText+strlen(dumpText), "RBA: 0x%06x.%08x.%04x ", m_rba.sequence,m_rba.blockNo, m_rba.offsetInBlk);
	sprintf(dumpText+strlen(dumpText), "LEN: 0x%04x ", this->getRecordLen());
	sprintf(dumpText+strlen(dumpText), "VLD: 0x%02x", m_recordHeader.valid);
	
	return dumpText;
}

const char* LGC_RedoRecord::recordBodyToString() 
{
	char* dumpText = this->getClearedDumpTextBuf();

	return dumpText;
}



const char* LGC_RedoRecord::toString() 
{
	char* dumpText = this->getClearedDumpTextBuf();
	return dumpText;
}

//.....................................
//public static member functions
//.....................................

/*
*����RedoRecordʵ��
*����: pRedoFile --RedoRecord������RedoFile; 
*      (recordHeader,recordHeaderLen) --redorecord��heaher������
*      (recordBody,recordBodyLen) --redorecord��body�����ݣ��ⲿ�������п��ܲ�����
*����: ==NULL --����ʧ��; !=NULL --������RedoRecord
*/
LGC_RedoRecord* LGC_RedoRecord::createRedoRecord(LGC_RedoFile* pRedoFile, const RBA& rba, const void* recordHeader, unsigned int recordHeaderLen, 
                                                 const void* recordBody, unsigned int recordBodyLen)
{
	//check: pRedoFile && recordHeader && recordBody
	//       && (recordHeaderLen == sizeof(log_record_header_minor) || recordHeaderLen == sizeof(log_record_header_majar))
	if(!(pRedoFile && recordHeader && recordBody
		  && (recordHeaderLen == sizeof(log_record_header_minor) || recordHeaderLen == sizeof(log_record_header_major))))
	{
		lgc_errMsg("check failed \n");
		return NULL;
	}

	const unsigned int recordLen = ((log_record_header_minor*)recordHeader)->record_length;
    const bool recordBodyIsFinish = (recordLen == (recordHeaderLen + recordBodyLen));
	
	//check: recordLen >= (recordHeaderLen + recordBodyLen)
	if(!(recordLen >= (recordHeaderLen + recordBodyLen))){
		lgc_errMsg("check failed \n");
		return NULL;
	}

	LGC_RedoRecord* pRedoRecord = new LGC_RedoRecord(pRedoFile,rba,recordHeader,recordHeaderLen,recordBody,recordBodyLen);
	if(pRedoRecord == NULL){
		lgc_errMsg("new failed \n");
		return NULL;
	}

	return pRedoRecord;
}

