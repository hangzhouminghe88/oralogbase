#include "lgc_TConsumer.h"
#include "lgc_Transaction.h"
#include "lgc_MediaFileOutput.h"
#include "lgc_DmlRowParser.h"
#include "lgc_api.h"

//..............................................
//class LGC_TConsumer
//功能: 将事务消费掉，
//      主要是解析事务数据并输出
//..............................................

//constructor and desctructor

/*
*构造函数
*/
LGC_TConsumer::LGC_TConsumer(LGC_Transaction* pTrsct)
{
	m_pTrsct = pTrsct;
	return;
}

/*
*析构函数
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
*消费事务: 解析事务数据，并输出
*/
int LGC_TConsumer::consume()
{
	int bytesWrited = 0;

	LGC_MediaFileOutput*		pMediaFileOutput	=	LGC_MediaFileOutput::getInstance();
	const dml_trsct_begin*		pTrsctBegin			=	m_pTrsct->getTrsctBegin();
	const dml_trsct_end*		pTrsctEnd			=	m_pTrsct->getTrsctEnd();
	const list<LGC_DmlRow*>*	pDmlRowList			=	m_pTrsct->getDmlRowList();     
	
	//解析事务开始的数据
	if(this->parseTrsctBegin(pTrsctBegin, pMediaFileOutput) < 0){
		lgc_errMsg("parseTrsctBegin failed \n");
		return -1;
	}
	
	//解析事务中的dmlRowList
	if(this->parseTrsctDmlRowList(pDmlRowList, pMediaFileOutput) < 0){
		lgc_errMsg("parserTrsctDmlRowList failed \n");
		return -1;
	}

	//解析事务结束时的数据
	if(this->parseTrsctEnd(pTrsctEnd, pMediaFileOutput) < 0){
		lgc_errMsg("parseTrsctEnd failed \n");
		return -1;
	}

	//success
	return 0;
}


//private member functions

/*
*解析事务开始的数据，并输出
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
*解析事务中的dmlRowList, 并输出
*/
int LGC_TConsumer::parseTrsctDmlRowList(const list<LGC_DmlRow*> *pDmlRowList, LGC_MediaFileOutput* pMediaFileOutput)
{
	//获取DmlRowParser对象
	LGC_DmlRowParser* pDmlRowParser = LGC_DmlRowParser::getInstance();
	
	//遍历dmlRowList
	list<LGC_DmlRow*>::const_iterator it = pDmlRowList->begin();
	for(it = pDmlRowList->begin(); it != pDmlRowList->end(); it++){
		
		//解析dmlRow
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
*解析事务结束时的数据，并输出
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
*创建TConsumer
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
*释放TConsumer
*/
void LGC_TConsumer::freeTConsumer(LGC_TConsumer* pTConsumer)
{
	if(pTConsumer){
		delete pTConsumer;
		pTConsumer = NULL;
	}

	return;
}
