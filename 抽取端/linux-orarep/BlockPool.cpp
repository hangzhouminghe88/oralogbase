
#include "BlockPool.h"

#include "ByteChange.h"
#include <iostream>
#include "ChuckSum.h"
#include "OciQuery.h"
#include "tw_api.h"

Mutex NodeLock;	//��ڵ���


//��̬��Ա������ʼ��
WORD RedoBlockPool::BlockIndex = 0;
WORD RedoBlockPool::PoolHead = 0;
BYTE RedoBlockPool::BlockPool[POOLSIZE][BLOCKSIZE] = {0};

bool RedoBlockPool::bHaveData = false;
//FILE *RedoBlockPool::RedoFile = NULL;
DWORD RedoBlockPool::FileSequence = 0;
bool RedoBlockPool::bFirstBlock = true;
bool RedoBlockPool::bLastBlock = false;
OciQuery *G_pQuery = NULL; // ��ASM�������ʱ����Ҫ��ѯ

BYTE ChangeCatch[3*DATASIZE]; //���ڼ���change�ĳ���

#ifdef WITHASM 
//ASM *RedoBlockPool::RedoASM = NULL;
#endif
//tw changed
RWDataBlocks* RedoBlockPool::RedoRWDataBlocks = NULL;
int RedoBlockPool::BlkOffInFile = 0;

bool RedoBlockPool::bASM = false;
int  RedoBlockPool::AUoffset = -1;
int  RedoBlockPool::AUSize = 1048576;
char *RedoBlockPool::AUPool = NULL;//[AUSIZE] = {0};

int RedoBlockPool::ms_RedoBlksReadedInAu = 0;
int RedoBlockPool::ms_RedoBlkOffInAu = 0;

bool RedoLog::bInterrupt = false;

#ifdef _WIN32
Semaphore G_AnalyseSem(0,_SEMAPHORE_SIZE);
#else
Semaphore G_AnalyseSem(0);
#endif

extern ResourceList Tranlist;
//extern int G_JobStat; //�����ж���־�����߳���Ҫ�쳣�˳���ԭ��: �����̨��������ˣ����뱸��������� tw20140610
int G_JobStat; //�����ж���־�����߳���Ҫ�쳣�˳���ԭ��: �����̨��������ˣ����뱸��������� tw20140610

/*
int RedoBlockPool::FillBlockPool(int &SucceedNum,int FillNum)
{
	if(RedoFile == NULL)
		return _FILE_ERROR;

	if (FillNum >= POOLSIZE)
		return _ERROR;

 	int LeftNum = (POOLSIZE+BlockIndex-PoolHead-1)%POOLSIZE; //���ж��ٿ���ŵ�

	int Num = FillNum>LeftNum?LeftNum:FillNum;

	int Ret = _OK;
	int HeadBefore = PoolHead;

	int i;
	DWORD LSN = 0;

	int ReadTimes;
	bool bLoadData;
	for (i=0;i<Num ;i++){
		if(bLastBlock == true){
			Ret = _BLOCK_EOF;
			break;
		}
		PoolHead = (PoolHead+1)%POOLSIZE;
		//fread �ź��ж�����
		// ��3��
		ReadTimes = ReloadTime;
		bLoadData = false;

		do{
			Ret = _OK;
			if(!fread(BlockPool[PoolHead],BLOCKSIZE,1,RedoFile)){
				if(feof(RedoFile)){
					Ret = _FILE_EOF;
					break;
				}			
				if (ferror(RedoFile) && (errno == EBUSY || errno == EINTR || errno == EIO) ){
					Ret = _FILE_ERROR;
				}
			}else{		
				bLoadData = true;
				break;
			}
		}while(ReadTimes--);

		if(bLoadData == false){
			PoolHead=(PoolHead)?(PoolHead-1):(POOLSIZE-1);
			break;
		}

		if (ORACLE_VERSION > _Oracle_9i && do_checksum(BLOCKSIZE,BlockPool[PoolHead]))	//��Ϊ0ʱ���˿鲻��ȷ
		{
			fseek(RedoFile,(-1*BLOCKSIZE),SEEK_CUR);
			PoolHead=(PoolHead)?(PoolHead-1):(POOLSIZE-1);
			Ret = _CHUCK_ERROR;
			break;
		}
		// �Ƚ�sequence(�ļ���)
		if (bFirstBlock){  //��һ��
			bFirstBlock = false;
			continue;
		}

		if (ORACLE_VERSION < _Oracle_10g){
			memcpy(&LSN,BlockPool[PoolHead],sizeof(LSN));
		}else{
			memcpy(&LSN,BlockPool[PoolHead]+_BLOCK_SEQ_OFFSET,sizeof(LSN));
		}

#ifdef _BYTECHANGE_ON_
		ChangeDWord(LSN);
#endif
		if (LSN != FileSequence)  // ��β����
		{
			bLastBlock = true;
			fseek(RedoFile,(-1*BLOCKSIZE),SEEK_CUR);
			PoolHead=(PoolHead)?(PoolHead-1):(POOLSIZE-1);
			Ret = _BLOCK_EOF;
			break;
		}
	}
	SucceedNum = PoolHead - HeadBefore;
	return Ret;
}
#ifdef WITHASM
int RedoBlockPool::FillPoolWithASM(int &SucceedNum,int FillNum)
{
	if(RedoASM == NULL)
		return _ERROR;

	if (FillNum >= POOLSIZE)
		return _ERROR;
	int Ret = _OK;

	int LeftNum = (POOLSIZE+BlockIndex-PoolHead-1)%POOLSIZE; //���ж��ٿ���ŵ�
	int Num = FillNum>LeftNum?LeftNum:FillNum;
	int HeadBefore = PoolHead;

	int i;
	DWORD LSN = 0;
	
	int ASMret;

	int ReadTimes = ReloadTime;
	bool bLoadData = true;
#ifdef _BLOCK_DEBUG_Change_ASM
	FILE* file2 = fopen("ASMlogfile.txt","ab+");
	AUoffset = AUSize;
	cout<<"AUSize:\t"<<AUSize<<endl;
	while(true)
	{
		if(AUoffset >= AUSize)
		{
			if((ASMret = RedoASM->GetOneAu(AUPool,FileSequence)) < 0){
					if(_ERROR_AU_EOF == ASMret){
						Ret = _BLOCK_EOF;
					}
					else if(_ERROR_DATA_DALY == ASMret){
						Ret = ASMret;
						sleep(1);
						continue;
					}
					else
						Ret = ASMret;
					break;
			}else{
				AUoffset = 0;
			}
		}
		fwrite(AUPool+AUoffset,BLOCKSIZE,1,file2);
		fflush(file2);
		AUoffset += BLOCKSIZE;
	}
	return -1;
#endif

	for (i=0;i<Num;i++){
		if(bLastBlock == true){
			Ret = _BLOCK_EOF;
			break;
		}
#ifdef _BLOCK_DEBUG_ASM
		cout<<"---before GetOneAu--"<<AUPool<<endl;
#endif
		if(AUoffset >= AUSize) //�ڴ���AU��������,Ҫ���µ�AU���ڴ�
		{
			bLoadData = false;
			while(ReadTimes--){
				ASMret = RedoASM->GetOneAu(AUPool,FileSequence);
				if(ASMret < 0){
					if(_ERROR_AU_EOF == ASMret){
						Ret = _BLOCK_EOF;
					}
					else if(_ERROR_DATA_DALY == ASMret){
						--ReadTimes;
						Ret = ASMret;
						fdsleep(1000);
						continue;
					}else{
						Ret = ASMret;
						cout<<"RedoASM->GetOneAu(AUPool,FileSequence) ERROR,error code: "<<Ret<<endl;
						exit(1);
					}
					break;
				}else{
					bLoadData = true;
					AUoffset = 0;
					break;
				}
			}
		}

		if(bLoadData && AUoffset < AUSize){

			PoolHead = (PoolHead+1)%POOLSIZE;	//����
			memcpy(BlockPool[PoolHead],AUPool+AUoffset,BLOCKSIZE);
			if (do_checksum(BLOCKSIZE,BlockPool[PoolHead]))	//��Ϊ0ʱ���˿鲻��ȷ
			{
				cout<<"do_checksum ERROR"<<endl;
				PoolHead=(PoolHead)?(PoolHead-1):(POOLSIZE-1);
				if(RedoASM->GetRemainBlocks() == AUoffset/BLOCKSIZE)
					Ret = _BLOCK_EOF;
				else
					Ret = _CHUCK_ERROR;
				break;
			}

			AUoffset +=	BLOCKSIZE;

			// �Ƚ�sequence(�ļ���)
			if (bFirstBlock){  //��һ��
				bFirstBlock = false;
				continue;
			}
			memcpy(&LSN,BlockPool[PoolHead]+8,sizeof(LSN));

			if (LSN != FileSequence)  // ��β����
			{
				bLastBlock = true;
				AUoffset -=	BLOCKSIZE;
				PoolHead=(PoolHead)?(PoolHead-1):(POOLSIZE-1);
				Ret = _BLOCK_EOF;
				break;
			}
		}
	}	

	SucceedNum = (PoolHead - HeadBefore + POOLSIZE)%POOLSIZE;
	return Ret;
}
#endif
*/


