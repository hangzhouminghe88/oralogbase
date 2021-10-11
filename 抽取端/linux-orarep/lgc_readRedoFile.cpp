#include "lgc_readRedoFile.h"
#include "lgc_api.h"
#include "lgc_param.h"
#include "lgc_Structure.h"



static int loadArchFileInfo(void *_pArchiveFileInfo, const char* pVal, char* fieldNum)
{
	int i = 0;
	const char *archPath=NULL;
	const char *p = NULL;
	string fileNameOnly;
	
	if(pVal == NULL)
		return QEURY_END_DATA;
	LGC_RedoFileInfo* pArchiveFileInfo = (LGC_RedoFileInfo*)_pArchiveFileInfo;

	stringstream sstr(pVal);

	pArchiveFileInfo->fileType = FILE_ARCHIVE_TYPE;
	
	sstr >> pArchiveFileInfo->threadId;
	sstr >> pArchiveFileInfo->sequence;
	sstr >> pArchiveFileInfo->firstChange;
	sstr >> pArchiveFileInfo->nextChange;
	sstr >> pArchiveFileInfo->fileName;
	sstr >> pArchiveFileInfo->blkSize;
	sstr >> pArchiveFileInfo->totalBlks;

	archPath = ( LGC_Param::getInstance()->getSubstitutePathOfArch(pArchiveFileInfo->threadId) );
	if(archPath != NULL && strlen(archPath) != 0){//need substitute the archived log path
		p = strrchr(pArchiveFileInfo->fileName, DIR_SEPARATOR);
		if(p == NULL){
			lgc_errMsg("fileName invalied:%s\n", pArchiveFileInfo->fileName);
			exit(1);
		}
		p++;
		fileNameOnly = p;
		sprintf(pArchiveFileInfo->fileName, "%s%c%s", archPath, DIR_SEPARATOR, fileNameOnly.data());
	}

	
	return QEURY_END_DATA;
}

static int loadOnlineFileInfo(void* pOnlineFileInfo, const char* pVal, char* fieldNum)
{
	if(pVal == NULL)
		return QEURY_END_DATA;

	LGC_RedoFileInfo redoFileInfo;

	char status[126] = {0};
	char archived[126] = {0};

	stringstream sstr(pVal);

	sstr >> redoFileInfo.threadId;
	sstr >> redoFileInfo.sequence;
	sstr >> redoFileInfo.firstChange;
	sstr >> redoFileInfo.nextChange;
	sstr >> redoFileInfo.fileName;
	sstr >> redoFileInfo.blkSize;
	sstr >> redoFileInfo.totalBlks;

	sstr >> status;
	sstr >> archived;

	if(strcmp(archived,"YES") == 0){//the online log have archived
		return QEURY_END_DATA;
	}
	
	if(strcmp(status,"CURRENT") == 0){
		redoFileInfo.fileType = FILE_ONLINE_CURRENT_TYPE;
	}else if(strcmp(status,"ACTIVE") == 0){
		redoFileInfo.fileType = FILE_ONLINE_ACTIVE_TYPE;
	}else if(strcmp(status,"DEACTIVE") == 0){
		redoFileInfo.fileType = FILE_ONLINE_DEACTIVE_TYPE;
	}else{
		lgc_errMsg("online log status is invalid\n");
		return -1;
	}
	
	*(LGC_RedoFileInfo*)pOnlineFileInfo = redoFileInfo;
	return QEURY_END_DATA;

}

//////////////////////////////LGC_ReadRedoFile/////////////////////////

LGC_ReadRedoFile::LGC_ReadRedoFile(OciQuery* pQuery)
{
	m_pQuery = pQuery;
	m_pRWDataBlocks = NULL;
	m_errNo = ERR_OK;
	m_fileContent = NULL;
	m_validBlks = 0;
	m_pASMQuery = NULL;
	
	bool isASM = LGC_Param::getInstance()->isASM();
	
	if(isASM){
		const char* asmUser = LGC_Param::getInstance()->getASMUser();
		const char* asmPasswd = LGC_Param::getInstance()->getASMPasswd();
		const char* asmSid = LGC_Param::getInstance()->getASMSid();

		m_pASMQuery = new OciQuery;
		if(!m_pASMQuery){
			lgc_errMsg("new failed \n");
			exit(1);
		}
		m_pASMQuery->SetValses(asmSid,asmUser,asmPasswd);
	}
	return;
}

