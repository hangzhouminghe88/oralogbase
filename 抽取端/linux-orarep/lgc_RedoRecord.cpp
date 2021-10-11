#include "lgc_RedoRecord.h"
#include "lgc_RedoFile.h"

/*
*redoRecord 由两部分组成：redoRecordHeader 和 redoRecordBody;
*其中RedoRecordBody由多个change vector组成，
*所以redoRecord提供了一些冲recordBody中读出change vector数据的接口
*/

//.............................
//constructor and desctructor
//.............................

/*
*构造函数
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
*redoRecord的数据不完整，加载剩下的数据，以构成完整的redoRecord
*返回: <0--失败; >=0 --加载的数据的长度
*/
int LGC_RedoRecord::loadLeftRedoRecordData(LGC_IncompleteRedoRecordData* pIncompleteRedoRecordData)
{
	unsigned int recordLen					= m_recordHeader.record_length;
	unsigned int prevRecordBodyLen			= m_recordBodyLen;
	unsigned int incompleteRecordBodyLen	= pIncompleteRedoRecordData->getDataLen();
	unsigned int recordHeaderLen			= m_recordHeaderLen;
	unsigned int completeRecordBodyLen		= recordLen - recordHeaderLen;

	//检查recordLen的合法性
	if(!lgc_check(    recordLen >  (recordHeaderLen + prevRecordBodyLen)
		           && recordLen <= (recordHeaderLen + prevRecordBodyLen + incompleteRecordBodyLen)
		         ))
	{
		lgc_errMsg("check failed \n");
		return -1;
	}
	
	//为完整的recordBody分配空间
	char* pRecordBody = new char[completeRecordBodyLen];
	if(pRecordBody == NULL){
		lgc_errMsg("new failed \n");
		return -1;
	}
	memset(pRecordBody,0,completeRecordBodyLen);
	
	//拷贝完整recordBody的前部分数据
	if(m_recordBodyLen > 0){
		memcpy(pRecordBody,m_recordBody,m_recordBodyLen);
	}

	//拷贝完整recordBody的后部分数据
	memcpy(pRecordBody+m_recordBodyLen, pIncompleteRedoRecordData->getDataBuf(), (completeRecordBodyLen-m_recordBodyLen));

	if(m_recordBody){
		delete[] m_recordBody;
		m_recordBody = NULL;
	}
	
	//更改RedoRecord所属的RedoFile为后一个RedoFile
	m_pRedoFile			= pIncompleteRedoRecordData->getRedoFile();

	m_recordBody		= pRecordBody;
	pRecordBody			= NULL;
	m_recordBodyLen		= completeRecordBodyLen;
	m_recordBodyRdPos	= m_recordBody;

	return (completeRecordBodyLen-prevRecordBodyLen);
}

/*
*从redoBody中读出change vector header的数据
*/
int LGC_RedoRecord::readChangeHeader(void* changeHeader, unsigned int size)
{
	if(m_recordBodyLen == (m_recordBodyRdPos-m_recordBody)){
		return 0;
	}

	return this->read(changeHeader,size);
}

/*
*从redoBody中读出change vector的 dataLenArray的长度的数据, 但是读取指针不变
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
*从redoBody中读出change vector的lenArray的数据
*/
int LGC_RedoRecord::readChangeLenArray(void* changeLenArray, unsigned int size)
{
	return this->read(changeLenArray,size);
}

/*
*从redoBody中读出change vector的dataArray的某个data的数据
*/
int LGC_RedoRecord::readChangeData(void* changeData, unsigned int size)
{	
	return this->read(changeData,size);
}


//........................................
//private member functions
//........................................

/*
*从recordBody中读出数据
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
*从recordBody中读出数据 
*但是不移动读取位置
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
*创建RedoRecord实例
*输入: pRedoFile --RedoRecord所属的RedoFile; 
*      (recordHeader,recordHeaderLen) --redorecord的heaher的数据
*      (recordBody,recordBodyLen) --redorecord的body的数据，这部分数据有可能不完整
*返回: ==NULL --创建失败; !=NULL --创建的RedoRecord
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

