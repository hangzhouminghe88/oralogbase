#include "lgc_RecordHandler.h"
#include "lgc_RedoRecord.h"
#include "lgc_ChangeInput.h"
#include "lgc_ChangeHandler.h"
#include "lgc_Change.h"

/*
*class LGC_RecordHandler
*����: ����RedoRecord
*      1.����RedoRecord����ChangeInput����
*      2.��ChangeInput�е����ض���Change
*      3.����ChangeHandler���������change
*/

//constructor and destructor 

/*
*���캯��
*/
LGC_RecordHandler::LGC_RecordHandler(LGC_RedoRecord* pRedoRecord)
{
	m_pRedoRecord = pRedoRecord;
	return;
}

/*
*��������
*/
LGC_RecordHandler::~LGC_RecordHandler()
{
	return;
}


//public member functions

/*
*����RedoRecord
*/
int LGC_RecordHandler::handle()
{
	int iFuncRet      = 0;
	int recordsReaded = 0;

	LGC_ChangeInput*	pChangeInput	= NULL;
	LGC_Change*			pChange			= NULL;
	LGC_ChangeHandler*	pChangeHandler	= NULL;
	
	//����ChangeInput
	pChangeInput = LGC_ChangeInput::createChangeInput(m_pRedoRecord);
	if(pChangeInput == NULL){
		lgc_errMsg("LGC_ChangeInput::createChangeInput failed \n");
		return -1;
	}
	
	//�����ش�ChangeInput�ж���Change
	while(1 == 
				(recordsReaded = pChangeInput->getNextChange(&pChange)))
	{
		if(pChange == NULL){
			lgc_errMsg("pChange is NULL \n");
			return -1;
		}
		
		//����ChangeHandler
		pChangeHandler = new LGC_ChangeHandler(pChange);
		pChange = NULL; //it will be deleted by pChangeInput's desctructor
		if(pChangeHandler == NULL){
			lgc_errMsg("new failed \n");
			return -1;
		}
		
		//��ChangeHandler����Change
		iFuncRet = pChangeHandler->handle();
		if(iFuncRet < 0){
			lgc_errMsg("handle failed \n");
			return -1;
		}
		
		//�ͷ�ChangeHanlder
		delete pChangeHandler;
		pChangeHandler = NULL;
		
	}

	if(recordsReaded != 0 || pChange != NULL ){
		lgc_errMsg("getNextChange failed \n");
		return -1;
	}

	//�ͷ�ChangeInput
	delete pChangeInput; //it will delete changes in it
	pChangeInput;

	return 0;
}