#ifdef _BLOCK_DEBUG_Change_ASM
	FILE* _BLOCK_DEBUG_Change_ASM_file = fopen("/home/tw/ASMlogfile.txt","ab+");
	FILE* _BLOCK_DEBUG_Change_ASM_file2 = fopen("/home/tw/ASMlogfile2.txt","ab+");
#endif

/*
int RedoBlockPool::FillBlockPool(int &SucceedNum,int FillNum)
{
	int Ret = _OK;

	int LeftNum = (POOLSIZE+BlockIndex-PoolHead-1)%POOLSIZE; //���ж��ٿ���ŵ�
	int Num = FillNum>LeftNum?LeftNum:FillNum;
	int HeadBefore = PoolHead;

	int i;
	int blksReaded;
	bool b;
	DWORD LSN = 0;
	
	int ASMret;

	int ReadTimes = ReloadTime;
	bool bLoadData = true;

	if(RedoRWDataBlocks == NULL){
		cout<<"error: RedoRWDataBlocks is NULL"<<endl;
		return _ERROR;
	}

	if (FillNum >= POOLSIZE){
		cout<<"error: FillNum is too big"<<endl;
		return _ERROR;
	}

	for (i=0;i<Num;i++){
		if(bLastBlock == true){
			Ret = _BLOCK_EOF;
			break;
		}
		PoolHead = (PoolHead+1)%POOLSIZE;
		bLoadData = false;

		blksReaded=RedoRWDataBlocks->readBlks(BlkOffInFile,BlockPool[PoolHead],1);
		if(blksReaded<0){//read failed
			bLoadData = false;
			cout<<"read Redo Block failed"<<endl;
			Ret = _ERROR;
		}else if (blksReaded==0){//read finished
			bLoadData = false;
			cout<<"EOF be readed in redoFile "<<endl;
			Ret = _FILE_EOF;
		}else{//read success
#ifdef _BLOCK_DEBUG_Change_ASM
		fwrite(BlockPool[PoolHead],BLOCKSIZE,1,_BLOCK_DEBUG_Change_ASM_file);
		fflush(_BLOCK_DEBUG_Change_ASM_file);
#endif
			bLoadData = true;
		}

		if(!bLoadData){
			PoolHead=(PoolHead)?(PoolHead-1):(POOLSIZE-1);
			break;
		}

		BlkOffInFile++;

		if (ORACLE_VERSION > _Oracle_9i && do_checksum(BLOCKSIZE,BlockPool[PoolHead]))	//��Ϊ0ʱ���˿鲻��ȷ
		{
			BlkOffInFile--;
			PoolHead=(PoolHead)?(PoolHead-1):(POOLSIZE-1);
			Ret = _CHUCK_ERROR;
			cout<<"checksum failed in RedoFile"<<endl;
			break;
		}

		if(bFirstBlock){
			bFirstBlock = false;
			continue;
		}

		//�Ƚ�scn,�ж��Ƿ�β����
		if (ORACLE_VERSION < _Oracle_10g){
			memcpy(&LSN,BlockPool[PoolHead],sizeof(LSN));
		}else{
			memcpy(&LSN,BlockPool[PoolHead]+_BLOCK_SEQ_OFFSET,sizeof(LSN));
		}

#ifdef _BYTECHANGE_ON_
		ChangeDWord(LSN);
#endif
		if (LSN != FileSequence)  // ��β����
		{

#ifdef _BLOCK_DEBUG_Change_ASM
		cout<<"LSN != FileSequence: blockOffInRedoFile="<<BlkOffInFile<<endl;
		fwrite(BlockPool[PoolHead],BLOCKSIZE,1,_BLOCK_DEBUG_Change_ASM_file2);
		fflush(_BLOCK_DEBUG_Change_ASM_file2);
#endif
			bLastBlock = true;
			BlkOffInFile--;
			PoolHead=(PoolHead)?(PoolHead-1):(POOLSIZE-1);
			Ret = _BLOCK_EOF;

			cout<<"LSN != FileSequence: LSN="<<LSN<<" FileSeqeunce="<<FileSequence<<endl;
			break;
		}

	}

	SucceedNum = (PoolHead - HeadBefore + POOLSIZE)%POOLSIZE;
	return Ret;
}
*/


int RedoBlockPool::FillBlockPool(int &SucceedNum,int FillNum)
{
	int Ret = _OK;

	int LeftNum = (POOLSIZE+BlockIndex-PoolHead-1)%POOLSIZE; //���ж��ٿ���ŵ�
	int Num = FillNum>LeftNum?LeftNum:FillNum;
	int HeadBefore = PoolHead;

	int i;
	int blksReaded;
	bool b;
	DWORD LSN = 0;
	
	int ASMret;

	int ReadTimes = ReloadTime;
	bool bLoadData = true;

	if(RedoRWDataBlocks == NULL){
		cout<<"error: RedoRWDataBlocks is NULL"<<endl;
		return _ERROR;
	}

	if (FillNum >= POOLSIZE){
		cout<<"error: FillNum is too big"<<endl;
		return _ERROR;
	}

	for (i=0;i<Num;i++){
		if(bLastBlock == true){
			Ret = _BLOCK_EOF;
			break;
		}
		PoolHead = (PoolHead+1)%POOLSIZE;
		bLoadData = false;

		blksReaded=readBlks(BlkOffInFile,1,BlockPool[PoolHead]);
		if(blksReaded<0){//read failed
			bLoadData = false;
			cout<<"read Redo Block failed"<<endl;
			Ret = _ERROR;
		}else if (blksReaded==0){//read finished
			bLoadData = false;
			cout<<"EOF be readed in redoFile "<<endl;
			Ret = _FILE_EOF;
		}else{//read success
#ifdef _BLOCK_DEBUG_Change_ASM
		fwrite(BlockPool[PoolHead],BLOCKSIZE,1,_BLOCK_DEBUG_Change_ASM_file);
		fflush(_BLOCK_DEBUG_Change_ASM_file);
#endif
			bLoadData = true;
		}

		if(!bLoadData){
			PoolHead=(PoolHead)?(PoolHead-1):(POOLSIZE-1);
			break;
		}

		BlkOffInFile++;

		if (ORACLE_VERSION > _Oracle_9i && do_checksum(BLOCKSIZE,BlockPool[PoolHead]))	//��Ϊ0ʱ���˿鲻��ȷ
		{
			BlkOffInFile--;
			PoolHead=(PoolHead)?(PoolHead-1):(POOLSIZE-1);
			Ret = _CHUCK_ERROR;
			cout<<"checksum failed in RedoFile"<<endl;
			break;
		}

		if(bFirstBlock){
			bFirstBlock = false;
			continue;
		}

		//�Ƚ�scn,�ж��Ƿ�β����
		if (ORACLE_VERSION < _Oracle_10g){
			memcpy(&LSN,BlockPool[PoolHead],sizeof(LSN));
		}else{
			memcpy(&LSN,BlockPool[PoolHead]+_BLOCK_SEQ_OFFSET,sizeof(LSN));
		}

#ifdef _BYTECHANGE_ON_
		ChangeDWord(LSN);
#endif
		if (LSN != FileSequence)  // ��β����
		{

#ifdef _BLOCK_DEBUG_Change_ASM
		cout<<"LSN != FileSequence: blockOffInRedoFile="<<BlkOffInFile<<endl;
		fwrite(BlockPool[PoolHead],BLOCKSIZE,1,_BLOCK_DEBUG_Change_ASM_file2);
		fflush(_BLOCK_DEBUG_Change_ASM_file2);
#endif
			bLastBlock = true;
			BlkOffInFile--;
			PoolHead=(PoolHead)?(PoolHead-1):(POOLSIZE-1);
			Ret = _BLOCK_EOF;

			cout<<"LSN != FileSequence: LSN="<<LSN<<" FileSeqeunce="<<FileSequence<<endl;
			break;
		}

	}

	SucceedNum = (PoolHead - HeadBefore + POOLSIZE)%POOLSIZE;
	return Ret;
}



