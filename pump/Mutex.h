#ifndef _MUTEX_H_
#define _MUTEX_H_
///////////////////////////////////////////////////////////////////////////////
// Mutex.h
// Rohit Joshi
// It implements the mutex for unix/linx os
//////////////////////////////////////////////////////////////////////////////
#include <pthread.h>
#include <semaphore.h>

#define  _SEMAPHORE_SIZE 1

// Lock class
template <class T> class Lock 
{
	T& m_obj;  //member
public:
	// lock
	Lock(T& obj):m_obj(obj)
	{
		m_obj.Lock();
	}
	// unlock
	~Lock()
	{
		m_obj.Unlock();
	}
};
// mutex class for unix/linux
class Mutex
{
public:
	// default constructor
	Mutex()
	{
		pthread_mutex_init(&m_mMutex, NULL);
	}
	// constructor with recursive param
    Mutex(bool bRecursive)
	{
		if(bRecursive) {
			pthread_mutexattr_t attr;
			pthread_mutexattr_init(&attr);
			pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
			pthread_mutex_init(&m_mMutex, &attr);
			pthread_mutexattr_destroy(&attr);
		}else {
			pthread_mutex_init(&m_mMutex, NULL);
		}

	}
	
    // destructor
	virtual ~Mutex()
	{
		pthread_mutex_unlock(&m_mMutex);
		pthread_mutex_destroy(&m_mMutex);
	}
	// lock
	int Lock()
	{
		return pthread_mutex_lock(&m_mMutex);
	}
	//unlock
	int Unlock()
	{
		return pthread_mutex_unlock(&m_mMutex);
	}
	// try lock
	int TryLock()
	{
		return pthread_mutex_trylock(&m_mMutex);
	}

private:
    pthread_mutex_t m_mMutex;   // pthread mutex
    void operator=(Mutex &m_mMutex) {} // private = operator 
    Mutex( const Mutex &m_mMutex ) {}  // private copy constructor
};

/*
DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
����δ����У�
hHandle ��ָ�򻥳�����ָ�롣
dwMilliseconds �ǳ�ʱʱ�䣬�Ժ���Ϊ��λ�������ֵ�� INFINITE����ô�����������߳�/���̵�ʱ����ǲ�ȷ���ġ�
�� Linux �У�sem_wait() ������ȡ���ź����ķ��ʡ�����������������̣߳�ֱ������ź�����һ���ǿռ���Ϊֹ��Ȼ��������ԭ�ӵؼ�������ź�����������ֵ��int sem_wait(sem_t * sem)��
�� POSIX �ź����в�û�г�ʱ�����������ͨ����һ��ѭ����ִ��һ���������� sem_trywait() ʵ�֣��ú�����Գ�ʱֵ���м��㣺int sem_trywait(sem_t * sem)��
*/

class Semaphore
{
private:	
	sem_t m_Semaphore;	
public:
	Semaphore(int initval)
	{
		sem_init(&m_Semaphore,0,initval);
	}
	~Semaphore(){
		sem_destroy(&m_Semaphore);
	}
	
	void P(){
		sem_wait(&m_Semaphore);
	};
	
	void V(){
		sem_post(&m_Semaphore);
	};

	void Reinit(int initval)
	{
		sem_init(&m_Semaphore,0,initval);
	}
	
	/*
	*��������: ��ȡ�ź����ĵ�ǰֵ �Դ��ж��ź����Ƿ��Ѿ���ס
	*����: >0 -- û���߳��ڵȴ����ź���; 
		   <=0 -- ���߳��ڵȴ����ź��� ���Ϊ�����������ֵΪ�ȴ����ź������߳��� ���巵�ظ������������ʵ������
	*/
	int GetVal()
	{
		int RetVal = -1;
		
		/*
		sem_getvalue() places the current value of the semaphore pointed to sem into the integer pointed to by sval.
       If  one  or  more processes or threads are blocked waiting to lock the semaphore with sem_wait(3), POSIX.1-2001
       permits two possibilities for the value returned in sval: either 0 is returned;  or  a  negative  number  whose
       absolute  value  is  the  count of the number of processes and threads currently blocked in sem_wait(3).  Linux
       adopts the former behaviour.
	   */

		sem_getvalue(&m_Semaphore,&RetVal);
		return RetVal;
	}
};



#endif // MUTEX_H
