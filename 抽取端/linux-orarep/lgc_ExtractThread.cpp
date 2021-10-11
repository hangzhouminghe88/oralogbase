
#include "OciQuery.h"
#include "RedologINI.h"

#include "lgc_RedoFile.h"
#include "lgc_RedoRecord.h"
#include "lgc_Change.h"
#include "lgc_Transaction.h"

#include "lgc_RedoFileInput.h"
#include "lgc_RedoRecordInput.h"
#include "lgc_ChangeInput.h"

#include "lgc_RecordHandler.h"
#include "lgc_ChangeHandler.h"

#include "lgc_TransactionMgr.h"

#include "lgc_param.h"
#include "lgc_tableMeta.h"
#include "lgc_readRedoFile.h"

#include "lgc_ExtractThread.h"



/*
*模块名: 分析线程
*分析事务日志得出事务结果
*/

void* extractThreadFunc(void* pExtractThreadArg)
{
	int iRet          = 0;
	int iFuncRet      = 0;
	int recordsReaded = 0;

	OciQuery*               pQuery                 = NULL;
	LGC_RedoFileInput*      pRedoFileInput         = NULL;
	LGC_RedoFile*           pRedoFile              = NULL;
	LGC_RedoRecordInput*    pRedoRecordInput       = NULL;
	LGC_RedoRecordInput*    pPrevRedoRecordInput   = NULL;
	LGC_RedoRecord*         pRedoRecord            = NULL;
	LGC_RecordHandler*      pRecordHandler         = NULL;
	
	LGC_ExtractThreadArg*   pThreadArg             = (LGC_ExtractThreadArg*)pExtractThreadArg;
	
	const char*          tnsname     = pThreadArg->tnsname;
	const char*          password    = pThreadArg->password;
	const char*          username    = pThreadArg->username;
	const BYTE8          startSCN    = pThreadArg->startSCN;
	const unsigned short threadId    = pThreadArg->threadId;
	const unsigned short threads     = pThreadArg->threads;
	
	pQuery = new OciQuery;
	if(pQuery == NULL ){
		lgc_errMsg("new failed \n");
		iRet = -1;
		goto errOut;
	}
	pQuery->SetValses(tnsname, username, password);
	
	//创建RedoFileInput
	pRedoFileInput = LGC_RedoFileInput::createRedoFileInput(threadId, startSCN, pQuery);
	if(pRedoFileInput == NULL){
		lgc_errMsg("createRedoFileInput failed \n");
		iRet = -1;
		goto errOut;
	}
	
	//迭代地从RedoFileInput中读取出RedoFile
	while(true){
		//获取下一个RedoFile
		iFuncRet = pRedoFileInput->getNextRedoFile(&pRedoFile);
		if(iFuncRet != 1 || pRedoFile == NULL){

			if(iFuncRet == 0 && pRedoFile == NULL){//没有redoFile了，
				                                   //等待新的RedoFile生成
				sleep(10);

			}else{

				lgc_errMsg("getNextRedoFile failed \n");
				iRet = -1;
				goto errOut;
			}
		}
		
		//创建RedoRecordInput,用于从RedoFile中读取出RedoRecord
		pRedoRecordInput = LGC_RedoRecordInput::createRedoRecordInput(pPrevRedoRecordInput, pRedoFile);
		if(pRedoRecordInput == NULL){
			lgc_errMsg("createRedoRecordInput failed \n");
			iRet = -1;
			goto errOut;
		}
		
		if(pPrevRedoRecordInput){
			delete pPrevRedoRecordInput;
			pPrevRedoRecordInput = NULL;
		}
		
		//迭代地从RedoRecordInput中读取出RedoRecord
		while( 1 == 
			    (recordsReaded = pRedoRecordInput->getNextRedoRecord(&pRedoRecord)) )
		{
			if(pRedoRecord == NULL){
				lgc_errMsg("pRedoRecord is NULL \n");
				iRet = -1;
				goto errOut;
			}
			
			//创建RecordHanlder
			pRecordHandler = new LGC_RecordHandler(pRedoRecord);
			if(pRecordHandler == NULL){
				lgc_errMsg("new RecordHandler failed \n");
				iRet = -1;
				goto errOut;
			}
			
			//用RecordHandler处理Record
			iFuncRet = pRecordHandler->handle();
			if(iFuncRet < 0){
				lgc_errMsg("handle RedoRecord failed \n");
				iRet = -1;
				goto errOut;
			}
			
			//释放已经处理好了的Record
			if(pRecordHandler){
				delete pRecordHandler;
				pRecordHandler = NULL;
			}

			if(pRedoRecord){
				delete pRedoRecord;
				pRedoRecord = NULL;
			}
		}//end while getNextRedoRecord

		if(recordsReaded != 0 || pRedoRecord != NULL){
			lgc_errMsg("getNextRedoRecord failed \n");
			iRet = -1;
			goto errOut;
		}
		
		//释放RedoFile 和 pRedoRecordInput
		pPrevRedoRecordInput = pRedoRecordInput;
		pRedoRecordInput = NULL;

		if(pRedoFile){
			delete pRedoFile;
			pRedoFile = NULL;
		}
	}//end while getNextRedoFile

errOut:
	lgc_errMsg("extract thread will exit\n");
	exit(1);
	return NULL;
}