int RedoBlockPool::GetOneBlock(BYTE *&block)
{
	block = NULL;
	int loadNum = 0;
	int RetVal = _OK;
	if (PoolHead == BlockIndex){ //����������	

//		if(false == bASM){
//			FillBlockPool(loadNum);
//		}
#ifdef WITHASM
//		else{
//			FillPoolWithASM(loadNum);
//			cout<<loadNum<<"\t"<<RetVal<<endl;
//		}
#endif
	
		FillBlockPool(loadNum);
//		cout<<"GetOneBlock: loadNum="<<loadNum<<endl;

		if (bHaveData == false) //�Ѿ����ļ�β����
		{
			if (loadNum == 0){
				RetVal = _FILE_EOF;//�Ѿ����ļ�β����
			}else if (loadNum == 1){
				BlockIndex = (BlockIndex+1)%POOLSIZE;
				block = BlockPool[BlockIndex];
				bHaveData = false;
			}else{
				BlockIndex = (BlockIndex+1)%POOLSIZE;
				block = BlockPool[BlockIndex];
				BlockIndex = (BlockIndex+1)%POOLSIZE;
				bHaveData = true;
			}
		}else{
			block = BlockPool[BlockIndex];
			if (loadNum == 0){	//���������Ѿ���ȡ��
				bHaveData = false;
			}else{
				BlockIndex = (BlockIndex+1)%POOLSIZE;
			}
		}
	}else{
		block = BlockPool[BlockIndex];
		BlockIndex = (BlockIndex+1)%POOLSIZE;
	}
	return RetVal;
}

int RedoBlockPool::SkipBlocks(DWORD SkipNum,bool bBoundary)
{
	if (SkipNum >= POOLSIZE){	//������̫�������ش�С
		return -1;
	}

	int DataNum = (POOLSIZE+PoolHead-BlockIndex)%POOLSIZE; //���ٿ��������ݵ�
	int Ret = 0;
	int Num;
	while(DataNum<SkipNum){	
//		if(false == bASM){
//			Ret = FillBlockPool(Num);
//		}
#ifdef WITHASM
//		else{
//			Ret = FillPoolWithASM(Num);
//		}
#endif
		Ret=FillBlockPool(Num);
		if (Num == 0 && Ret < 0){
			if (bBoundary == true && DataNum+1 == SkipNum){//�߽�����������ļ�ĩβ��һ�飬��ʱ˵���ļ�������
				return _FILE_EOF;
			}			
			return Ret;
		}
		DataNum = (POOLSIZE+PoolHead-BlockIndex)%POOLSIZE;
	}
	BlockIndex = (BlockIndex+SkipNum)%POOLSIZE;
	return SkipNum;
}


int RedoBlockPool::readBlks(int blkOffInFile, int blksToRead, void* blksBuf_)
{
	int iRet = -1;
	const int auSize = AUSize;
	const int blkSizeInRedoFile = 512;
	const int blksInAu = auSize/blkSizeInRedoFile;


	const int blksReadedInAu = ms_RedoBlksReadedInAu;
	const int blkOffInAu = ms_RedoBlkOffInAu;
	
	const bool isNeedReadAu = (blkOffInAu+blksToRead) <= blksReadedInAu? false:true;
	char* blksBuf = (char*)blksBuf_;

	int bytesToCpy = 0;
	int bytesCopied = 0;

	int blksCopied = 0;
	int blksToCpy = 0;

	int blkOffToRdInFile = 0;
	
	if(blksToRead > blksInAu){
		iRet = -1;
		t_errMsg("too blks to read\n");
		goto errOut;
	}


	//��AUPool�в�������ݿ�����blksBuf��
	bytesCopied = 0;
	blksCopied = 0;
	blksToCpy = isNeedReadAu? (blksReadedInAu-blkOffInAu):blksToRead;
	memcpy(blksBuf+blksCopied*blkSizeInRedoFile, AUPool+ms_RedoBlkOffInAu*blkSizeInRedoFile, blksToCpy*blkSizeInRedoFile);
	blksCopied += blksToCpy;
	ms_RedoBlkOffInAu += blksToCpy;
	
	if(!isNeedReadAu){//�����ٶ�һ��Au��AuPool
		iRet = blksCopied;
		goto errOut;
	}
	
	if(ms_RedoBlksReadedInAu != 0 && ms_RedoBlksReadedInAu < blksInAu){//�Ѿ������һ��Au��
		iRet=blksCopied;
		goto errOut;
	}

	//�ٶ�һ��AU
	blkOffToRdInFile = blkOffInFile+blksCopied;
	if(blkOffToRdInFile%blksInAu){
		t_errMsg("blkOffToRdInFile%blksInAu is not 0: blkOffToRdInFile=%d blksInAu=%d\n",blkOffToRdInFile, blksInAu);
		iRet = -1;
		exit(1);
		goto errOut;
	}

	ms_RedoBlksReadedInAu = RedoRWDataBlocks->readBlks(blkOffToRdInFile, AUPool, blksInAu);
	if(ms_RedoBlksReadedInAu<0){
		t_errMsg("read error\n");
		iRet = -1;
		exit(1);
		goto errOut;
	}
	
	//���¶���AU�п�������
	ms_RedoBlkOffInAu = 0; //���¿�ʼ����
	blksToCpy = ms_RedoBlksReadedInAu > (blksToRead-blksCopied)?(blksToRead-blksCopied):ms_RedoBlksReadedInAu;
	memcpy(blksBuf+blksCopied*blkSizeInRedoFile, AUPool+ms_RedoBlkOffInAu*blkSizeInRedoFile, blksToCpy*blkSizeInRedoFile);
	blksCopied += blksToCpy;
	ms_RedoBlkOffInAu += blksToCpy;
	
	iRet = blksCopied;



errOut:
	return iRet;
}


int RedoBlock::ReInit()
{
	int RetVal = RedoBlockPool::GetOneBlock(Head1);
	int RetVal2 = RedoBlockPool::GetOneBlock(Head2);
	if(RetVal <0){
		cout<<"GetOneBlock1 failed in ReInit: ret="<<RetVal<<endl;
		return RetVal;
	}
	if(RetVal2 <0){
		cout<<"GetOneBlock2 failed in ReInit: ret="<<RetVal2<<endl;
		return RetVal2;
	}
	ValidNum = 0;

	int LSNOffset = 8;
#ifdef _ORACLE_9I
	LSNOffset = 0;
#endif

	if (Head1){
		memcpy(&BlockNum1,Head1+4,sizeof(BlockNum1));
		memcpy(&LSN1,Head1+LSNOffset,sizeof(LSN1));
		memcpy(BlockCatch,Head1+BLOCKHEAD,DATASIZE);
		ValidNum++;
	}else{
		BlockNum1 = 0;
		LSN1 = 0;
		memset(BlockCatch,0,DATASIZE);
	}

	if (Head2){
		memcpy(&BlockNum2,Head2+4,sizeof(BlockNum2));
		memcpy(&LSN2,Head2+LSNOffset,sizeof(LSN2));
		memcpy(BlockCatch+DATASIZE,Head2+BLOCKHEAD,DATASIZE);
		ValidNum++;
	}else{
		BlockNum2 = 0;
		LSN2 = 0;
		memset(BlockCatch+DATASIZE,0,DATASIZE);
	}

#ifdef _BYTECHANGE_ON_
	ChangeDWord(BlockNum1);
	ChangeDWord(BlockNum2);
	ChangeDWord(LSN1);
	ChangeDWord(LSN2);
#endif
	
	BlockOffset = 0;
	CurrentData = BlockCatch;
	return _OK;

}

