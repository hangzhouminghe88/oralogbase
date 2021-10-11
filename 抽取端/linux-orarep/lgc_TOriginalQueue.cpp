#include "lgc_TOriginalQueue.h"
#include "lgc_Transaction.h"
#include "lgc_api.h"

static const unsigned int MAX_TNUM = 100000;

//..........................................
//Class TOriginalQueue
//功能: 分析线程将分析出的事务
//      放到这个队列中
//..........................................
LGC_TOriginalQueue LGC_TOriginalQueue::s_instanceArray[MAX_RACTHREADS];

//constructor and desctructor

/*
*构造函数
*/
LGC_TOriginalQueue::LGC_TOriginalQueue()
{
	m_count = 0;
	return;
}

/*
*析构函数
*/
LGC_TOriginalQueue::~LGC_TOriginalQueue()
{
	return;
}

	
//public member functions

/*
*获取队列头的事务
* 当队列为空时返回NULL
*/
LGC_Transaction* LGC_TOriginalQueue::front()
{
	if(m_trsctList.empty()){
		return NULL;
	}else{
		return m_trsctList.front();
	}
}

/*
*将队列头的事务踢出队列
*/
void LGC_TOriginalQueue::pop_front()
{
	if(m_trsctList.empty()){
		//nothing to do 
	}else{
		m_mutex.Lock();
		m_trsctList.pop_front();
		--m_count;
		m_mutex.Unlock();
	}
	return;
}

/*
*向队列中添加新的事务
* 当队列元素数超过上限值时，
* 需要等待队列元素数下降到上限值以下，
* 才能添加新的元素
*/
void LGC_TOriginalQueue::push(LGC_Transaction* pTransaction)
{
	//队列太长，等待消费者消费
	//再添加新元素
	while(m_count > MAX_TNUM){
		sleep(1);
	}
	
	//添加新元素
	m_mutex.Lock();
	m_trsctList.push_back(pTransaction);
	++m_count;
	m_mutex.Unlock();
	
	return;
}

//static member functions

/*
*获取threadId对应的实例
*/
LGC_TOriginalQueue* 
LGC_TOriginalQueue::getInstance(unsigned short threadId)
{
	if(!lgc_check(threadId >= 1 
				  && threadId <= MAX_RACTHREADS))
	{
		lgc_errMsg("threadId invalid \n");
		exit(1);
	}

	return &s_instanceArray[threadId-1];
}
