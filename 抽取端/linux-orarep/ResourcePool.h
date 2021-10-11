//**************************************************************
//*
//* 文件名: ResourcePool.h
//* 设 计 者：wqy 日期：2012-5-24:15
//* 修 改 者：    日期：2012-5-24:15
//*
//****************************************************************

#ifndef _NEWRESOURCEPOOL_
#define _NEWRESOURCEPOOL_

#include "fd_dlist.h"
#include "Defines.h"

#ifdef _WIN32
	#include "MutexWin.h"
#else
	#include "Mutex.h"
#endif

#include <stdio.h>
#include <vector>

using namespace std;

struct BlockNode 
{
	dlink link;
	int FileIndex;	//将DWORD改为int  ，负数时特殊定义
	DWORD BlockIndex;
	DWORD Num;
	BlockNode()
	{
		FileIndex = 0;
		BlockIndex = 0;
		Num = 0;
		link.next = NULL;
		link.prev = NULL;
	}
	BlockNode(const BlockNode& Node)
	{
		this->FileIndex = Node.FileIndex;
		this->BlockIndex = Node.BlockIndex;
		this->Num = Node.Num;
		this->link.next = Node.link.next;
		this->link.prev = Node.link.prev;
	}
	BlockNode& operator = (const BlockNode& Node)
	{
		this->FileIndex = Node.FileIndex;
		this->BlockIndex = Node.BlockIndex;
		this->Num = Node.Num;
		this->link.next = Node.link.next;
		this->link.prev = Node.link.prev;
		return *this;
	}
};

enum{
	_TYPE_PRODUCER,
	_TYPE_CONSUMER,
};

class ResourceList:public dlist
{
public:
	bool pull(int num,ResourceList &List); //  从头部切num个
	bool pull(void *item,ResourceList &List);
	bool pull(void *item,int num,ResourceList &List);
	bool pullall(ResourceList &List);

	bool pushback(ResourceList &List);	//加到尾部

	void *GetandRemoveHead();	//从头部得到一个数据
};

//////////////////////////////////////////////////////////////////////////
//生产者
class Producer{
private:
	BlockNode* m_CurrentNode;	
public:
	ResourceList m_Reslist;
	
	Producer();
	~Producer();
	
	int GetNodeNum();

	BlockNode *GetOneNode();
	void BackOneNode(BlockNode *node);

 	bool InsertOneNode(int nFile,DWORD nBlock,DWORD Num=1);
	bool InsertOneNode(BlockNode *node);

 	bool CutFromList(ResourceList &List);
	
};

//////////////////////////////////////////////////////////////////////////
// 消费者
class Consumer{
private:
	BlockNode* m_CurrentNode;

	ResourceList m_Reslist;

public:

	Consumer();
	~Consumer();

	inline bool IsHaveData(){
		return (m_CurrentNode)?true:false;
	};

	int  GetOneNode(BlockNode& Node,int nFile = 0);
	bool GetOneNode(DWORD &nFile,DWORD &nBlock,DWORD &nNum);

	bool GetFromList(ResourceList &List);
	bool GetFromProduce();

	bool GiveBack2Pool(ResourceList &List);
	bool GiveBack2Pool();

	bool CleanPool();//清空缓存 主要是为了释放锁

};

#endif

