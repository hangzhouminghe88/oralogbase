//**************************************************************
//*
//* �ļ���: ResourcePool.h
//* �� �� �ߣ�wqy ���ڣ�2012-5-24:15
//* �� �� �ߣ�    ���ڣ�2012-5-24:15
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
	int FileIndex;	//��DWORD��Ϊint  ������ʱ���ⶨ��
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
	bool pull(int num,ResourceList &List); //  ��ͷ����num��
	bool pull(void *item,ResourceList &List);
	bool pull(void *item,int num,ResourceList &List);
	bool pullall(ResourceList &List);

	bool pushback(ResourceList &List);	//�ӵ�β��

	void *GetandRemoveHead();	//��ͷ���õ�һ������
};

//////////////////////////////////////////////////////////////////////////
//������
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
// ������
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

	bool CleanPool();//��ջ��� ��Ҫ��Ϊ���ͷ���

};

#endif

