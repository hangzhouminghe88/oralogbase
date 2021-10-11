#ifndef LGC_TMERGEDQUEUE_H
#define LGC_TMERGEDQUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>
#include <list>
using namespace std;

#include "lgc_Defines.h"
#include "lgc_Structure.h"
#include "Mutex.h"

class LGC_Transaction;

class LGC_TMergedQueue
{
private:
	//member variables
	unsigned int m_count;
	list<LGC_Transaction*> m_trsctList;
	Mutex m_mutex;

	static LGC_TMergedQueue* s_instance;
	static Mutex s_mutex;

private:
	//constructor and desctructor
	LGC_TMergedQueue();
	~LGC_TMergedQueue();

public:
	//public member functions
	LGC_Transaction* pop();
	void push(LGC_Transaction* pTransaction);

public:
	//static member functions
	static LGC_TMergedQueue* getInstance();
};
#endif