LGC_ReadRedoFile::~LGC_ReadRedoFile()
{
	if(m_pASMQuery){
		delete m_pASMQuery;
		m_pASMQuery = NULL;
	}

	if(m_pRWDataBlocks){
		delete m_pRWDataBlocks;
		m_pRWDataBlocks = NULL;
	}
	if(m_fileContent){
		delete[] m_fileContent;
		m_fileContent = NULL;
	}
	return;
}

bool LGC_ReadRedoFile::openRedoFile(const LGC_RedoFileInfo& redoFileInfo)
{
	bool bRet = true;
	RWInfoOfFile rwInfoOfFile;
	memset(&rwInfoOfFile,0,sizeof(rwInfoOfFile));
	char* blkBuf = NULL;
	
//	const char* asmUser = LGC_Param::getInstance()->getASMUser();
//	const char* asmPasswd = LGC_Param::getInstance()->getASMPasswd();
//	const char* asmSid = LGC_Param::getInstance()->getASMSid();


	
	//clear statistics
	m_validBlks = 0;

	m_errNo = ERR_OK;

	m_redoFileInfo = redoFileInfo;

	m_fileContent = new char[redoFileInfo.blkSize*(redoFileInfo.totalBlks+1)];
	if(!m_fileContent){
		lgc_errMsg("new failed\n");
		m_errNo = ERR_FATAL;
		bRet = false;
		goto errOut;
	}
	memset(m_fileContent,0,redoFileInfo.blkSize*(redoFileInfo.totalBlks+1));

	m_pRWDataBlocks = new RWDataBlocks;
	if(!m_pRWDataBlocks){
		lgc_errMsg("new failed \n");
		m_errNo = ERR_FATAL;
		bRet = false;
		goto errOut;
	}

	rwInfoOfFile.fileName = m_redoFileInfo.fileName; 
	rwInfoOfFile.blkSize =	m_redoFileInfo.blkSize;
	rwInfoOfFile.fileType = (enum TW_FILE_TYPE)m_redoFileInfo.fileType;
	rwInfoOfFile.pQuery = m_pASMQuery;
	bRet=m_pRWDataBlocks->setFileInfo(&rwInfoOfFile);
	if(!bRet){
		lgc_errMsg("m_pRWDataBlocks->setFileInfo(&rwInfoOfFile) failed\n");
		m_errNo = ERR_FATAL;
		goto errOut;
	}

	blkBuf = new char[redoFileInfo.blkSize];
	if(!blkBuf){
		lgc_errMsg("new failed\n");
		m_errNo = ERR_FATAL;
		bRet = false;
		goto errOut;
	}
	memset(blkBuf,0,redoFileInfo.blkSize);

	if( 1 != m_pRWDataBlocks->readBlks(0,blkBuf,1) ){
		lgc_errMsg(" m_pRWDataBlocks->readBlks(blkBuf,0,1)\n");
		m_errNo = ERR_FATAL;
		bRet = false;
		goto errOut;
	}
	memcpy(&m_logFileHeader,blkBuf,sizeof(m_logFileHeader));

	if( 1 != m_pRWDataBlocks->readBlks(1,blkBuf,1) ){
		lgc_errMsg(" m_pRWDataBlocks->readBlks(blkBuf,0,1)\n");
		m_errNo = ERR_FATAL;
		bRet = false;
		goto errOut;
	}
	memcpy(&m_logRedoHeader,blkBuf,sizeof(m_logRedoHeader));

	if(m_logRedoHeader.thread != redoFileInfo.threadId || redoFileInfo.sequence != m_logRedoHeader.block_head.log_file_sequence){
		if(m_logRedoHeader.thread == redoFileInfo.threadId &&  m_logRedoHeader.block_head.log_file_sequence > redoFileInfo.sequence){
			m_errNo = ERR_HEADER_SEQ_HIGHER;
		}else{
			m_errNo = ERR_FATAL;
		}
		bRet = false;
		goto errOut;
	}

//	if( (redoFileInfo.totalBlks+1) != this->readAllRedoBlk() ){
//		lgc_errMsg("readAllRedoBlk failed\n");
//		bRet = false;
//		goto errOut;
//	}

errOut:
	if(blkBuf){
		delete[] blkBuf;
		blkBuf = NULL;
	}

	return bRet;
}

int  LGC_ReadRedoFile::readBlks(char* buf, int blkOffOfRead, int blksToRead)
{
	return 0;
}