int RedoBlock::LoadNextBlock()
{
	if (ValidNum == 0 || ValidNum>2){
		return _ValidNum_Error;
	}

	int LSNOffset = 8;
#ifdef _ORACLE_9I
	LSNOffset = 0;
#endif

	bool bOffsetChange = true;

	BYTE *BlockHead;
	
	int RetVal = RedoBlockPool::GetOneBlock(BlockHead);

	if (ValidNum == 2){
		Head1 = Head2;
	}

	if (ValidNum == 1){
		bOffsetChange = false;
	}

	Head2 = BlockHead;
	ValidNum = 0;
	
	if (Head1){
		memcpy(&BlockNum1,Head1+4,sizeof(BlockNum1));
		memcpy(&LSN1,Head1+LSNOffset,sizeof(LSN1));
		memcpy(BlockCatch,Head1+BLOCKHEAD,DATASIZE);
		ValidNum++;
	}else{
		BlockNum1 = 0;
		LSN1 = 0;
		memset(BlockCatch,0,DATASIZE);
	}

	if (Head2){
		memcpy(&BlockNum2,Head2+4,sizeof(BlockNum2));
		memcpy(&LSN2,Head2+LSNOffset,sizeof(LSN2));
		memcpy(BlockCatch+DATASIZE,Head2+BLOCKHEAD,DATASIZE);
		ValidNum++;
	}else{
		BlockNum2 = 0;
		LSN2 = 0;
		memset(BlockCatch+DATASIZE,0,DATASIZE);
	}

#ifdef _BYTECHANGE_ON_
	ChangeDWord(LSN1);
	ChangeDWord(BlockNum1);
	ChangeDWord(LSN2);
	ChangeDWord(BlockNum2);
#endif

	if (bOffsetChange){
		BlockOffset = BlockOffset%DATASIZE;
		CurrentData = BlockCatch+BlockOffset;
	}
	
	return RetVal;
}

int RedoBlock::TestBoundary(DWORD Offset)
{
	if (ValidNum == 0){
		return _ValidNum_Error;
	}

	if ( BlockOffset+Offset < DATASIZE){
		return _OK;
	}
	
	if (ValidNum == 1){
		int Ret = LoadNextBlock();
		if (Ret < 0){
			return Ret;
		}
	}
	
	if ( BlockOffset+Offset < DATASIZE*2){
		return _OK;
	}

	int Ret = _OK;
	
	if ((BlockOffset<DATASIZE && (BlockOffset+Offset) >= DATASIZE*2) || //��ǰ�ڵ�һ��,ƫ�ƺ󳬹�2��
		(BlockOffset>=DATASIZE && (BlockOffset+Offset) >= DATASIZE*3) )    // ���ߵ�Ȼ�ڵڶ��飬ƫ��֧����3��
	{
		DWORD SkipNum = (((BlockOffset%DATASIZE)+Offset)/DATASIZE);
 		DWORD BlockBoundary = ((BlockOffset+Offset)%DATASIZE); //ƫ�ƺ�պ��Ǳ߽�����
		bool Boundary = BlockBoundary?false:true;
		if ((Ret = SkipBlocks(SkipNum,Boundary)) < 0){
			return Ret;
		}
	}else{
		Ret = LoadNextBlock();
	}
	return Ret;
}

int RedoBlock::Move(DWORD Offset)
{
	int Ret = _OK;
	Ret = TestBoundary(Offset);
	if (Ret < 0){  //�Ѿ������һ����
		return Ret;
	}
	
	if (BlockOffset+Offset < ValidNum*DATASIZE){
		BlockOffset += Offset;
		CurrentData += Offset;
	}else{
		BlockOffset = (BlockOffset+Offset)%DATASIZE;
		CurrentData = BlockCatch+BlockOffset;
	}
	return Ret;
}

// �ӵ�ǰ�����飬����Num��
// ����ǰ������	�ǵ�һ�飬�򻺴������Num-2��
//				�ǵڶ��飬�򻺴������Num-1��
int RedoBlock::SkipBlocks(DWORD Num,bool bBoundary)
{
	if (Num < 2)  //2��֮��,��Move����
		return -3;

	if (BlockOffset >= ValidNum*DATASIZE)
		return -2;

	DWORD nSkipNum = (BlockOffset>(DATASIZE-1))?(Num-1):(Num-2);
	DWORD BlockBeginNum = (BlockOffset>(DATASIZE-1))?(BlockNum2):(BlockNum1);
	DWORD BlockEndNum = 0;

	int Ret = RedoBlockPool::SkipBlocks(nSkipNum,bBoundary);
	if (nSkipNum != Ret){
		if (Ret == _FILE_EOF){
			return _FILE_EOF;
		}		
		return _SKIP_BLOCK_ERROR;
	}

	int RetVal = RedoBlockPool::GetOneBlock(Head1);
	int RetVal2 = RedoBlockPool::GetOneBlock(Head2);

	if (NULL == Head1 && NULL == Head2){
		return _GetOneBlock_EOF;
	}	
	ValidNum = 0;

	int LSNOffset = 8;
#ifdef _ORACLE_9I
	LSNOffset = 0;
#endif

	if (Head1){
		memcpy(&BlockNum1,Head1+4,sizeof(BlockNum1));
		memcpy(&LSN1,Head1+LSNOffset,sizeof(LSN1));
		memcpy(BlockCatch,Head1+BLOCKHEAD,DATASIZE);
		ValidNum++;
	}else{
		BlockNum1 = 0;
		LSN1 = 0;
		memset(BlockCatch,0,DATASIZE);
	}
	
	if (Head2){
		memcpy(&BlockNum2,Head2+4,sizeof(BlockNum2));
		memcpy(&LSN2,Head2+LSNOffset,sizeof(LSN2));
		memcpy(BlockCatch+DATASIZE,Head2+BLOCKHEAD,DATASIZE);
		ValidNum++;
	}else{
		BlockNum2 = 0;
		LSN2 = 0;
		memset(BlockCatch+DATASIZE,0,DATASIZE);
	}

#ifdef _BYTECHANGE_ON_
	ChangeDWord(BlockNum1);
	ChangeDWord(BlockNum2);
	ChangeDWord(LSN1);
	ChangeDWord(LSN2);
#endif
	if (BlockNum1 - BlockBeginNum != Num){// && !bFileHead){
		return _SKIP_BLOCK_ERROR;
	}

	return BlockNum1;
}

// �Ƚϵ�ǰ������һ��
// ����:
//	0:�Ѿ���β��
//	2:������һ��ͷ
// -1:�ļ�����
int RedoBlock::CompareNextBlock()
{
	int RetVal = _SKIP_BLOCK;
	if (BlockOffset>=DATASIZE && ValidNum == 2){ //��2�����õ�
		int Ret = LoadNextBlock();
		if (Ret < 0){
			CurrentData = BlockCatch+DATASIZE;
			BlockOffset = DATASIZE;
			RetVal = Ret;
		}
	}

	if (ValidNum == 1){
		RetVal = _FILE_EOF;
	}	
	CurrentData = BlockCatch+DATASIZE;
	BlockOffset = DATASIZE;

	return RetVal;
}

