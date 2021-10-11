#include "lgc_TOriginalQueue.h"
#include "lgc_Transaction.h"
#include "lgc_api.h"

static const unsigned int MAX_TNUM = 100000;

//..........................................
//Class TOriginalQueue
//����: �����߳̽�������������
//      �ŵ����������
//..........................................
LGC_TOriginalQueue LGC_TOriginalQueue::s_instanceArray[MAX_RACTHREADS];

//constructor and desctructor

/*
*���캯��
*/
LGC_TOriginalQueue::LGC_TOriginalQueue()
{
	m_count = 0;
	return;
}

/*
*��������
*/
LGC_TOriginalQueue::~LGC_TOriginalQueue()
{
	return;
}

	
//public member functions

/*
*��ȡ����ͷ������
* ������Ϊ��ʱ����NULL
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
*������ͷ�������߳�����
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
*�����������µ�����
* ������Ԫ������������ֵʱ��
* ��Ҫ�ȴ�����Ԫ�����½�������ֵ���£�
* ��������µ�Ԫ��
*/
void LGC_TOriginalQueue::push(LGC_Transaction* pTransaction)
{
	//����̫�����ȴ�����������
	//�������Ԫ��
	while(m_count > MAX_TNUM){
		sleep(1);
	}
	
	//�����Ԫ��
	m_mutex.Lock();
	m_trsctList.push_back(pTransaction);
	++m_count;
	m_mutex.Unlock();
	
	return;
}

//static member functions

/*
*��ȡthreadId��Ӧ��ʵ��
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