//protected
bool LGC_ReadRedoFile::readAllRedoBlk()
{
	bool bRet = true;
	const int blkSize = m_redoFileInfo.blkSize;
	const int totalBlks = m_redoFileInfo.totalBlks + 1;
	const DWORD sequence = m_redoFileInfo.sequence;
	log_block_header* pBlockHeader = NULL;
	int blksReaded = 0;

	const int auSize = 128*1024;
	const int blksInAu = auSize/blkSize;
	const int ausInFile = totalBlks/blksInAu;
	const int blksInLastAu = totalBlks%blksInAu;
	int ausReaded = 0;

	m_errNo = ERR_OK;
	blksReaded = 0;
	m_validBlks = 0;
	ausReaded = 0;

	for(ausReaded = 0; ausReaded < ausInFile; ausReaded++){
		if(blksInAu != m_pRWDataBlocks->readBlks(ausReaded*blksInAu,&m_fileContent[ausReaded*auSize],blksInAu)){
			lgc_errMsg("read redo blks failed \n");
			bRet = false;
			goto errOut;
		}
		for(blksReaded = ausReaded*blksInAu; blksReaded < (ausReaded+1)*blksInAu;blksReaded++){
			if(blksReaded == 0)
				continue;
			pBlockHeader = (log_block_header*)&m_fileContent[blkSize*blksReaded];
			if(pBlockHeader->block_id != blksReaded || pBlockHeader->log_file_sequence != sequence) {
				lgc_errMsg("block_id or sequence invalied \n");
				m_errNo = ERR_FATAL;
				bRet = false;
				goto errOut;
			}
		}
	}
	
	if(blksReaded != ausInFile*blksInAu){
		lgc_errMsg("blksReaded inValid \n");
		bRet = false;
		goto errOut;
	}

	for(blksReaded = (ausReaded)*blksInAu;blksReaded < (ausReaded)*blksInAu+blksInLastAu;blksReaded++){
		if( 1 != m_pRWDataBlocks->readBlks(blksReaded, &m_fileContent[blksReaded*blkSize],1) ){
			lgc_errMsg("reade read file failed\n");
			bRet = false;
			goto errOut;
		}
		
		if(blksReaded == 0)
			continue;

		pBlockHeader = (log_block_header*)&m_fileContent[blkSize*blksReaded];
		if(pBlockHeader->block_id != blksReaded || pBlockHeader->log_file_sequence != sequence) {
				lgc_errMsg("block_id or sequence invalied \n");
				m_errNo = ERR_FATAL;
				bRet = false;
				goto errOut;
		}
	}

	if(blksReaded != totalBlks){
		lgc_errMsg("blksReaded invalid \n");
		bRet = false;
		goto errOut;
	}

	m_validBlks = totalBlks;

errOut:
	return bRet;
}

//public static functions 
LGC_ReadRedoFile* LGC_ReadRedoFile::createReadRedoFile(const LGC_RedoFileInfo& redoFileInfo, OciQuery* pNormalQuery)
{
	bool bFuncRet = false;
	LGC_ReadRedoFile* pReadRedoFile = NULL;
	
	//new 
	switch(redoFileInfo.fileType){
		case FILE_ARCHIVE_TYPE:
			pReadRedoFile = new LGC_ReadArchiveFile(pNormalQuery);
			break;
		case FILE_ONLINE_DEACTIVE_TYPE:
		case FILE_ONLINE_ACTIVE_TYPE:
			pReadRedoFile = new LGC_ReadDeactiveOnlineFile(pNormalQuery);
			break;
		case FILE_ONLINE_CURRENT_TYPE:
			pReadRedoFile = new LGC_ReadCurrentOnlineFile(pNormalQuery);
			break;
		default:
			lgc_errMsg("redo file type is invalid \n");
			goto errOut;
	}

	if(pReadRedoFile == NULL){
		lgc_errMsg("new failed \n");
		goto errOut;
	}

	
	//open
	bFuncRet = pReadRedoFile->openRedoFile(redoFileInfo);
	if(!bFuncRet){
		goto errOut;
	}

	//success
	return pReadRedoFile;

errOut:
	if(pReadRedoFile){
		delete pReadRedoFile;
		pReadRedoFile = NULL;
	}

	return NULL;
}

/////////////////////////////LGC_ReadArchiveFile///////////////////////
LGC_ReadArchiveFile::LGC_ReadArchiveFile(OciQuery* pQuery):LGC_ReadRedoFile(pQuery)
{
	return;
}

LGC_ReadArchiveFile::~LGC_ReadArchiveFile()
{
	return;
}