DWORD RedoBlock::GetCurrentNum()
{
	return GetBlockNum((BlockOffset/DATASIZE+1));
}

DWORD RedoBlock::GetBlockNum(int FirstOrSecond)
{
	switch (FirstOrSecond)
	{
	case 1:
		return BlockNum1;
	case 2:
		return BlockNum2;
	default:
		return 0;
	}
}

int RedoBlock::GetLeftInOneBlock()
{
	return (DATASIZE*2-BlockOffset)%DATASIZE;
}

DWORD RedoBlock::GetLSN(int FirstOrSecond)
{
	switch (FirstOrSecond)
	{
	case 1:
		return LSN1;
	case 2:
		return LSN2;
	default:
		return 0;
	}
}

bool RedoBlock::GetData(BYTE &date,int offset)
{
	if (BlockOffset+offset >= ValidNum*DATASIZE)
		return false;
	memcpy(&date,CurrentData+offset,sizeof(date));
	return true;
}

bool RedoBlock::GetData(WORD &date,int offset)
{
	if (BlockOffset+offset >= ValidNum*DATASIZE)
		return false;
	memcpy(&date,CurrentData+offset,sizeof(date));
#ifdef _BYTECHANGE_ON_
	ChangeWord(date);
#endif
	return true;
}

bool RedoBlock::GetData(DWORD &date,int offset)
{
	if (BlockOffset+offset >= ValidNum*DATASIZE)
		return false;
	memcpy(&date,CurrentData+offset,sizeof(date));
#ifdef _BYTECHANGE_ON_
	ChangeDWord(date);
#endif
	return true;
}

Change::Change(RedoBlock *Block){//
	m_pBlock = Block;
	HeadLength = _CHANGE_HEADSIZE; //24
	DataLength = 0;
	ChangeLength = 0;
	nSkipBlock = 0;	//�����������	0��ʾ��ͬһ����
	FileNum = 0;	//�ļ���
	BlockNum = 0;	//���
	FileSize = 0;
}

WORD Change::GetFixOffset(WORD nOffset)
{
	if( (nOffset&0x3) !=0 )
		nOffset = ((nOffset>>2)+1)<<2;
	return nOffset;	 
}

WORD Change::GetChangeState()
{
	WORD ChangeTemp = ChangeTypeH;
	WORD State = (ChangeTemp<<8)|ChangeTypeL ;
	return State;
}

void Change::GetValues(WORD &nFileNum,DWORD &nBlockNum)
{
	nFileNum = FileNum;
	nBlockNum = BlockNum;
}

bool Change::CalValues()
{
	if (m_pBlock == NULL)
		return false;
	m_pBlock->TestBoundary(HeadLength);

	m_pBlock->GetData(ChangeTypeH,_TYPEH);
	m_pBlock->GetData(ChangeTypeL,_TYPEL);
	m_pBlock->GetData(FileNum,_AFN);

	DWORD BlockTemp;
	m_pBlock->GetData(BlockTemp,_DBA);
	BlockNum = BlockTemp & _SCN_BIT ;
	return true;
}

bool Change::GetValuesInside(int ChangeType,int offset)
{
	if (m_pBlock == NULL)
		return false;
	switch (ChangeType)
	{
	case _ADDFILE:
		{
			WORD TablespaceLen=0;
			WORD TablespaceNamelen = 0;
			
			//0063364ch: 08 00 04 00 20 00 E8 00
			m_pBlock->GetData(TablespaceLen,HeadLength+2);
			m_pBlock->GetData(TablespaceNamelen,HeadLength+4);

			int FileNumOffset = TablespaceLen+TablespaceNamelen+4;//�����Ҫ�����
			DWORD dFileNum = 0;
			m_pBlock->GetData(dFileNum,offset+FileNumOffset); // �ļ���
			FileNum = dFileNum;
		}
		break;
	case _EXPANDFILE:
		{
			DWORD dFileNum = 0;
			m_pBlock->GetData(dFileNum,offset); // �ļ���
			FileNum = dFileNum;
			DWORD dFilesize = 0;
			m_pBlock->GetData(dFilesize,offset+8); //��չ���ļ���С��
			FileSize = dFilesize;
		}
		
		break;
	default:
		break;
	}
	return true;
}

int Change::CalChangeLength(bool &SkipOneBlock)
{
	if (m_pBlock == NULL)
		return 0;

	SkipOneBlock = 0;

	m_pBlock->TestBoundary(sizeof(ControlHeadLength)+HeadLength);//���Ƴ��Ȳ���ͷ2�ֽ�
	m_pBlock->GetData(ControlHeadLength,HeadLength);

// �������飬��Ϊ����֮ǰ�����ݻ�Ҫ�õ�
	if (ControlHeadLength > DATASIZE*3){
		return _ANALYSE_ERROR;
	}

	if ( m_pBlock->BlockOffset >= DATASIZE ){
		m_pBlock->LoadNextBlock();
	}	
	
	ChangeLength = HeadLength + GetFixOffset(ControlHeadLength);

	GetValuesInside(GetChangeState(),ChangeLength);

	if (m_pBlock->BlockOffset+ControlHeadLength+HeadLength < DATASIZE*2 )
	{
		WORD lengthTemp;
		for (WORD n=2; n<ControlHeadLength; n += 2){ //û�����
			m_pBlock->GetData(lengthTemp,n+HeadLength);
			DataLength += GetFixOffset(lengthTemp);
		}
	}
	else //������
	{
		memset(ChangeCatch,0,sizeof(ChangeCatch));
		int BlockLeft = DATASIZE*2-m_pBlock->BlockOffset-HeadLength;
		int NeedLoad = ControlHeadLength - BlockLeft;

		memcpy(ChangeCatch,m_pBlock->CurrentData+HeadLength,BlockLeft);
		
		m_pBlock->LoadNextBlock();
		SkipOneBlock = 1;
		if (NeedLoad <= DATASIZE){
			memcpy(ChangeCatch+BlockLeft,m_pBlock->BlockCatch+DATASIZE,NeedLoad);
		}else{
			return _ANALYSE_ERROR;
		}
		WORD lengthTemp;
		for (WORD n=2; n<ControlHeadLength; n += 2){ 
			memcpy(&lengthTemp,ChangeCatch+n,sizeof(lengthTemp));
#ifdef _BYTECHANGE_ON_
			ChangeWord(lengthTemp);
#endif
			DataLength += GetFixOffset(lengthTemp);
		}
	}

	ChangeLength += DataLength;

	CalSkipBlockNum();
 	return ChangeLength;
}

int Change::GetChangeLength()
{
	return ChangeLength;
}

int Change::CalSkipBlockNum()
{
	if (m_pBlock == NULL)
		return -1;
	nSkipBlock = ((m_pBlock->BlockOffset)%DATASIZE+ChangeLength)/DATASIZE;
	return nSkipBlock;
}

int Change::GetSkepBlockNum()
{
	return nSkipBlock;
}

//����Ҫ�����β
RedoRecord::RedoRecord(RedoBlock *Block){//
	m_pBlock = Block;	
	VLD = 0;
	m_HSCN = -1;
	m_SCN = -1;
	RedoRecordLeftlen = -1;
	nSkipBlock = 0;
	RedoRecordLength = -1;
	RedoRecordHeadlen = 0;
	m_SSCN = 0;
}

RedoRecord::~RedoRecord()
{
	this->m_Changes.clear();
}

// VLDΪ0D��05��06��09��04�ģ�������¼ͷ(��RedoRecordͷ)����Ϊ68�ֽڣ��������Ϊ24�ֽ�
// ��汾��ͬ��RedoRecordHeadlenҲ�᲻ͬ��
int RedoRecord::SkipRecordHead(int OracleVersion)
{
	if (OracleVersion < _Oracle_10g){
		RedoRecordHeadlen = 12;
	}else{		
		switch (VLD)
		{
		case 0x0D:
		case 0x05:
		case 0x06:
		case 0x04:
			RedoRecordHeadlen = 68;
			break;
		case 0x01:
		case 0x02:
		case 0x09:
		default:
			RedoRecordHeadlen = 24;
			break;
		}
	}
	RedoRecordLeftlen -= RedoRecordHeadlen;
	m_pBlock->Move(RedoRecordHeadlen);
	return RedoRecordHeadlen;
}

