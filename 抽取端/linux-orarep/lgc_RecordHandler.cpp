#include "lgc_RecordHandler.h"
#include "lgc_RedoRecord.h"
#include "lgc_ChangeInput.h"
#include "lgc_ChangeHandler.h"
#include "lgc_Change.h"

/*
*class LGC_RecordHandler
*功能: 处理RedoRecord
*      1.根据RedoRecord创建ChangeInput对象
*      2.从ChangeInput中迭代地读出Change
*      3.创建ChangeHandler处理读出的change
*/

//constructor and destructor 

/*
*构造函数
*/
LGC_RecordHandler::LGC_RecordHandler(LGC_RedoRecord* pRedoRecord)
{
	m_pRedoRecord = pRedoRecord;
	return;
}

/*
*析构函数
*/
LGC_RecordHandler::~LGC_RecordHandler()
{
	return;
}


//public member functions

/*
*处理RedoRecord
*/
int LGC_RecordHandler::handle()
{
	int iFuncRet      = 0;
	int recordsReaded = 0;

	LGC_ChangeInput*	pChangeInput	= NULL;
	LGC_Change*			pChange			= NULL;
	LGC_ChangeHandler*	pChangeHandler	= NULL;
	
	//创建ChangeInput
	pChangeInput = LGC_ChangeInput::createChangeInput(m_pRedoRecord);
	if(pChangeInput == NULL){
		lgc_errMsg("LGC_ChangeInput::createChangeInput failed \n");
		return -1;
	}
	
	//迭代地从ChangeInput中读出Change
	while(1 == 
				(recordsReaded = pChangeInput->getNextChange(&pChange)))
	{
		if(pChange == NULL){
			lgc_errMsg("pChange is NULL \n");
			return -1;
		}
		
		//创建ChangeHandler
		pChangeHandler = new LGC_ChangeHandler(pChange);
		pChange = NULL; //it will be deleted by pChangeInput's desctructor
		if(pChangeHandler == NULL){
			lgc_errMsg("new failed \n");
			return -1;
		}
		
		//用ChangeHandler处理Change
		iFuncRet = pChangeHandler->handle();
		if(iFuncRet < 0){
			lgc_errMsg("handle failed \n");
			return -1;
		}
		
		//释放ChangeHanlder
		delete pChangeHandler;
		pChangeHandler = NULL;
		
	}

	if(recordsReaded != 0 || pChange != NULL ){
		lgc_errMsg("getNextChange failed \n");
		return -1;
	}

	//释放ChangeInput
	delete pChangeInput; //it will delete changes in it
	pChangeInput;

	return 0;
}