bool LGC_ReadArchiveFile::openRedoFile(const LGC_RedoFileInfo& redoFileInfo)
{
	bool bRet = true;

	bRet =LGC_ReadRedoFile::openRedoFile(redoFileInfo);
	if(!bRet){
		lgc_errMsg("openRedoFile failed:fileName=%s\n",redoFileInfo.fileName);
		goto errOut;
	}

	bRet = LGC_ReadRedoFile::readAllRedoBlk();
	if(!bRet){
		lgc_errMsg("readAllRedoBlk failed\n");
		goto errOut;
	}

errOut:
	return bRet;
}

int  LGC_ReadArchiveFile::readBlks(char* buf, int blkOffOfRead, int _blksToRead)
{
	const int blkSize = m_redoFileInfo.blkSize;
	int blksToRead = (blkOffOfRead+_blksToRead) > m_validBlks? (m_validBlks-blkOffOfRead):_blksToRead;

	memcpy(buf,&m_fileContent[blkOffOfRead*blkSize], blksToRead*blkSize);

	return blksToRead;
}



///////////////////////////LGC_ReadDeactiveOnlineFile//////////////////
LGC_ReadDeactiveOnlineFile::LGC_ReadDeactiveOnlineFile(OciQuery* pQuery):LGC_ReadRedoFile(pQuery)
{
	m_pReadArchiveFile = NULL;
	return;
}

LGC_ReadDeactiveOnlineFile::~LGC_ReadDeactiveOnlineFile()
{
	if(m_pReadArchiveFile){
		delete m_pReadArchiveFile;
		m_pReadArchiveFile = NULL;
	}
	return;
}

bool LGC_ReadDeactiveOnlineFile::openRedoFile(const LGC_RedoFileInfo& redoFileInfo)
{
	bool bRet = true;
	m_errNo = ERR_OK;

	if(redoFileInfo.fileType == FILE_ARCHIVE_TYPE){
		if(m_pReadArchiveFile){
			lgc_errMsg("the Redo File have not be closed \n");
			bRet = false;
			goto errOut;
		}

		m_pReadArchiveFile = new LGC_ReadArchiveFile(m_pQuery);
		if(!m_pReadArchiveFile){
			lgc_errMsg("new failed\n");
			bRet = false;
			goto errOut;
		}
		//open archive file
		bRet = m_pReadArchiveFile->openRedoFile(redoFileInfo);
		if(!bRet){
			lgc_errMsg("openRedoFile failed\n");
			goto errOut;
		}
	
	}else if(redoFileInfo.fileType == FILE_ONLINE_DEACTIVE_TYPE || redoFileInfo.fileType == FILE_ONLINE_ACTIVE_TYPE){
			//open Redo File
			bRet = LGC_ReadRedoFile::openRedoFile(redoFileInfo);
			if(!bRet){
				if(m_errNo == ERR_HEADER_SEQ_HIGHER){
					bRet = handleSeqHigher();
					if(!bRet){
						lgc_errMsg("handleSeqHigher failed \n");
						goto errOut;
					}else{
						goto errOut;
					}
				}else{
					lgc_errMsg("openRedoFile failed \n");
					bRet = false;
					goto errOut;
				}
			}
			//read all redo blks
			bRet = this->readAllRedoBlk();
			if(!bRet){
				if(m_errNo == ERR_SEQ_HIGHER){
					bRet = handleSeqHigher();
					if(!bRet){
						lgc_errMsg("handleSeqHigher failed\n");
						goto errOut;
					}
				}else{
					lgc_errMsg("ReadAllRedoBlk failed\n");
					bRet = false;
					goto errOut;
				
				}

			}
	}else {
		lgc_errMsg("fileType invalid \n");
		bRet = false;
		goto errOut;
	}

errOut:
	return bRet;
}

int  LGC_ReadDeactiveOnlineFile::readBlks(char* buf, int blkOffOfRead, int _blksToRead)
{
	if(m_pReadArchiveFile){
		return m_pReadArchiveFile->readBlks(buf,blkOffOfRead, _blksToRead);
	}else{
		const int blkSize = m_redoFileInfo.blkSize;
		int blksToRead = (blkOffOfRead+_blksToRead) > m_validBlks? (m_validBlks-blkOffOfRead):_blksToRead;

		memcpy(buf,&m_fileContent[blkOffOfRead*blkSize], blksToRead*blkSize);

		return blksToRead;
	}
	return -1;
}


