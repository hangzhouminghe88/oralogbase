#include "lgc_MergeThread.h"
#include "lgc_TOriginalQueue.h"
#include "lgc_TMergedQueue.h"
#include "lgc_Transaction.h"
#include "lgc_MediaFileOutput.h"

#include "lgc_api.h"

/*
* ģ����: �ϲ��߳�
* oracle��racͬʱ������ʵ����д��־���ҿ��������������̷ֱ߳���������ڵ����־�� 
* �������Ľ���ֱ���ڸ��ԵĽ��������(����ΪTOriginalQueue)
* ��������еĽ�������񣬼����������еĽ���ǰ���commitSCN�ϸ������
* �ϲ��̵߳Ĺ��ܾ��ǽ����������кϲ���һ��������У�
* ���ս�������е�����Ҳ�ǰ���commitSCN�ϸ������
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
*�ϲ��̵߳��̺߳���
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

	//��ʼ��TOriginalQueue����
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
	
	//�ϲ��������
	BYTE8			minCommitSCN            = MAXSCN;
	unsigned short	threadIdOfMinCommitSCN	= 0;
	while(true){
		//Ѱ����СcommitSCN�����ڵ�TOriginalQueue
		for(int threadId=1; threadId <= threads; threadId++){
			BYTE8 commitSCN = getMinCommitSCNOfOriQueue(trsctOriQueueArray,threads, threadId);
			if(minCommitSCN > commitSCN){
				minCommitSCN = commitSCN;
				threadIdOfMinCommitSCN = threadId;
			}
		}
		
		//ȡ����СCommitSCN��Ӧ������
		pTransaction = popFromOriQueue(trsctOriQueueArray, threads, threadIdOfMinCommitSCN);
		//����СCommitSCN��Ӧ������push���ϲ�������
		pushToMergedQueue(trsctMergedQueue,pTransaction);
		pTransaction = NULL;
	}
	
	//never to here
	lgc_errMsg("never to here \n");
	return NULL;
}

/*
*��TOriginalQueue�л�ȡ��СCommitSCN
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
	
	//�ҵ�threadId��Ӧ��TOriginalQueue
	LGC_TOriginalQueue* pTOriQueue = trsctOriQueueArray[threadId];
	if(pTOriQueue == NULL){
		lgc_errMsg("pTOriQueue is NULL \n");
		exit(1);
	}
	
	//��TOriginalQueue�л�ȡ��ͷ�ǿ�����
	LGC_Transaction* pTransaction = NULL;
	while(NULL == 
		  (pTransaction = pTOriQueue->front() ) 
		 )
	{
		sleep(1);
	}
	
	//���طǿ������CommitSCN
	return pTransaction->getCommitSCN();

}

/*
*��TOriginalQueue�Ķ�ͷ����ȡ����
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
	
	//��ȡthreadId��Ӧ��TOriginalQueue
	LGC_TOriginalQueue* pTOriQueue = trsctOriQueueArray[threadId];
	if(pTOriQueue == NULL){
		lgc_errMsg("pTOriQueue is NULL \n");
		exit(1);
	}
	
	//��ȡTOriginalQueue�Ķ�ͷ����
	LGC_Transaction* pTransaction = pTOriQueue->front();
	if(NULL == pTransaction){
		lgc_errMsg("pTransaction is NULL \n");
		exit(1);
	}

	//����ͷ����Ӷ�����pop��ȥ
	pTOriQueue->pop_front();
	
	//���ض�ͷ����
	return pTransaction;
}

/*
*������push���ϲ�������
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
