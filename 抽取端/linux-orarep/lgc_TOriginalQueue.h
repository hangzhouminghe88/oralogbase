#ifndef LGC_TORIGINALQUEUE_H
#define LGC_TORIGINALQUEUE_H

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

class LGC_TOriginalQueue
{
private:
	//member variables	
	list<LGC_Transaction*> m_trsctList;
	unsigned int m_count;
	Mutex m_mutex;

	//static member variables
	static LGC_TOriginalQueue s_instanceArray[MAX_RACTHREADS];

private:
	//constructor and desctructor
	LGC_TOriginalQueue();
	~LGC_TOriginalQueue();

public:
	//public member functions
	LGC_Transaction* front();
	void pop_front();
	void push(LGC_Transaction* pTransaction);

private:
	//some get or set properties functions
	inline unsigned int getMaxTrscts() const
	{
		return 100000;
	}

	inline unsigned int getTrsctNum() const
	{
		return m_count;
	}

public:
	//static member functions
	static LGC_TOriginalQueue* getInstance(unsigned short threadId);
};
#endif
