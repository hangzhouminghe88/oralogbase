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
在这段代码中：
hHandle 是指向互斥句柄的指针。
dwMilliseconds 是超时时间，以毫秒为单位。如果该值是 INFINITE，那么它阻塞调用线程/进程的时间就是不确定的。
在 Linux 中，sem_wait() 用来获取对信号量的访问。这个函数会挂起调用线程，直到这个信号量有一个非空计数为止。然后，它可以原子地减少这个信号量计数器的值：int sem_wait(sem_t * sem)。
在 POSIX 信号量中并没有超时操作。这可以通过在一个循环中执行一个非阻塞的 sem_trywait() 实现，该函数会对超时值进行计算：int sem_trywait(sem_t * sem)。
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
	*函数功能: 获取信号量的当前值 以此判断信号量是否已经锁住
	*返回: >0 -- 没有线程在等待该信号量; 
		   <=0 -- 有线程在等待该信号量 如果为负数，其绝对值为等待该信号量的线程数 具体返回负数还是零根据实现来的
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