int RedoRecord::GetNextChange(bool &bSkipBlock)
{
	Change ChangeTemp(m_pBlock);
	ChangeTemp.CalValues();
	bool bSkipOneBlock = false;
	ChangeTemp.CalChangeLength(bSkipOneBlock);

	if (ChangeTemp.ChangeLength > 0){
		nSkipBlock += ChangeTemp.GetSkepBlockNum();
		m_Changes.push_back(ChangeTemp);
	}

	bSkipBlock = bSkipOneBlock;
	return ChangeTemp.GetChangeLength();
}

int RedoRecord::LoadallChanges()
{
	if (m_pBlock == NULL)
		return 0;

	int ChanegLen = 0;
	bool bSkipBlock = false;
	int RetVal = _OK;
	while(RedoRecordLeftlen)	//����0ʱִ��
	{
		ChanegLen = GetNextChange(bSkipBlock);
		if (ChanegLen > 0){
			RedoRecordLeftlen -= ChanegLen;
			if (RedoRecordLeftlen <0){
				return _CHANGE_ERROR;
			}
			if (bSkipBlock){//���һ��ʸ���ķ���
				RetVal = m_pBlock->Move(ChanegLen-DATASIZE);
			}else{
				RetVal = m_pBlock->Move(ChanegLen);
			}
		}else{
			return m_Changes.size();
		}
	}
	if (RedoRecordLeftlen != 0){	//�϶����������
		RetVal = _CHANGE_ERROR;
	}
	if (RetVal>0){
		RetVal = m_Changes.size();
	}
	return RetVal;
}

// ����Record����Ϊ0ʱ�����,
//	����	0:�Ѿ����ļ�β��
//			1 :����
//			2 :����
//			-1:����
int RedoRecord::HandleRecordHead(DWORD &nSCN,WORD &nHSCN,WORD &SSCN)
{
	int RetVal = _OK;
	int Blockleft = m_pBlock->GetLeftInOneBlock();
	if (Blockleft <= RECORDMINSIZE && Blockleft >0 ){ //һ����������ֽڲ���ʱ������
		RetVal = _SKIP_BLOCK;
	}else{
		if ((RedoRecordLength>=MIN_RECORD_LEN && RedoRecordLength <= MAX_RECORD_LEN)){		
			switch (VLD)
			{
			case 0x01:
			case 0x02:
			case 0x04:
			case 0x05:
			case 0x06:
			case 0x09:
			case 0x0D:
				break;
			case 0x00:
					RetVal = _SKIP_RECORDLEN;				
				break;
			default://�����ݻ���NEW VLD
				{
					if ((m_SCN == nSCN) || ( (m_SCN-nSCN)<=0x0FFF && (nSCN<m_SCN)) ){					
						char mes[256] = {0};
						sprintf(mes,"new vld:\t 0x%x BlockNum:\t%d SCN:\t%d",VLD,m_pBlock->GetCurrentNum(),m_SCN);
						cout<<mes<<endl;
							RetVal = _REDOLOG_ERROR;
					}
				}
			break;
			}
		}else{
			RetVal = _SKIP_BLOCK;
		}
	}
	
	if (RetVal == _SKIP_BLOCK){
		RetVal = m_pBlock->CompareNextBlock();
	}

	if (RetVal == _SKIP_RECORDLEN){
		int ret = m_pBlock->Move(RedoRecordLength);
		if (ret<0){
			RetVal = ret;
		}		
	}	
	return RetVal;
}


int RedoRecord::LoadRecordHead()
{
	if (m_pBlock == NULL)
		return _REDOLOG_ERROR;

	int Ret = m_pBlock->TestBoundary(_HEADSIZE);
	if (Ret <= 0){
		return Ret;
	}
	//ͷ��8�ֽ���Ҫ���⴦��
	DWORD len;
	m_pBlock->GetData(len);
	RedoRecordLength = len;
	m_pBlock->GetData(VLD,_VLD);
	m_pBlock->GetData(m_HSCN,_HSCN);
	m_pBlock->GetData(m_SCN,_LSCN);
	m_pBlock->GetData(m_SSCN,_SSCN);
	RedoRecordLeftlen = RedoRecordLength;
	return true;
}

RedoLog::RedoLog()
{
	m_FileHead = NULL;
	m_FILE = NULL;
	OperateBlock = NULL;
	OperateBlock = new RedoBlock;
	ChangeNum = 0;
	Status = _INACTIVE;
	Sequence = 0;
	m_ErrCode = _OK;
	bInterrupt = false;
	t_debugMsg(t_tmpLvl,"RedoLog::RedoLog: bInterrupt=%d\n",bInterrupt);
	memset(RedoLogName,0,sizeof(RedoLogName));
	m_pRWDataBlocks = new RWDataBlocks();
	if(!m_pRWDataBlocks){
		exit(1);
	}

}

RedoLog::~RedoLog()
{
	if (OperateBlock){
		delete OperateBlock;
		OperateBlock = NULL;
	}
	if(m_FILE){
		fclose(m_FILE);
		m_FILE = NULL;
	}
	if(m_FileHead){
		fclose(m_FileHead);
		m_FileHead = NULL;
	}

	if(m_pRWDataBlocks){
		delete m_pRWDataBlocks;
		m_pRWDataBlocks = NULL;
	}

	bInterrupt = false;
}

/*
#ifdef WITHASM 
int RedoLog::LoadNewFile_wqy(char *FileName,int nStatus,int nSequence,bool bASM,OciQuery *myQuery)
{
	if(bASM){
		if(NULL == myQuery)
			return _ERROR;
		m_FILE = NULL;
		if(m_ASM.SetAsmValues(FileName,myQuery,0) == false){
			return _ERROR_FILE_OPEN;
		}
		G_pQuery = myQuery;
		int TempAUsize = m_ASM.GetAuSize();
		if(TempAUsize != RedoBlockPool::AUSize){  //AU��С�仯�ˣ��ڴ�Ҫ��������
			if(RedoBlockPool::AUPool != NULL){
				delete RedoBlockPool::AUPool;
				RedoBlockPool::AUPool = NULL;
			}
		}
		RedoBlockPool::AUSize = TempAUsize;
		if(RedoBlockPool::AUSize > 0){
			RedoBlockPool::AUoffset = RedoBlockPool::AUSize;
		}else{
			cout<<"Get AUSize ERROR"<<endl;
			return _ERROR;
		};

		if(RedoBlockPool::AUPool == NULL){
			RedoBlockPool::AUPool = new BYTE[RedoBlockPool::AUSize]; //��̬���������ͷ�
			if(RedoBlockPool::AUPool == NULL){
				cout<<"new AUPool ERROR"<<endl;
				return _ERROR;
			}
		}

		if(RedoBlockPool::AUPool == NULL){
			RedoBlockPool::AUPool = new BYTE[RedoBlockPool::AUSize]; //��̬���������ͷ�
			if(RedoBlockPool::AUPool == NULL){
				cout<<"new AUPool ERROR"<<endl;
				return _ERROR;
			}
		}

		RedoBlockPool::SetRedoFile(&m_ASM,nSequence);
	}else if(FileName && strlen(FileName)>0){
		if(m_FILE){
			fclose(m_FILE);
			m_FILE = NULL;
		}
		m_FILE = fopen(FileName,"rb");
		if(m_FILE == NULL)
			return _ERROR_FILE_OPEN;
		RedoBlockPool::SetRedoFile(m_FILE,nSequence);
	} else {
		return _ERROR_FILE_OPEN;
	}

	strcpy(RedoLogName,FileName);
	m_FileHead = NULL;
	OperateBlock->ReInit();
	CurrentSCN = 0;
	BeginSCN = 0;
	EndSCN = 0;
	HSCN = 0xFFFF;
	SSCN = 0;
	Status = nStatus;
	Sequence = nSequence;
	return _OK;
}
#else	
int RedoLog::LoadNewFile(char *FileName,int nStatus,int nSequence)
{
	if(FileName && strlen(FileName)>0){
		if(m_FILE){
			fclose(m_FILE);
			m_FILE = NULL;
		}
		m_FILE = fopen(FileName,"rb");
		if(m_FILE == NULL)
			return _ERROR_FILE_OPEN;
		RedoBlockPool::SetRedoFile(m_FILE,nSequence);
	} else {
		return _ERROR_FILE_OPEN;
	}

	strcpy(RedoLogName,FileName);
	m_FileHead = NULL;
	OperateBlock->ReInit();
	CurrentSCN = 0;
	BeginSCN = 0;
	EndSCN = 0;
	HSCN = 0xFFFF;
	SSCN = 0;
	Status = nStatus;
	Sequence = nSequence;
	return _OK;
}

#endif
*/
#ifdef _DEBUG_LoadandSkipLogHead
FILE *LoadandSkipLogHead_file = fopen("/home/tw/LoadandSkipLogHead.txt", "ab+");
#endif

