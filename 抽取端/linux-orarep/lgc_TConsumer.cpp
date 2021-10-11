#include "lgc_TConsumer.h"
#include "lgc_Transaction.h"
#include "lgc_MediaFileOutput.h"
#include "lgc_DmlRowParser.h"
#include "lgc_api.h"

//..............................................
//class LGC_TConsumer
//����: ���������ѵ���
//      ��Ҫ�ǽ����������ݲ����
//..............................................

//constructor and desctructor

/*
*���캯��
*/
LGC_TConsumer::LGC_TConsumer(LGC_Transaction* pTrsct)
{
	m_pTrsct = pTrsct;
	return;
}

/*
*��������
*/
LGC_TConsumer::~LGC_TConsumer()
{
	if(m_pTrsct){
		delete m_pTrsct;
		m_pTrsct = NULL;
	}

	return;
}

//public member functions

/*
*��������: �����������ݣ������
*/
int LGC_TConsumer::consume()
{
	int bytesWrited = 0;

	LGC_MediaFileOutput*		pMediaFileOutput	=	LGC_MediaFileOutput::getInstance();
	const dml_trsct_begin*		pTrsctBegin			=	m_pTrsct->getTrsctBegin();
	const dml_trsct_end*		pTrsctEnd			=	m_pTrsct->getTrsctEnd();
	const list<LGC_DmlRow*>*	pDmlRowList			=	m_pTrsct->getDmlRowList();     
	
	//��������ʼ������
	if(this->parseTrsctBegin(pTrsctBegin, pMediaFileOutput) < 0){
		lgc_errMsg("parseTrsctBegin failed \n");
		return -1;
	}
	
	//���������е�dmlRowList
	if(this->parseTrsctDmlRowList(pDmlRowList, pMediaFileOutput) < 0){
		lgc_errMsg("parserTrsctDmlRowList failed \n");
		return -1;
	}

	//�����������ʱ������
	if(this->parseTrsctEnd(pTrsctEnd, pMediaFileOutput) < 0){
		lgc_errMsg("parseTrsctEnd failed \n");
		return -1;
	}

	//success
	return 0;
}


//private member functions

/*
*��������ʼ�����ݣ������
*/
int LGC_TConsumer::parseTrsctBegin(const dml_trsct_begin* pTrsctBegin, LGC_MediaFileOutput* pMediaFileOutput)
{
	if (0 > pMediaFileOutput->writeLine("begin") ){
		lgc_errMsg("writeLine failed \n");
		return -1;
	}
	
	
	//success
	return 0;
}

/*
*���������е�dmlRowList, �����
*/
int LGC_TConsumer::parseTrsctDmlRowList(const list<LGC_DmlRow*> *pDmlRowList, LGC_MediaFileOutput* pMediaFileOutput)
{
	//��ȡDmlRowParser����
	LGC_DmlRowParser* pDmlRowParser = LGC_DmlRowParser::getInstance();
	
	//����dmlRowList
	list<LGC_DmlRow*>::const_iterator it = pDmlRowList->begin();
	for(it = pDmlRowList->begin(); it != pDmlRowList->end(); it++){
		
		//����dmlRow
		const LGC_DmlRow* pDmlRow = *it;
		if (0 > pDmlRowParser->parse(pDmlRow, pMediaFileOutput) ){
			lgc_errMsg("parse failed \n");
			return -1;
		}
	}
	
	//success
	return 0;
}

/*
*�����������ʱ�����ݣ������
*/
int LGC_TConsumer::parseTrsctEnd(const dml_trsct_end* pTrsctEnd, LGC_MediaFileOutput* pMediaFileOutput)
{
	if (0 > pMediaFileOutput->writeLine("commit;") ){
		lgc_errMsg("writeLine failed \n");
		return -1;
	}

	if (0 > pMediaFileOutput->writeLine("end;") ){
		lgc_errMsg("writeLine failed \n");
		return -1;
	}
	
	if (0 > pMediaFileOutput->writeLine("\n") ){
		lgc_errMsg("writeLine failed \n");
		return -1;
	}

	return 0;
}

//static member functions

/*
*����TConsumer
*/
LGC_TConsumer* LGC_TConsumer::createTConsumer(LGC_Transaction* pTrsct)
{
	LGC_TConsumer* pTConsumer = new LGC_TConsumer(pTrsct);
	if(pTConsumer == NULL){
		lgc_errMsg("new failed \n");
		return NULL;
	}

	return pTConsumer;
}

/*
*�ͷ�TConsumer
*/
void LGC_TConsumer::freeTConsumer(LGC_TConsumer* pTConsumer)
{
	if(pTConsumer){
		delete pTConsumer;
		pTConsumer = NULL;
	}

	return;
}