//protected
bool LGC_ReadDeactiveOnlineFile::readAllRedoBlk()
{
	bool bRet = true;
	const int blkSize = m_redoFileInfo.blkSize;
	const int totalBlks = m_redoFileInfo.totalBlks+1;
	const DWORD sequence = m_redoFileInfo.sequence;
	log_block_header* pBlockHeader = NULL;
	int blksReaded = 0;

	const int auSize = 128*1024;
	const int blksInAu = auSize/blkSize;
	const int ausInFile = totalBlks/blksInAu;
	const int blksInLastAu = totalBlks%blksInAu;
	int ausReaded = 0;

	m_errNo = ERR_OK;
	blksReaded = 0;
	m_validBlks = 0;
	ausReaded = 0;
	for(ausReaded = 0; ausReaded < ausInFile; ausReaded++){
		if(blksInAu != m_pRWDataBlocks->readBlks(ausReaded*blksInAu,&m_fileContent[ausReaded*auSize],blksInAu)){
			lgc_errMsg("read redo blks failed \n");
			bRet = false;
			goto errOut;
		}
		for(blksReaded = ausReaded*blksInAu; blksReaded < (ausReaded+1)*blksInAu;blksReaded++){
			if(blksReaded == 0)
				continue;
			pBlockHeader = (log_block_header*)&m_fileContent[blkSize*blksReaded];

			if(pBlockHeader->block_id != blksReaded){
				lgc_errMsg("block_id invalid \n");
				bRet = false;
				goto errOut;
			}

			if(pBlockHeader->log_file_sequence > sequence){
				m_errNo = ERR_SEQ_HIGHER;
				bRet = false;
				goto errOut;
			}else if(pBlockHeader->log_file_sequence < sequence){
				break;
			}
		}//end for
		if(pBlockHeader->log_file_sequence < sequence){
			break;
		}
	}//end for

	if(blksReaded < ausInFile*blksInAu){
		if( !(pBlockHeader->block_id == blksReaded && pBlockHeader->log_file_sequence < sequence) ){
			lgc_errMsg("block_id or sequence invalid\n");
			bRet = false;
			goto errOut;
		}
		m_validBlks = blksReaded;
		bRet = true;
		goto errOut;
		
	}
	
	for(blksReaded = (ausReaded)*blksInAu;blksReaded < (ausReaded)*blksInAu+blksInLastAu;blksReaded++){
		if( 1 != m_pRWDataBlocks->readBlks(blksReaded, &m_fileContent[blksReaded*blkSize],1) ){
			lgc_errMsg("reade read file failed\n");
			bRet = false;
			goto errOut;
		}
		
		if(blksReaded == 0)
			continue;

		pBlockHeader = (log_block_header*)&m_fileContent[blkSize*blksReaded];
		if(pBlockHeader->block_id != blksReaded){
			lgc_errMsg("block_id invalid \n");
			bRet = false;
			goto errOut;
		}
		if(pBlockHeader->log_file_sequence > sequence){
			m_errNo = ERR_SEQ_HIGHER;
			bRet = false;
			goto errOut;
		}else if(pBlockHeader->log_file_sequence < sequence){
			break;
		}
	}

	if(blksReaded < totalBlks){
		if( !(pBlockHeader->block_id == blksReaded && pBlockHeader->log_file_sequence < sequence) ){
			lgc_errMsg("block_id or sequence invalid\n");
			bRet = false;
			goto errOut;
		}
		m_validBlks = blksReaded;
		bRet = true;
		goto errOut;
		
	}else if(blksReaded == totalBlks){
		m_validBlks = blksReaded;
		bRet = true;
		goto errOut;
	}else{
		lgc_errMsg("block_id or sequence invalid \n");
		bRet = false;
		goto errOut;
	}

errOut:
	return bRet;
}

bool LGC_ReadDeactiveOnlineFile::handleSeqHigher()
{
	bool bRet = true;
	LGC_RedoFileInfo redoFileInfo;
	
	memset(&redoFileInfo,0,sizeof(redoFileInfo));
	bRet = m_pQuery->lgc_getArchFileInfo(&redoFileInfo, m_redoFileInfo.threadId, m_redoFileInfo.sequence, loadArchFileInfo);
	if(!bRet){
		lgc_errMsg("getArchFileInfo failed\n");
		goto errOut;
	}

	if(m_redoFileInfo.threadId != redoFileInfo.threadId || m_redoFileInfo.sequence != redoFileInfo.sequence 
				|| redoFileInfo.fileType != FILE_ARCHIVE_TYPE || strcmp(redoFileInfo.fileName, m_redoFileInfo.fileName) == 0){
		lgc_errMsg("the archive file info by selecting is invalid\n");
		bRet = false;
		goto errOut;
	}

	bRet = this->openRedoFile(redoFileInfo);
	if(!bRet){
		lgc_errMsg("openRefoFile invalid\n");
		goto errOut;
	}

errOut:
	return bRet;
}

