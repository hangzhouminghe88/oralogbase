#ifndef LGC_TCONSUMER_H
#define LGC_TCONSUMER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <list>
using namespace std;

#include "lgc_Defines.h"
#include "lgc_Structure.h"

class LGC_Transaction;
class LGC_MediaFileOutput;
class LGC_DmlRow;


//.....................................
//class LGC_TConsumer
//.....................................

class LGC_TConsumer
{
private:
	//member variables
	LGC_Transaction* m_pTrsct;
private:
	//constructor and desctructor
	LGC_TConsumer(LGC_Transaction* pTrsct);
	~LGC_TConsumer();

public:
	//public member functions
	int consume();

private:
	//private member functions
	int parseTrsctBegin(const dml_trsct_begin* pTrsctBegin, LGC_MediaFileOutput* pMediaFileOutput);
	int parseTrsctDmlRowList(const list<LGC_DmlRow*> *pDmlRowList, LGC_MediaFileOutput* pMediaFileOutput);
	int parseTrsctEnd(const dml_trsct_end* pTrsctEnd, LGC_MediaFileOutput* pMediaFileOutput);
public:
	//static member functions
	static LGC_TConsumer* createTConsumer(LGC_Transaction* pTrsct);
	static void freeTConsumer(LGC_TConsumer* pTConsumer);
};

#endif


