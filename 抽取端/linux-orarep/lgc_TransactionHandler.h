#ifndef LGC_TRANSACTIONHANDLER_H
#define LGC_TRANSACTIONHANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lgc_Defines.h"
#include "lgc_Structure.h"

class LGC_Transaction;

class LGC_TransactionHandler
{
public:
	//constructor and desctructor
	LGC_TransactionHandler();
	~LGC_TransactionHandler();

public:
	//static member functions
	static int handle(LGC_Transaction* pTransction);
private:
	static int  addTrsctToQueue(LGC_Transaction*          pTransaction);
	static void delFromActiveTList(const LGC_Transaction* pTransaction);
};
#endif