/////////////////////////////LGC_ReadCurrentOnlineFile/////////////////
LGC_ReadCurrentOnlineFile::LGC_ReadCurrentOnlineFile(OciQuery* pQuery):LGC_ReadRedoFile(pQuery)
{
	m_pReadNotCurrentOnlineFile = NULL;
	m_validBlks = 0;

	return;
}

LGC_ReadCurrentOnlineFile::~LGC_ReadCurrentOnlineFile()
{
	if(m_pReadNotCurrentOnlineFile){
		delete m_pReadNotCurrentOnlineFile ;
		m_pReadNotCurrentOnlineFile = NULL;
	}
	
	return;
}
	
bool LGC_ReadCurrentOnlineFile::openRedoFile(const LGC_RedoFileInfo& redoFileInfo)
{
	bool bRet = true;

	if(redoFileInfo.fileType == FILE_ONLINE_CURRENT_TYPE){
		bRet = 	LGC_ReadRedoFile::openRedoFile(redoFileInfo);
		if(!bRet){
			if(m_errNo == ERR_HEADER_SEQ_HIGHER){
				bRet = this->handleSeqHigher();
				if(!bRet){
					lgc_errMsg("handleSeqHigher failed\n");
					bRet = false;
					goto errOut;
				}else{
					bRet = true;
					goto errOut;
				}
			}else{
				lgc_errMsg("openRedoFile failed\n");
				bRet = false;
				goto errOut;
			}
		}

		bRet = this->readAllRedoBlk();
		if(!bRet){
			if(m_errNo == ERR_SEQ_HIGHER ){
				bRet = this->handleSeqHigher();
				if(!bRet){
					lgc_errMsg("handleSeqHigher failed\n");
				}
				goto errOut;
			}else{
				lgc_errMsg("readAllRedoBlk failed\n");
				bRet = false;
				goto errOut;
			}
		}
	}else{
		if(m_pReadNotCurrentOnlineFile){
			lgc_errMsg("the file not be closed\n");
			bRet = false;
			goto errOut;
		}
		if(redoFileInfo.fileType == FILE_ARCHIVE_TYPE){
			m_pReadNotCurrentOnlineFile = new LGC_ReadArchiveFile(m_pQuery);
			if(!m_pReadNotCurrentOnlineFile){
				lgc_errMsg("new failed\n");
				bRet = false;
				goto errOut;
			}
		}else if(redoFileInfo.fileType == FILE_ONLINE_DEACTIVE_TYPE || redoFileInfo.fileType == FILE_ONLINE_ACTIVE_TYPE){
			m_pReadNotCurrentOnlineFile = new LGC_ReadDeactiveOnlineFile(m_pQuery);
			if(!m_pReadNotCurrentOnlineFile){
				lgc_errMsg("new failed\n");
				bRet = false;
				goto errOut;
			}
		}else{
			lgc_errMsg("fileType invalid \n");
			bRet = false;
			goto errOut;
		}

		bRet = m_pReadNotCurrentOnlineFile->openRedoFile(redoFileInfo);
		if(!bRet){
			lgc_errMsg("openRedoFile failed\n");
			bRet = false;
			goto errOut;
		}
	}
errOut:
	return bRet;
}

int  LGC_ReadCurrentOnlineFile::readBlks(char* buf, int blkOffOfRead, int _blksToRead)
{
	bool bFuncRet = 0;
	int iRet = 0;

	if(m_pReadNotCurrentOnlineFile){
		return m_pReadNotCurrentOnlineFile->readBlks(buf,blkOffOfRead,_blksToRead);
	}else{
		const int totalBlks = m_redoFileInfo.totalBlks+1;
		const int blkSize = m_redoFileInfo.blkSize;
		const int blksToRead = (blkOffOfRead+_blksToRead) > totalBlks? (totalBlks-blkOffOfRead):_blksToRead;

		if((blkOffOfRead + blksToRead) > m_validBlks){// need read new updated current online redo blocks
			sleep(2);
			if(this->isArchived()){
				bFuncRet = this->handleArchived();
				if(!bFuncRet){
					lgc_errMsg("handleArchived failed\n");
					iRet = -1;
					goto errOut;
				}
			}else{
				m_validBlks = 0;
				bFuncRet = this->readAllRedoBlk();
				if(!bFuncRet){
					if(m_errNo == ERR_SEQ_HIGHER){
						bFuncRet = handleSeqHigher();
						if(!bFuncRet){
							lgc_errMsg("handleSeqHigher failed\n");
							iRet = -1;
							goto errOut;
						}
					}else{
						lgc_errMsg("readAllRedoBlk faild\n");
						iRet = -1;
						goto errOut;
					}
				}
			}
			return this->readBlks(buf,blkOffOfRead, blksToRead);
		}

		memcpy(buf,&m_fileContent[blkOffOfRead*blkSize],blksToRead*blkSize);
		iRet = blksToRead;
	}

errOut:
	return iRet;
}


