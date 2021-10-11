#include "lgc_MergeThread.h"
#include "lgc_TOriginalQueue.h"
#include "lgc_TMergedQueue.h"
#include "lgc_Transaction.h"
#include "lgc_MediaFileOutput.h"

#include "lgc_api.h"

/*
* 模块名: 合并线程
* oracle的rac同时有两个实例在写日志，我开启了两个分析线程分别分析两个节点的日志， 
* 分析出的结果分别放在各自的结果队列中(命名为TOriginalQueue)
* 结果队列中的结果是事务，假设结果队列中的结果是按照commitSCN严格排序的
* 合并线程的功能就是将多个结果队列合并成一个结果队列，
* 最终结果队列中的事务也是按照commitSCN严格排序的
*/


static const BYTE8 MAXSCN = 0xffffffffffff;

static BYTE8 getMinCommitSCNOfOriQueue(LGC_TOriginalQueue** trsctOriQueueArray, 
                                       unsigned short threads, 
									   unsigned short threadId);

static LGC_Transaction* popFromOriQueue(LGC_TOriginalQueue** trsctOciQueueArray, 
                                        unsigned short threads,
										unsigned short threadId);

static void pushToMergedQueue(LGC_TMergedQueue* pTMergedQueue, 
                              LGC_Transaction* pTransaction);

/*
*合并线程的线程函数
*/
void* mergeThreadFunc(void* pMergeThreadArg)
{
	const LGC_MergeThreadArg*		pThreadArg				= (LGC_MergeThreadArg*)pMergeThreadArg;
	const unsigned short			threads					= pThreadArg->threads;
	const char*						mediaFileDir			= pThreadArg->mediaFileDir;

	LGC_TMergedQueue*				trsctMergedQueue		= LGC_TMergedQueue::getInstance();
	LGC_Transaction*				pTransaction			= NULL;
	LGC_MediaFileOutput*			pMediaFileOutput		= LGC_MediaFileOutput::getInstance();
	LGC_TOriginalQueue**			trsctOriQueueArray		= new LGC_TOriginalQueue*[threads+1];
	
	pMediaFileOutput->setMediaFileDir(mediaFileDir);

	//初始化TOriginalQueue数组
	if(NULL == trsctOriQueueArray){
		lgc_errMsg("new failed \n");
		exit(1);
	}
	memset(trsctOriQueueArray,0,sizeof(LGC_TOriginalQueue*)*(threads+1));
	for(int threadId=1; threadId <= threads; threadId++){
		trsctOriQueueArray[threadId] = LGC_TOriginalQueue::getInstance(threadId);
		if(NULL == trsctOriQueueArray[threadId]){
			lgc_errMsg("getInstance failed \n");
			exit(1);
		}
	}
	
	//合并结果队列
	BYTE8			minCommitSCN            = MAXSCN;
	unsigned short	threadIdOfMinCommitSCN	= 0;
	while(true){
		//寻找最小commitSCN号所在的TOriginalQueue
		for(int threadId=1; threadId <= threads; threadId++){
			BYTE8 commitSCN = getMinCommitSCNOfOriQueue(trsctOriQueueArray,threads, threadId);
			if(minCommitSCN > commitSCN){
				minCommitSCN = commitSCN;
				threadIdOfMinCommitSCN = threadId;
			}
		}
		
		//取出最小CommitSCN对应的事务
		pTransaction = popFromOriQueue(trsctOriQueueArray, threads, threadIdOfMinCommitSCN);
		//将最小CommitSCN对应的事务push到合并队列中
		pushToMergedQueue(trsctMergedQueue,pTransaction);
		pTransaction = NULL;
	}
	
	//never to here
	lgc_errMsg("never to here \n");
	return NULL;
}

/*
*从TOriginalQueue中获取最小CommitSCN
*/
BYTE8 getMinCommitSCNOfOriQueue(LGC_TOriginalQueue** trsctOriQueueArray, 
                     unsigned short threads, 
					 unsigned short threadId)
{
	if(!lgc_check(threads >= 1
				  && threadId >= 1
		          && threadId <= threads))
	{
		lgc_errMsg("check failed \n");
		exit(1);
	}
	
	//找到threadId对应的TOriginalQueue
	LGC_TOriginalQueue* pTOriQueue = trsctOriQueueArray[threadId];
	if(pTOriQueue == NULL){
		lgc_errMsg("pTOriQueue is NULL \n");
		exit(1);
	}
	
	//从TOriginalQueue中获取队头非空事务
	LGC_Transaction* pTransaction = NULL;
	while(NULL == 
		  (pTransaction = pTOriQueue->front() ) 
		 )
	{
		sleep(1);
	}
	
	//返回非空事务的CommitSCN
	return pTransaction->getCommitSCN();

}

/*
*将TOriginalQueue的队头事务取出来
*/
LGC_Transaction* popFromOriQueue(LGC_TOriginalQueue** trsctOriQueueArray, 
                                 unsigned short threads,
								 unsigned short threadId)
{
	if(!lgc_check(threads >= 1 
				  && threadId >= 1
				  && threadId <= threads))
	{
		lgc_errMsg("check failed \n");
		exit(1);
	}
	
	//获取threadId对应的TOriginalQueue
	LGC_TOriginalQueue* pTOriQueue = trsctOriQueueArray[threadId];
	if(pTOriQueue == NULL){
		lgc_errMsg("pTOriQueue is NULL \n");
		exit(1);
	}
	
	//获取TOriginalQueue的队头事务
	LGC_Transaction* pTransaction = pTOriQueue->front();
	if(NULL == pTransaction){
		lgc_errMsg("pTransaction is NULL \n");
		exit(1);
	}

	//将队头事务从队列中pop出去
	pTOriQueue->pop_front();
	
	//返回队头事务
	return pTransaction;
}

/*
*将事务push到合并队列中
*
*/
void pushToMergedQueue(LGC_TMergedQueue* pTMergedQueue, 
                       LGC_Transaction* pTransaction)
{
	if(pTMergedQueue == NULL){
		lgc_errMsg("pTMergedQueue is NULL \n");
		exit(1);
	}

	pTMergedQueue->push(pTransaction);
	pTransaction = NULL;

	return;
}
