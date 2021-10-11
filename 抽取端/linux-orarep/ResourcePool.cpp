

#include "ResourcePool.h"
#include "tw_api.h"
#include <iostream>

//声明
int CompareNode(void *item1, void *item2);
	
Mutex s_Mutex;

#ifdef _WIN32
Semaphore s_EmptySem(_SEMAPHORE_SIZE,_SEMAPHORE_SIZE);
Semaphore s_FullSem(0,_SEMAPHORE_SIZE);
#else
Semaphore s_EmptySem(_SEMAPHORE_SIZE);
Semaphore s_FullSem(0);
#endif


Mutex PoolLock;	//缓存池锁

ResourceList Tranlist; //全局的变量，传递数据用
ResourceList ResourcePool;

//提出前num个元素
bool ResourceList::pull(int num,ResourceList &List)
{
	if (num > size())
		return false;

	void *tempItem = first();

	if (num == size()){
		return pull(last(),num,List);
	}
	
	for(int i=1;i<num;i++)
	{
		tempItem = next(tempItem);
	}
	
	return pull(tempItem,num,List);
}

bool ResourceList::pull(void *item,ResourceList &List)
{	
	int num = 1;
	bool bFound = false;
	void *TempItem = first();
	while(TempItem)
	{
		if (TempItem == item)
		{
			bFound = true;
			break;
		}
		TempItem = next(TempItem);
		++num;
	}
	
	int pullnum = bFound?num:0;
	
	return pull(item,pullnum,List);
}
//删除从头部开始num个
bool ResourceList::pull(void *item,int num,ResourceList &List)
{
	if (num > size() || 0 == num || (!item))
		return false;
	
	if (num == size()){
		List = *this;
		init();
		return true;
	}
	
	void *Head = first();
	void *SaveHead = Head;
	
	dlink *ilink = get_link(item);
    Head = ilink->next;
	if (Head) { //没删光
		set_prev(Head, NULL);
	}
	
	List.SetVals(SaveHead,item,num,GetOffset());
	
	return true;
}

bool ResourceList::pullall(ResourceList &List)
{
	bool retval = false;
	retval = pull(size(),List);
	return retval;
}

bool ResourceList::pushback(ResourceList &List)
{
	if (List.size() == 0){
		return false;
	}

	if (size() == 0){
		*this = List;
		return true;
	}

	void *Head2 = List.first();
	
	set_prev(Head2,tail);
	if (tail) {
		set_next(tail, Head2);
	}

	tail = List.last();
	num_items += List.size();
	
	return true;
}

void *ResourceList::GetandRemoveHead()
{
	void *Head = first();
	if (Head){
		remove(Head);
	}
	return Head;
}


Producer::Producer():m_CurrentNode(NULL)
{
	m_Reslist.init(m_CurrentNode,&m_CurrentNode->link);
}

Producer::~Producer()
{
	m_Reslist.destroy();
}

int Producer::GetNodeNum()
{
	return m_Reslist.size();
}
//从缓存池中得到一块内存 暂时先用new

#ifdef _BLOCK_DEBUG_MEM
FILE *NodeFile = fopen("/tmp/GetOneNode.txt","w+");
#endif

BlockNode* Producer::GetOneNode()
{
	BlockNode *newNode = NULL;

	PoolLock.Lock();
	newNode = (BlockNode *)ResourcePool.GetandRemoveHead();

	if (NULL == newNode){
#ifdef _BLOCK_DEBUG_MEM
		fprintf(NodeFile,"----New BlockNode\n");
		fflush(NodeFile);
#endif
		newNode = new BlockNode;
	}
#ifdef _BLOCK_DEBUG_MEM
	else{
		fprintf(NodeFile,"ResourcePool,Left:\t%d\n",ResourcePool.size());
		fflush(NodeFile);
	}
#endif
	PoolLock.Unlock();
	return newNode;
}

void Producer::BackOneNode(BlockNode* node)
{
	if (node){
		PoolLock.Lock();
		ResourcePool.append(node);
		PoolLock.Unlock();		
	}
}

bool Producer::InsertOneNode(int nFile,DWORD nBlock,DWORD Num)
{
	BlockNode *node = GetOneNode();
	if (NULL == node){
		node = GetOneNode();
	}
	node->FileIndex = nFile;
	node->BlockIndex = nBlock;
	node->Num = Num;
	if (false == InsertOneNode(node)){
		BackOneNode(node);
	}
	return true;
}

bool Producer::InsertOneNode(BlockNode *node)
{
	if (node == m_Reslist.binary_insert(node,CompareNode)){
		return true;
	}
	return false;
}

