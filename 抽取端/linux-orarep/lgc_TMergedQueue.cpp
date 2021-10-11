#include "lgc_TMergedQueue.h"
#include "lgc_api.h"

static unsigned int MAX_TNUM = 10000000;

LGC_TMergedQueue* LGC_TMergedQueue::s_instance = NULL;
Mutex LGC_TMergedQueue::s_mutex;

//constructor and desctructor

LGC_TMergedQueue::LGC_TMergedQueue()
{
	m_count = 0;
	return;
}

LGC_TMergedQueue::~LGC_TMergedQueue()
{
	return;
}

//public member functions
LGC_Transaction* LGC_TMergedQueue::pop()
{
	if(m_trsctList.empty()){
		return NULL;
	}else{
		m_mutex.Lock();
		LGC_Transaction* pTrsct = m_trsctList.front();
		m_trsctList.pop_front();
		--m_count;
		m_mutex.Unlock();
		return pTrsct;
	}

	//never to here
	lgc_errMsg("never to here");
	exit(1);
}

void LGC_TMergedQueue::push(LGC_Transaction* pTransaction)
{
	//队列中元素太多，等待消费者消费一些
	//再添加新元素
	while(m_count > MAX_TNUM){
		sleep(1);
	}

	m_mutex.Lock();
	m_trsctList.push_back(pTransaction);
	++m_count;
	m_mutex.Unlock();


	return;
}

//static member functions
LGC_TMergedQueue* 
LGC_TMergedQueue::getInstance()
{
	if(s_instance == NULL){
		s_mutex.Lock();
		if(s_instance == NULL){
			s_instance = new LGC_TMergedQueue;
			if(s_instance == NULL){
				lgc_errMsg("new failed \n");
				exit(1);
			}
		}
		s_mutex.Unlock();
	}

	return s_instance;
}
