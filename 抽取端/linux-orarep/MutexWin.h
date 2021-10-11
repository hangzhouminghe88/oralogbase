#ifndef _MUTEX_H_
#define _MUTEX_H_
///////////////////////////////////////////////////////////////////////////////
// Mutex.h
// Rohit Joshi
// It implements the mutex for windows
//////////////////////////////////////////////////////////////////////////////
#include <windows.h>

// 信号量大小
#define  _SEMAPHORE_SIZE 1


// Lock class
template <class T > class Lock 
{
	T& m_obj; // type object
public:
    // Lock
	Lock(T& obj):m_obj(obj)
	{
		m_obj.Lock();
	}
	// Unlock
	~Lock()
	{
		m_obj.Unlock();
	}
};
// Mutex class for windows
class Mutex
{
public:
	// constructor
    Mutex()
	{
		InitializeCriticalSection(&m_mMutex);
	}
    // destructor
	virtual ~Mutex()
	{
		DeleteCriticalSection(&m_mMutex);
	}
	// lock
	bool Lock()
	{
		EnterCriticalSection(&m_mMutex);
		return true;
	}
	// unlock
	bool Unlock()
	{
		LeaveCriticalSection(&m_mMutex);
		return true;
	}
	// try lock
	bool TryLock()
	{
		/*
		if(TryEnterCriticalSection(&m_mMutex)) {
			return true;
		}else {
		    return false;
		}
		*/
		
	}

private:
    CRITICAL_SECTION  m_mMutex;  // critical section as mutex
    void operator=(Mutex &m_mMutex) {} // private = operator
    Mutex( const Mutex &m_mMutex ) {} // private copy constructor
};

class Semaphore
{
private:	
	HANDLE m_Semaphore;	
public:
	Semaphore(int initval,int size)
	{
		m_Semaphore = NULL;
		m_Semaphore = CreateSemaphore(NULL,initval,size,NULL);
	}
	~Semaphore(){
		if (m_Semaphore){
			CloseHandle(m_Semaphore);
		}
	}
	
	bool P(DWORD dwMilliseconds = INFINITE){		
		DWORD Ret = WaitForSingleObject(m_Semaphore,dwMilliseconds);
		if (Ret == WAIT_OBJECT_0)
			return true;
		return false;		
	};
	
	void V(int num = 1){
		if (m_Semaphore){
			ReleaseSemaphore(m_Semaphore,num,NULL);
		}
	};
};



#endif // MUTEX_H