bool LGC_ReadCurrentOnlineFile::readAllRedoBlk()
{
	bool bRet = true;
	const int blkSize = m_redoFileInfo.blkSize;
	const int totalBlks = m_redoFileInfo.totalBlks+1;
	const DWORD sequence = m_redoFileInfo.sequence;
	log_block_header* pBlockHeader = NULL;
	int blksReaded = m_validBlks;

	const int auSize = 128*1024;
	const int blksInAu = auSize/blkSize;
	const int ausInFile = totalBlks/blksInAu;
	const int blksInLastAu = totalBlks%blksInAu;
	int ausReaded = m_validBlks/blksInAu;

	
	m_errNo = ERR_OK;
	blksReaded = m_validBlks;
	ausReaded = m_validBlks/blksInAu;

	for(ausReaded = m_validBlks/blksInAu; ausReaded < ausInFile; ausReaded++){
		if(blksInAu != m_pRWDataBlocks->readBlks(ausReaded*blksInAu,&m_fileContent[ausReaded*auSize],blksInAu)){
			lgc_errMsg("read redo blks failed \n");
			bRet = false;
			goto errOut;
		}
		for(blksReaded = ausReaded*blksInAu; blksReaded < (ausReaded+1)*blksInAu;blksReaded++){
			if(blksReaded == 0)
				continue;
			pBlockHeader = (log_block_header*)&m_fileContent[blkSize*blksReaded];

			if(pBlockHeader->block_id != blksReaded){
				lgc_errMsg("block_id invalid: pBlockHeader->block_id=%u blksReaded=%u fileName=%s sequence=%u\n", pBlockHeader->block_id ,blksReaded,m_redoFileInfo.fileName,m_redoFileInfo.sequence);
				bRet = false;
				goto errOut;
			}

			if(pBlockHeader->log_file_sequence > sequence){
				m_errNo = ERR_SEQ_HIGHER;
				bRet = false;
				goto errOut;
			}else if(pBlockHeader->log_file_sequence < sequence){
				break;
			}
		}//end for
		if(pBlockHeader->log_file_sequence < sequence){
			break;
		}
	}//end for
	
	if(m_validBlks/blksInAu < ausInFile){
		if(blksReaded < ausInFile*blksInAu){
			if( !(pBlockHeader->block_id == blksReaded && pBlockHeader->log_file_sequence < sequence) || blksReaded < m_validBlks){
				lgc_errMsg("block_id or sequence invalid\n");
				bRet = false;
				goto errOut;
			}
			m_validBlks = blksReaded;
			bRet = true;
			goto errOut;
		
		}
	}

	for(blksReaded = (ausReaded)*blksInAu;blksReaded < (ausReaded)*blksInAu+blksInLastAu;blksReaded++){
		if( 1 != m_pRWDataBlocks->readBlks(blksReaded, &m_fileContent[blksReaded*blkSize],1) ){
			lgc_errMsg("reade read file failed\n");
			bRet = false;
			goto errOut;
		}
		
		if(blksReaded == 0)
			continue;

		pBlockHeader = (log_block_header*)&m_fileContent[blkSize*blksReaded];
		if(pBlockHeader->block_id != blksReaded){
			lgc_errMsg("block_id invalid \n");
			bRet = false;
			goto errOut;
		}
		if(pBlockHeader->log_file_sequence > sequence){
			m_errNo = ERR_SEQ_HIGHER;
			bRet = false;
			goto errOut;
		}else if(pBlockHeader->log_file_sequence < sequence){
			break;
		}
	}


	if(blksReaded < m_validBlks){
		lgc_errMsg("blksReaded invalid \n");
		bRet = false;
		goto errOut;
	}

	if(blksReaded < totalBlks){
		if( !(pBlockHeader->block_id == blksReaded && pBlockHeader->log_file_sequence < sequence) ){
			lgc_errMsg("block_id or sequence invalid\n");
			bRet = false;
			goto errOut;
		}
		m_validBlks = blksReaded;
		bRet = true;
		goto errOut;
		
	}else if(blksReaded == totalBlks){
		m_validBlks = blksReaded;
		bRet = true;
		goto errOut;
	}else{
		lgc_errMsg("block_id or sequence invalid \n");
		bRet = false;
		goto errOut;
	}

errOut:
	return bRet;
}