int RedoLog::LoadNewFile(char *FileName, int nStatus,int nSequence,bool bASM,OciQuery *pASMQuery)
{
	int iRet = _OK;
	bool b;
	RWInfoOfFile rwInfoOfFile;

	rwInfoOfFile.fileName = FileName; 
	rwInfoOfFile.blkSize =	BLOCKSIZE;
	rwInfoOfFile.fileType = FILE_ARCHIVE_TYPE;
	rwInfoOfFile.pQuery = pASMQuery;
	b=m_pRWDataBlocks->setFileInfo(&rwInfoOfFile);
	if(!b){
		iRet = _ERROR;
		goto errOut;
	}
	RedoBlockPool::SetRedoFile(m_pRWDataBlocks,nSequence,bASM);

	strcpy(RedoLogName,FileName);
	m_FileHead = NULL;
	iRet=OperateBlock->ReInit();
	
#ifdef _DEBUG_LoadandSkipLogHead
	cout<<"OperateBlock->ReInit()'s ret="<<iRet<<endl;
	fwrite(OperateBlock->BlockCatch,2*DATASIZE,1,LoadandSkipLogHead_file);
#endif
	CurrentSCN = 0;
	BeginSCN = 0;
	EndSCN = 0;
	HSCN = 0xFFFF;
	SSCN = 0;
	Status = nStatus;
	Sequence = nSequence;
	iRet= _OK;
errOut:
	return iRet;
}


//  ƫ��0xa4	�õ���ʼSCN
bool RedoLog::LoadandSkipLogHead()
{
	OperateBlock->Move(DATASIZE);//�Ƶ���2��
	DWORD tLSCN;
	WORD tHSCN;
#ifdef _DEBUG_LoadandSkipLogHead
	fwrite(OperateBlock->CurrentData,DATASIZE,1,LoadandSkipLogHead_file);
#endif
	OperateBlock->GetData(tLSCN,0xA4);
	OperateBlock->GetData(tHSCN,0xA4+sizeof(DWORD));
	
	fprintf(stdout,"tLSCN=%#x tHSCN=%#x \n", tLSCN, tHSCN);

	//BeginSCN = ((tHSCN<<32) + tLSCN);
	BeginSCN =  tHSCN;
	BeginSCN <<= 32;
	BeginSCN |= tLSCN;
	
	t_debugMsg(t_tmpLvl,"RedoLog::LoadandSkipLogHead(): beginSCN=%lld\n",BeginSCN);
	fprintf(stdout,"tLSCN=%#x tHSCN=%#x BeginSCN=%#x \n", tLSCN, tHSCN,BeginSCN);
	OperateBlock->GetData(tLSCN,0xB0);
	OperateBlock->GetData(tHSCN,0xB0+sizeof(DWORD));

	EndSCN =  tHSCN;
	EndSCN <<= 32;
	EndSCN |= tLSCN;


	OperateBlock->Move(DATASIZE);
	return true;
}

#ifdef _BLOCK_DEBUG_Change
	int G_Line = 0;
#ifdef _WIN_32
	FILE *ChangeLog = fopen("E:\\Change.txt","w");
#else
	FILE *ChangeLog = fopen("/home/tw/Change.txt","w");
#endif
#endif

int RedoLog::GetNextRecord()
{
#ifdef _BLOCK_DEBUG_Change
	if (G_Line == 60756){
		int x=0;
	}
#endif
	
	RedoRecord TempRecord(OperateBlock);
	TempRecord.LoadRecordHead();

	int RetVal = 2;	
	while(RetVal)
	{
		RetVal = TempRecord.HandleRecordHead(CurrentSCN,HSCN,SSCN);
		if (RetVal < 0){
			return RetVal;
		}else if (RetVal == _OK){
			break;
		}
		TempRecord.LoadRecordHead();
	}

	TempRecord.SkipRecordHead(ORACLE_VERSION);
	RetVal = TempRecord.LoadallChanges();
	if ( RetVal < 0 && RetVal != _BLOCK_EOF && RetVal != _FILE_EOF && RetVal != _GetOneBlock_EOF ){
		return RetVal;
	}else{
		Changelist::iterator plist;
		int num=1;
		for(plist = TempRecord.m_Changes.begin(); plist != TempRecord.m_Changes.end(); ++plist)
		{
			WORD ChState = plist->GetChangeState();
#ifdef _BLOCK_DEBUG_Change
			if(ChangeLog){
				if (plist->FileNum == 0){
					fprintf(ChangeLog,"CHANGE #%d MEDIA RECOVERY MARKER SCN:0x0000.00000000 SEQ:  0\n",num++);
				}else{
					int headnum = (OperateBlock->BlockOffset/0x200)?2:1;
					fprintf(ChangeLog,"CHANGE #%d AFN: %d\tDBA: %5x SCN:\t%x BOLCK:\t%x Offset:\t 0x%x\t ChState:\t%d\n",num++,
						plist->FileNum,plist->BlockNum,TempRecord.m_SCN,OperateBlock->GetCurrentNum(),(OperateBlock->BlockOffset%0x200)+0x10*headnum,ChState);
				}				
			}	
#endif
//////////////////////////////////////////////////////////////////////////
//		��������������ļ�����չ�ļ��Ĵ���
//////////////////////////////////////////////////////////////////////////
			//t_debugMsg(4,"ChState=0x%x  _ADDFILE=0x%x ORACLE_VERSION=%d _Oracle_11g=%d _Oracle_10g=%d\n",
			//	ChState,_ADDFILE, ORACLE_VERSION, _Oracle_11g, _Oracle_10g);
			switch (ChState)
			{
			case _ADDFILE:
				{
					t_debugMsg(t_addExpandLvl,"������change�����ļ���Ӳ�������������change���������̣߳�Ȼ��ȴ������ļ�������\n RedoFileName:%s\t sequence:%u\t DataFileNum:%u\n ",
											  RedoLogName, RedoBlockPool::FileSequence, plist->FileNum);
					cout<<"������change�����ļ���Ӳ�������������change���������߳�: fileNum="<<plist->FileNum<<endl;

					m_NodeProducer.InsertOneNode(_ANALYSE_ADDFILE,plist->FileNum);
					m_NodeProducer.CutFromList(Tranlist);
					
					//G_AnalyseSemÿ��������������ס
					if(G_AnalyseSem.GetVal()>0){//û����ס
						G_AnalyseSem.Reinit(0); //��֤����ʱһ������ס
					}
					G_AnalyseSem.P();
					continue;
				}
				break;
			case _EXPANDFILE:
				{
					t_debugMsg(t_addExpandLvl,"������change�����ļ���չ��������������change���������̣߳�Ȼ��ȴ������ļ�������\n RedoFileName:%s\t sequence:%u\t DataFileNum:%u\n ",
											  RedoLogName, RedoBlockPool::FileSequence, plist->FileNum);
					cout<<"������change�����ļ���չ��������������change���������߳�: fileNum="<<plist->FileNum<<endl;
					m_NodeProducer.InsertOneNode(_ANALYSE_EXPAND,plist->FileNum,plist->FileSize);
					m_NodeProducer.CutFromList(Tranlist);
					//G_AnalyseSemÿ��������������ס
					if(G_AnalyseSem.GetVal()>0){//û����ס
						G_AnalyseSem.Reinit(0); //��֤����ʱһ������ס
					}
					G_AnalyseSem.P();
					continue;
				}
				break;
			default:
				break;
			}
			
			if (plist->FileNum == 0){
				continue;
			}
			t_debugMsg(t_dbgChangesLvl,"m_NodeProducer.InsertOneNode: fileNum=%d blockIndex=%d\n", plist->FileNum,plist->BlockNum);
			m_NodeProducer.InsertOneNode(plist->FileNum,plist->BlockNum);
		}
#ifdef _BLOCK_DEBUG_Change
		if(ChangeLog){
			fflush(ChangeLog);
		}	
#endif
	}
	return RetVal;
}