bool Producer::CutFromList(ResourceList &List)
{
	t_debugMsg(t_dbgLvl,"Producer::CutFromList start:\n");
	t_debugMsg(t_dbgLvl,"Producer::CutFromList: s_EmptySem.GetVal()=%d s_FullSem.GetVal()=%d\n",s_EmptySem.GetVal(),s_FullSem.GetVal());
	bool retval = false;
	s_EmptySem.P();
	s_Mutex.Lock();
	retval = m_Reslist.pullall(List);
	s_Mutex.Unlock();
	s_FullSem.V();
	t_debugMsg(t_dbgLvl,"Producer::CutFromList: s_EmptySem.GetVal()=%d s_FullSem.GetVal()=%d\n",s_EmptySem.GetVal(),s_FullSem.GetVal());
	t_debugMsg(t_dbgLvl,"Producer::CutFromList end:\n");
	return retval;
}

Consumer::Consumer():m_CurrentNode(NULL)
{
	m_Reslist.init(m_CurrentNode,&m_CurrentNode->link);
}

Consumer::~Consumer()
{
	GiveBack2Pool();
}

int Consumer::GetOneNode(BlockNode& Node,int nFile)
{
	if (m_CurrentNode)
	{
		int TempFile = m_CurrentNode->FileIndex;
		if (TempFile == nFile || 0 == nFile || TempFile <0 )	//小于0时，特殊定义
		{
			Node = *m_CurrentNode;
			m_CurrentNode = (BlockNode *)m_Reslist.next(m_CurrentNode);
		}		
		return TempFile;
	}
	else
		return 0;
}

bool Consumer::GetFromProduce()
{
	return GetFromList(Tranlist);// Tranlist全局变量
}

bool Consumer::GetFromList(ResourceList &List)
{	
	t_debugMsg(t_dbgLvl,"Consumer::GetFromList start:\n");
	t_debugMsg(t_dbgLvl,"Consumer::GetFromList: s_EmptySem.GetVal()=%d s_FullSem.GetVal()=%d\n",s_EmptySem.GetVal(),s_FullSem.GetVal());
	s_FullSem.P();
	s_Mutex.Lock();
	m_Reslist = List;
	m_CurrentNode = (BlockNode *)m_Reslist.first();
	List.init();
	s_Mutex.Unlock();
	s_EmptySem.V();
	t_debugMsg(t_dbgLvl,"Consumer::GetFromList: s_EmptySem.GetVal()=%d s_FullSem.GetVal()=%d\n",s_EmptySem.GetVal(),s_FullSem.GetVal());
	t_debugMsg(t_dbgLvl,"Consumer::GetFromList end:\n");
	return true;
}

bool Consumer::GiveBack2Pool()
{
	return GiveBack2Pool(m_Reslist);
}

bool Consumer::GiveBack2Pool(ResourceList &List)
{
	bool RetVal;
	PoolLock.Lock();
	RetVal = ResourcePool.pushback(List);	//ResourcePool 全局变量 
	m_CurrentNode = NULL;	
	PoolLock.Unlock();
	return RetVal;
}

//清空缓存 主要是为了释放锁
bool Consumer::CleanPool()
{
	bool bRet = true;
	
	cout<<s_EmptySem.GetVal()<<"\t"<<s_FullSem.GetVal()<<endl;
	t_debugMsg(t_dbgLvl,"Consumer::CleanPool(): s_EmptySem.GetVal()=%d s_FullSem.GetVal()=%d\n",s_EmptySem.GetVal(),s_FullSem.GetVal());
	if(s_EmptySem.GetVal() <= 0 && s_FullSem.GetVal() >0){
		GiveBack2Pool();
		GetFromProduce();
	}
	
	
normal_out:
	return bRet;
}


//item1的num为1
int CompareNode(void *item1, void *item2)
{
	BlockNode *node1, *node2;
	node1 = (BlockNode *)item1;
	node2 = (BlockNode *)item2;

	if (node1->FileIndex < 0){ //文件号小于0时，作为特殊定义，插到最后
		return 1;
	}

	if (node1->Num != 1){
		return 0;
	}
	
	if (node1->FileIndex < node2->FileIndex){
		return -1;
	}
	
	if (node1->FileIndex > node2->FileIndex){
		return 1;
	}
	
	if (node1->BlockIndex < node2->BlockIndex){ //不是相邻的
		return -1;
	}
	
	if (node2->BlockIndex < node1->BlockIndex){
		return 1;
	}
	/*
	if (node2->BlockIndex == node1->BlockIndex+1) //需要更新
	{
		node2->BlockIndex = node1->BlockIndex;
		node2->Num++;
	}else if ((node2->BlockIndex+node2->Num) == node1->BlockIndex)
	{
		node2->Num++;
	}
	*/
	return 0;
}