/*
bool LGC_ReadCurrentOnlineFile::readAllRedoBlk()
{
	bool bRet = true;

	const int blkSize = m_redoFileInfo.blkSize;
	int totalBlks = m_redoFileInfo.totalBlks+1;
	int blksReaded = m_validBlks;
	const DWORD sequence = m_redoFileInfo.sequence;
	log_block_header* pBlockHeader = NULL;
	m_errNo = ERR_OK;
	for(blksReaded=m_validBlks; blksReaded < totalBlks; blksReaded++){
		if(1 != m_pRWDataBlocks->readBlks(blksReaded,&m_fileContent[blkSize*blksReaded],1)){
			lgc_errMsg("read blks failed\n");
			bRet = false;
			goto errOut;
		}
		if(blksReaded > 0){
			pBlockHeader = (log_block_header*)&m_fileContent[blkSize*blksReaded];
			if(pBlockHeader->block_id != blksReaded ){
				lgc_errMsg("block_id invalid\n");
				bRet = false;
				goto errOut;
			}
			if(pBlockHeader->log_file_sequence > sequence){
				m_errNo = ERR_SEQ_HIGHER;
				bRet = false;
				goto errOut;
			}else if(pBlockHeader->log_file_sequence < sequence){
				bRet = true;
				break;
			}
		}	
	}

	//update statistics
	m_validBlks = blksReaded;

errOut:
	return bRet;
}
*/


bool LGC_ReadCurrentOnlineFile::handleSeqHigher()
{
	bool bRet = true;
	LGC_RedoFileInfo redoFileInfo;
	memset(&redoFileInfo,0,sizeof(redoFileInfo));
	
	bRet = m_pQuery->lgc_getArchFileInfo(&redoFileInfo,m_redoFileInfo.threadId, m_redoFileInfo.sequence, loadArchFileInfo);
	if(!bRet){
		lgc_errMsg("loadArchiveFileInfo failed\n");
		goto errOut;
	}
	if(redoFileInfo.threadId != m_redoFileInfo.threadId || redoFileInfo.sequence != m_redoFileInfo.sequence ){
		bRet = m_pQuery->lgc_getOnlineFileInfo(&redoFileInfo, m_redoFileInfo.threadId, m_redoFileInfo.sequence, loadOnlineFileInfo);
		if(!bRet){
			lgc_errMsg("loadOnlineFileInfo failed\n");
			goto errOut;
		}
	}

	if(redoFileInfo.threadId != m_redoFileInfo.threadId || redoFileInfo.sequence != m_redoFileInfo.sequence || redoFileInfo.fileType == FILE_ONLINE_CURRENT_TYPE
				|| strcmp(redoFileInfo.fileName, m_redoFileInfo.fileName) == 0){
		lgc_errMsg("redofileinfo by selecting is invalid \n");
		bRet = false;
		goto errOut;
	}

	bRet = this->openRedoFile(redoFileInfo);
	if(!bRet){
		lgc_errMsg("openRedoFile failed\n");
		goto errOut;
	}

errOut:
	return bRet;
}

bool LGC_ReadCurrentOnlineFile::isArchived()
{
	bool bRet = true;
	bool isArchived = false;

	int i =0;
	for(i=0;i<100;i++){
		bRet = m_pQuery->lgc_isArchivedOfRedoFile(&isArchived, m_redoFileInfo.threadId, m_redoFileInfo.sequence);
		if(!bRet){
			lgc_errMsg("select isArchivedOfRedoFile failed \n");
			continue;
			exit(1);
		}

		bRet = isArchived;
		break;
	}

	if(i == 100){
		lgc_errMsg("select isArchivedOfRedoFile failed \n");
		exit(1);
	}

errOut:
	return bRet;
}

bool LGC_ReadCurrentOnlineFile::handleArchived()
{
	return this->handleSeqHigher();
}