void RedoLog::GetErrorMsg(int &BlockNum,int &SCN,int &Offset,char *TheReason)
{
	if(OperateBlock){
		BlockNum = OperateBlock->GetCurrentNum();
		SCN = CurrentSCN;
		Offset = OperateBlock->BlockOffset%DATASIZE;
	}
	switch(m_ErrCode)
	{
	case _FILE_EOF:
		strcpy(TheReason,"File is end");
		break;
	case _BLOCK_EOF:
		strcpy(TheReason,"Block Eof");
		break;
	case _CHANGE_ERROR:
		strcpy(TheReason,"Change Length Error");
		break;
	case _CHUCK_ERROR:
		strcpy(TheReason,"Chucksum Error");
		break;
	case _ANALYSE_ERROR:
		strcpy(TheReason,"Analyse Error");
		break;
	case _FILE_ERROR:
		strcpy(TheReason,"Read File Error");
		break;
	case _REDOLOG_ERROR:
		strcpy(TheReason,"REDOLOG ERROR");
		break;
	default:
		sprintf(TheReason,"Unknow reason:%d",m_ErrCode);
		break;
	};
}

///////////////////////////////////////////////////////////////////////////////////////////
struct TimeThreadArgs
{
	Producer* pProducer;
	DWORD *pCount;
	Mutex *pMutex;
	TimeThreadArgs(){
		pProducer = NULL;
		pCount = NULL;
		pMutex = NULL;
	}

};

ReturnType TimeThread(LPVOID *Context)
{
	Producer *pProd = ((TimeThreadArgs*)Context)->pProducer;
	DWORD *pCou = ((TimeThreadArgs*)Context)->pCount;
	Mutex *pMut = ((TimeThreadArgs*)Context)->pMutex;

	if (NULL == pProd || NULL == pMut || NULL == pCou){
		return (ReturnType)-1;
	}
	
	while (true)
	{		
		fdsleep(TIMER_TIME);		
		pMut->Lock();
		if (*pCou == -1){
			if (pProd->GetNodeNum()){	
				pProd->CutFromList(Tranlist);
			}
			pMut->Unlock();
			break;
		}
		
		if (*pCou > TIMER_COUNT){
			if (pProd->GetNodeNum()){					
				if (pProd->CutFromList(Tranlist) == false){ //consumer�˻�û�ж�ȡ����
					pMut->Unlock();
					return (ReturnType)0;//������
				}
			}
			*pCou = 0;	
		}else{
			(*pCou)++;
		}	
		pMut->Unlock();		
	}
	return (ReturnType)0;
}
////////////////////////////////////////////////////////////////////////////////////////////////

int RedoLog::Loadall()
{
	int retval=0;

	m_Count = 0;
	while(true)
	{
		m_ErrCode = GetNextRecord();
		if(bInterrupt){
			cout<<"�����ж�"<<endl;
			t_debugMsg(t_dbgLvl,"�����ж� RedoAnalyseEnd() start:\n");
			//edit by tw 20140610
			if(G_JobStat == _REP_STAT_RUN){//�������̨��������� ��Ҫ����Ľڵ����ж϶��߳� 
				t_debugMsg(t_dbgLvl,"network between zhu and houtai have disconneted, send a special Node to make read thread exit\n");
				t_errMsg("network between zhu and houtai have disconneted, send a special Node to make read thread exit\n");
				RedoAnalyseEnd();
			}else{//����������뱸��������� ���߳��Լ����ж��Լ�
				t_debugMsg(t_dbgLvl,"network between zhu and bei have disconneted, read thread will exit by itself\n");
				t_errMsg("network between zhu and bei have disconneted, read thread will exit by itself\n");
			}
			retval = _ANALYSE_INTERRUPT;
			fdsleep(TIMER_TIME);
			t_debugMsg(t_dbgLvl,"�����ж� RedoAnalyseEnd() end: retval=%d _ANALYSE_INTERRUPT=%d \n",retval, _ANALYSE_INTERRUPT);
			t_errMsg("�����ж� RedoAnalyseEnd() end: retval=%d _ANALYSE_INTERRUPT=%d \n",retval, _ANALYSE_INTERRUPT);
			return retval;
		}

		if (m_NodeProducer.GetNodeNum() > BLOCKGAP ){
			if (m_NodeProducer.CutFromList(Tranlist) == false){ //consumer�˻�û�ж�ȡ����
				t_errMsg("m_NodeProducer.CutFromList error\n");
				return false;//������
			}else{
				m_Count = 0;
			}			
		}

		retval=m_ErrCode;
		if (m_ErrCode == _FILE_EOF)	{
			cout<<"�ļ�����:\t_FILE_EOF"<<endl;
			t_debugMsg(t_redoLogFileLvl,"�ļ�����:\t_FILE_EOF\n");
			break;
		}
		if (m_ErrCode == _GetOneBlock_EOF || m_ErrCode == _BLOCK_EOF)	{
			cout<<"�����:\t_GetOneBlock_EOF"<<endl;
			t_debugMsg(t_redoLogFileLvl,"�����:\t_GetOneBlock_EOF\n");
			break;
		}

		if (m_ErrCode < 0)	{
			t_debugMsg(t_redoLogFileLvl,"Loadall m_ErrCode:%d\n",m_ErrCode);
			cout<<"Loadall m_ErrCode:\t"<<m_ErrCode<<endl;
			break;
		}
	}
		
	if (m_NodeProducer.GetNodeNum()){	
		m_NodeProducer.InsertOneNode(_BLOCK_EOF,BeginSCN); //edit by tw 20140609 ���һ���鵵�Ľ���
		m_NodeProducer.CutFromList(Tranlist);
		retval = _BLOCK_EOF;
		t_debugMsg(t_redoLogFileLvl,"RedoLog::Loadall: BeginSCN=%lld retval=%d\n",BeginSCN, retval);
	}
	fdsleep(TIMER_TIME);
	return retval;
}

void RedoLog::RedoAnalyseEnd()
{
	t_debugMsg(t_dbgLvl,"RedoLog::RedoAnalyseEnd() start\n");
	m_NodeProducer.InsertOneNode(_ANALYSE_END,0);	
	m_NodeProducer.CutFromList(Tranlist);
	t_debugMsg(t_dbgLvl,"RedoLog::RedoAnalyseEnd() end\n");
}

bool RedoLog::InsertSpecialNode(int Node)
{
	m_NodeProducer.InsertOneNode(Node,0);	
	m_NodeProducer.CutFromList(Tranlist);
	return true;
}

bool RedoLog::InsertOneNode(int nFile,int nBlock)
{
	NodeLock.Lock();
	m_NodeProducer.InsertOneNode(nFile,nBlock);
	NodeLock.Unlock();
}

int RedoLog::InsertLocked()
{
	return NodeLock.Lock();
}

int RedoLog::InsertUnlocked()
{
	return NodeLock.Unlock();
}
