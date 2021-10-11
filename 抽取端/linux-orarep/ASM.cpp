//**************************************************************
//* ASM.cpp
//****************************************************************

#include "ASM.h"

#if 0
// 设置全局变量作为缓存
// 用G_Groups[Index]来索引磁盘组信息

extern Datafiles G_Datafiles;
vector<vector<DISK> > G_Disks;
vector<Group> G_Groups;

//通过查询，载入所有磁盘信息
int LoadAllDisksOCI()
{
	XPTR	xptr;
	KFBH	kfbh;
	KFFFDB	kfffdb;//文件头信息
	KFDHDB	kfdhdb;
	BYTE	*pBlock = NULL;

	RedologINI ConfigINI;
	ConfigINI.LoadFile(_CONFIG_FILE);
	char *strUSER = new char[MAX_NAME_LENGTH];
	char *strPASSWD = new char[MAX_NAME_LENGTH];
	char *strSID = new char[MAX_NAME_LENGTH];

	ConfigINI.LoadValuse("ASM", "USER", strUSER);
	ConfigINI.LoadValuse("ASM", "PASSWD", strPASSWD);
	ConfigINI.LoadValuse("ASM", "ASM", strSID);

	OciQuery *ASMQuery = new OciQuery();

	// 清理全局变量
	for(int i=0;i<G_Disks.size();i++){
		G_Disks[i].clear();
	}
	G_Disks.clear();
	G_Groups.clear();


	ASMQuery->SetValses(strSID,strUSER,strPASSWD);
	ASMQuery->GetAllGroupInfo(&G_Groups);

	int GroupSize = G_Groups.size(); 
	int Size = GroupSize;
	for(int i=0;i<Size;i++){ // 可能存在不连续Group的情况
		if(GroupSize < G_Groups[i].GroupNum){
			GroupSize = G_Groups[i].GroupNum;
		}
	}

	ASMQuery->GetAllASMDisks(&G_Disks,GroupSize);

	int size1 = G_Disks.size();
	int size2 = 0;
	char strFullPath[STR_LEN];
	char strDiskName[STR_LEN];
	int headlen = sizeof(ASMHEAD);
	for(int index=0;index<size1;index++){ //这里处理磁盘路径为CRTL:DISK1 这种情况
		size2 = G_Disks[index].size();
		for(int j=0;j<size2;j++){
			if(G_Disks[index][j].Path[0] != '/' && !strncmp(ASMHEAD,G_Disks[index][j].Path,headlen)){ //ORCL:
				memset(strFullPath,0,sizeof(strFullPath));
				strcpy(strDiskName,(G_Disks[index][j].Path+headlen-1));
				ConfigINI.LoadValuse("ASM", strDiskName, strFullPath);
				if(strFullPath[0] != 0){
					strncpy(G_Disks[index][j].Path,strFullPath,sizeof(G_Disks[index][j].Path));
				}else{
					cout<<"配置磁盘信息出错:\t"<<G_Disks[index][j].Path<<endl;
					return -1;
				}
			}
		}
	}

	// 循环读所有DISK，导入Group信息
	int nGroupNum = -1;
	int FDisk = -1;
	for (int i=0;i<G_Groups.size();++i){
		if (G_Groups[i].GroupNum != -1){ // 有效的Group
			nGroupNum = G_Groups[i].GroupNum;
			if (nGroupNum >= size1){
				return _ERROR_INIT;
			}

			if (pBlock){
				delete pBlock;
				pBlock = NULL;
			}
			pBlock = new BYTE[G_Groups[i].Block_Size];

			if (G_Groups[i].pMetadataFile == NULL){
				G_Groups[i].pMetadataFile = new ASMFile;
			}			

			FDisk = -1;
			size2 = G_Disks[nGroupNum].size();
			for(int j=0;j<size2;j++){
				FDisk = myopen(G_Disks[nGroupNum][j].Path,MYOPEN_READ);
				if(FDisk<=2){
					cout<<"Open "<<G_Disks[nGroupNum][j].Path<<" Error\n"<<endl;
					return _ERROR_OPEN;
				}

				// 文件头
				memset(pBlock,0,G_Groups[i].Block_Size);
				int readblock = myread(FDisk,pBlock,G_Groups[i].Block_Size);
				if (readblock<0){
					return _ERROR_READ;
				}

				memcpy(&kfdhdb,pBlock+KFBH_L,KFDHDB_L);

				if (kfdhdb.f1b1locn != 0){
					G_Groups[i].Redundancy_Type		= kfdhdb.grptyp;
					G_Groups[i].MetaBlockSize		= kfdhdb.blksize;

					if (G_Groups[i].Block_Size != kfdhdb.blksize || G_Groups[i].AU_Size != kfdhdb.ausize){
						return _ERROR_INIT;
					}

					BYTE8 temp_AUsize		= G_Groups[i].AU_Size;
					BYTE8 temp_AU			= kfdhdb.f1b1locn;
					BYTE8 temp_BlockSize	= G_Groups[i].Block_Size;
					BYTE8 temp_BlockNum		= 1;	// 1号文件
					BYTE8 seeklen			= temp_AUsize*temp_AU+temp_BlockSize*temp_BlockNum;//元数据
					// 载入1号文件
					BYTE8 pos2 = mylseek(FDisk,seeklen,SEEK_SET);
					int posread2 = myread(FDisk,pBlock,G_Groups[i].Block_Size); // 读入4096
					if (posread2 == -1){
						return _ERROR_READ;
					}

					memset(&kfffdb,0,KFFFDB_L);
					memcpy(&kfffdb,pBlock+KFBH_L,KFFFDB_L);

					ASMFile *pASMFile		= G_Groups[i].pMetadataFile;

					pASMFile->MetaBlockSize = G_Groups[i].MetaBlockSize;
					pASMFile->AU_Size		= G_Groups[i].AU_Size;
					pASMFile->BlockSize		= kfffdb.blkSize;
					pASMFile->TotelAUCount	= kfffdb.xtntcnt;
					pASMFile->DirectRedundancy= (kfffdb.dXrs & 0x0F);
					pASMFile->InDirectRedundancy = (kfffdb.iXrs & 0x0F);

					// 处理索引信息
					int CountIndex		= kfffdb.xtntblk;	// 总共有多少索引,索引是从0开始，
					int InDirectBegin	= kfffdb.asm_break;	// 间接索引开始位置

					//pASMFile->AUIndexs[0].AuNum = kfdhdb.f1b1locn;				//元数据AU
					//pASMFile->AUIndexs[0].DiskNum = G_Disks[nGroupNum][j].DiskNum;		
					int InDirectIndexCount = (CountIndex-InDirectBegin)/pASMFile->InDirectRedundancy; // 间接索引数，如:(63-60)/3 = 1

					if (InDirectIndexCount>0){
						pASMFile->AUIndexs.resize(InDirectIndexCount+1);	// 加上一个直接索引
					}else{
						pASMFile->AUIndexs.resize(1);
					}					
					
					XPTR xptr_tmp;
					int xptr_pos;
					memset(&xptr_tmp,0,sizeof(xptr_tmp));
					xptr_pos = KFBH_L+ POS_AU;
					memcpy(&xptr_tmp,pBlock+xptr_pos,XPTR_L);

					pASMFile->AUIndexs[0].AuNum		= xptr_tmp.au;
					pASMFile->AUIndexs[0].DiskNum	= xptr_tmp.disk;
					pASMFile->AUIndexs[0].BlockNum  = 1;	//

					if (InDirectIndexCount >0){					
						for (int i=0; i<InDirectIndexCount; ++i){ // 把所有间接索引加到容器中去
							memset(&xptr_tmp,0,sizeof(xptr_tmp));
							xptr_pos = KFBH_L+ POS_AU + XPTR_L*(InDirectBegin+i*pASMFile->InDirectRedundancy);
							memcpy(&xptr_tmp,pBlock+xptr_pos,XPTR_L);
							pASMFile->AUIndexs[i+1].AuNum	= xptr_tmp.au;
							pASMFile->AUIndexs[i+1].DiskNum = xptr_tmp.disk;	
						}
					}

					break;
				}else{	// 元数据不在这磁盘上
					continue;
				}
			}
		}
	}

	if(ASMQuery){
		delete ASMQuery;
		ASMQuery = NULL;
	}

	if(strUSER){
		delete[] strUSER;
		strUSER = NULL;
	}

	if(strPASSWD){
		delete[] strPASSWD;
		strPASSWD = NULL;
	}

	if(strSID){
		delete[] strSID;
		strSID = NULL;
	}
}


ASM::ASM()
{
	OneBlock = new BYTE[MAX_BLOCK_SIZE];
	m_pASMFile	= NULL;	
	m_pDisks	= NULL;		// 指向全局缓存里的一个数组
	m_pGroup	= NULL;		// 指向全局变量中的Group信息
	m_FileAuOffset = 0;
}

ASM::~ASM()
{
	if (OneBlock){
		delete[] OneBlock;
		OneBlock = NULL;
	}	
}

// 元数据时，pASMFile指向的就是元数据，在全局量中用0号文件索引
// DiskNum,DiskFD表示在转化过程中打开的磁盘号与文件描述符
int ASM::LogicalAUToPhysicalAU(BYTE8 LogicalAU,XPTR &PhysicalAU,ASMFile *pASMFile,int &DiskNumOut,int &DiskFDOut)
{
	int Index = -1;
	int DiskNum;
	BYTE8 AuNum;
	int IndexBlock =0;
	int IndexInBlock =0;
	int BlockCountInAU = pASMFile->AU_Size/pASMFile->BlockSize;
	if (LogicalAU < (Diretc_Ext/pASMFile->DirectRedundancy)){
		Index = 0;
		IndexBlock =0;
		IndexInBlock = (LogicalAU*pASMFile->DirectRedundancy);	// 落在直接索引里的哪个块里
	}else{														// 一般不会超过1，如果超过1，那就得有 (60+480*256)*256个文件(不存在冗余)
		Index = 1+(LogicalAU-(Diretc_Ext/pASMFile->DirectRedundancy))/((AUinMetaB/pASMFile->DirectRedundancy)*BlockCountInAU);
		IndexBlock = (LogicalAU-(Diretc_Ext/pASMFile->DirectRedundancy))/(AUinMetaB/pASMFile->DirectRedundancy);	// 到底落在哪个480里了
		IndexInBlock = (LogicalAU*pASMFile->DirectRedundancy-Diretc_Ext)%AUinMetaB;	// 落在480里的哪个块里
	}

	if (Index>=0){												// 元数据索引已经超过60M，也就是存在60*256个文件(不存在冗余的情况下),要载入间接索引
		if (Index < pASMFile->AUIndexs.size()){
			AuNum	= pASMFile->AUIndexs[Index].AuNum;
			DiskNum = pASMFile->AUIndexs[Index].DiskNum;
			if (Index == 0){
				IndexBlock = pASMFile->AUIndexs[Index].BlockNum;
			}
			
		}else{
			return _ERROR_ASM;
		}
	}else{
		return _ERROR_ASM;
	}

#ifdef _DEBUG_ON_ASM

	cout<<"AuNum:\t"<<AuNum<<endl;
	cout<<"DiskNum:\t"<<DiskNum<<endl;
	cout<<"Index:\t"<<Index<<endl;
	cout<<"IndexBlock:\t"<<IndexBlock<<endl;
	cout<<"Disk Path:\t"<<m_pDisks->at(DiskNum).Path<<endl;

#endif

	//元数据AU所在硬盘		先打开硬盘,再定位到AU
	int FD = GetDisk(DiskNum);
	if (FD<0){
		return _ERROR_DISK;
	}

	BYTE8 seeklen=pASMFile->AU_Size*AuNum;
	BYTE8 skipblockhead = 0;

	if (Index==0){	//直接索引,则要跳过1块，相当于blkn=1.
		seeklen += pASMFile->MetaBlockSize*IndexBlock;
		skipblockhead = KFBH_L+POS_AU+XPTR_L*IndexInBlock;	//跳过块的头部
	}else{			// 间接索引
		seeklen += pASMFile->MetaBlockSize*IndexBlock;		// 一般来说，MetaBIndexBlock都为0，超过(60+480*256)*256个文件时，MetaBIndexBlock大于0
		skipblockhead = KFBH_L+KFFIXB_L+XPTR_L*IndexInBlock;
	}

	BYTE8 pos = mylseek(FD,seeklen,SEEK_SET);
	if (pos<0 || pos != seeklen){
		cout<<"FD:\t"<<FD<<endl;
		cout<<"pos != seeklen\t"<<pos<<" "<<seeklen<<endl;
		return _ERROR_SEEK;
	}


#ifdef _DEBUG_ON_ASM

	cout<<"FD:\t"<<FD<<endl;
	cout<<"pos:\t"<<pos<<endl;
	cout<<"seeklen:\t"<<seeklen<<endl;
	cout<<"skipblockhead:\t"<<skipblockhead<<endl;

#endif

	memset(OneBlock,0,pASMFile->MetaBlockSize);
	int posread = myread(FD,OneBlock,pASMFile->MetaBlockSize);// 4096
	if (posread == -1){
		return _ERROR_READ;
	}

	// 读取文件头所以的AU
	memset(&PhysicalAU,0,XPTR_L);
	memcpy(&PhysicalAU,OneBlock+skipblockhead,XPTR_L);

#ifdef _DEBUG_ON_ASM
	cout<<"PhysicalAU:\t"<<PhysicalAU.disk<<"  "<<PhysicalAU.au<<endl;
#endif
	int TmpDisk = PhysicalAU.disk;
	if(TmpDisk <0 || TmpDisk >= m_pDisks->size()){
		return _ERROR_DISK;
	}

	DiskNumOut = DiskNum;
	DiskFDOut = FD;
	return _ASM_OK;
}

int ASM::LoadASMFileHead()
{
	int RetVal = _ASM_OK;
	if (NULL == m_pGroup || NULL == m_pDisks){
		return _ERROR_INIT;
	}
	
	if (m_pGroup->pMetadataFile == NULL){
		return _ERROR_INIT;
	}

	XPTR  xptr;
	KFFFDB kfffdb;//文件头信息

	BYTE8 temp_AUsize	= m_pGroup->AU_Size;
	ASMFile *pMETAFile	= m_pGroup->pMetadataFile;

	int BlockCountInAU	= pMETAFile->AU_Size/pMETAFile->BlockSize;	// 一个AU里有几个元数据索引  1M为256
	int IndexExtent		= m_pASMFile->ASMFileIndex/BlockCountInAU;
	int IndexBlock		= m_pASMFile->ASMFileIndex%BlockCountInAU;			// 索引AU里的块,比如257号文件，就是 IndexAU*1*冗余+IndexBlock

	int TempFD = -1;
	int TempDiskNum = -1;
	RetVal = LogicalAUToPhysicalAU(IndexExtent,xptr,pMETAFile,TempDiskNum,TempFD);
	if (RetVal <0){
		cout<<"LogicalAUToPhysicalAU Error:\t"<<endl;
		return RetVal;
	}

#ifdef _DEBUG_ON_ASM
	cout<<"m_pGroup:" <<m_pGroup->AU_Size<<" GroupNum "<<m_pGroup->GroupNum<<endl;
	cout<<"LogicalAU: "<<IndexExtent<<endl;
	cout<<"PhyAU: "<<xptr.au<<endl;
#endif

	BYTE8 temp_BlockNum		= IndexBlock;
	BYTE8 temp_AU			= xptr.au;
	BYTE8 seekfilehead		= temp_AUsize*temp_AU + pMETAFile->BlockSize*temp_BlockNum;//AU Index

	int FileFD = 0;
	if (xptr.disk != TempDiskNum){ //换磁盘了
		FileFD = GetDisk(xptr.disk);
		if (FileFD<0){
			return _ERROR_DISK;
		}
	}else{
		FileFD = TempFD;
	}

	BYTE8 pos = mylseek(FileFD,seekfilehead,SEEK_SET);
	if (pos<0 || pos != seekfilehead){
		return _ERROR_SEEK;
	}
	memset(OneBlock,0,m_pASMFile->BlockSize);
	int posread2 = myread(FileFD,OneBlock,m_pGroup->MetaBlockSize);// 4096
	if (posread2 == -1){
		return _ERROR_READ;
	}

	memset(&kfffdb,0,KFFFDB_L);
	memcpy(&kfffdb,OneBlock+KFBH_L,KFFFDB_L);

	m_pASMFile->MetaBlockSize		= m_pGroup->MetaBlockSize;
	m_pASMFile->BlockSize			= kfffdb.blkSize;
	m_pASMFile->TotelAUCount		= kfffdb.xtntcnt;
	m_pASMFile->DirectRedundancy	= (kfffdb.dXrs & 0x0F);
	m_pASMFile->InDirectRedundancy	= (kfffdb.iXrs & 0x0F);

	if(m_pASMFile->InDirectRedundancy == 0 || m_pASMFile->DirectRedundancy == 0){
		cout<<" read head error"<<endl;
		return _ERROR_DISK;
	}

	// 处理索引信息
	int CountIndex		= kfffdb.xtntblk;		// 总共有多少索引,索引是从0开始，
	int InDirectBegin	= kfffdb.asm_break;	// 间接索引开始位置

	int InDirectIndexCount = (CountIndex-InDirectBegin)/m_pASMFile->InDirectRedundancy; // 间接索引数，如:(63-60)/3 = 1	

	if (InDirectIndexCount>0){
		m_pASMFile->AUIndexs.resize(InDirectIndexCount+1);	// 加上一个直接索引
	}else{
		m_pASMFile->AUIndexs.resize(1);
	}	

	m_pASMFile->AUIndexs[0].AuNum	= xptr.au;
	m_pASMFile->AUIndexs[0].DiskNum = xptr.disk;
	m_pASMFile->AUIndexs[0].BlockNum  = IndexBlock;

	XPTR xptr_tmp;
	int xptr_pos;
	for (int i=0; i<InDirectIndexCount; ++i){ // 把所有间接索引加到容器中去
		memset(&xptr_tmp,0,sizeof(xptr_tmp));
		xptr_pos = KFBH_L+ POS_AU + XPTR_L*(InDirectBegin+i*m_pASMFile->InDirectRedundancy);
		memcpy(&xptr_tmp,OneBlock+xptr_pos,sizeof(xptr_tmp));
		m_pASMFile->AUIndexs[i+1].AuNum		= xptr_tmp.au;
		m_pASMFile->AUIndexs[i+1].DiskNum	= xptr_tmp.disk;	
	}
	return _ASM_OK;
}

int ASM::GetASMFileInfo(char *FileName,OciQuery *myQuery)
{
	if( FileName==NULL || FileName[0] != '+'){// || NULL == pFind)
		cout<<"文件名格式不对:\t"<<endl;
		return _ERROR_FILENAME;
	}

	string strtemp = FileName;
	string strGroup;
	string strFile;

	int pos = strtemp.find('/');
	if (pos > 0){
		strGroup = strtemp.substr(1,pos-1);
	}

	int pos2 = strtemp.rfind('/');
	if (pos2 > 0){
		strFile = strtemp.substr(pos2+1,strtemp.length()-pos2-1);
	}

	if (0 == strFile.length() || 0 == strGroup.length()){
		cout<<"文件名格式不对:\t"<<FileName<<endl;
		return _ERROR_FILENAME;
	}

	m_pGroup = NULL;
	int Index;
	for(Index =0; Index<G_Groups.size(); ++Index){
		if(!strGroup.compare(G_Groups[Index].Name)){ // group信息以group的名字为准
			m_pGroup = &G_Groups[Index];
			break;
		}
	}

	if(m_pGroup){ // groupnum 放到m_pASMFile->去
		m_pASMFile->GroupNum = m_pGroup->GroupNum;
		m_pASMFile->AU_Size	 = m_pGroup->AU_Size;
		m_pDisks = &G_Disks[m_pGroup->GroupNum];
	}else{
		cout<<"载入 Group 信息出错:TempName:\t"<<strGroup<<endl;
		return _ERROR_FILENAME;
	}

	if(myQuery->GetASMFileInfo2(m_pASMFile,(char *)strFile.c_str()) < 0){ //这里用到OCI，得到ASM中的文件号
		cout<<"GetASMFileInfo2 Error:\t"<<strFile.c_str()<<endl;
		return _ERROR_FILENAME;
	}

	LoadASMFileHead();
	return _ASM_OK;

}

int ASM::SetAsmValues(char *FileName,OciQuery *myQuery,int FileIndex)
{
	int RetVal = _ASM_OK;
	// FileIndex =0表示此文件为归档
	if (FileIndex <0 || FileIndex >= G_Datafiles.size() || (FileIndex &&G_Datafiles[FileIndex].FileName[0] == 0)){	// 文件号出错
		cout<<"Wrong FileIndex:\t"<<FileIndex<<endl;
		return false;
	}
	else if (FileIndex > 0){

#ifdef _DEBUG_ON_ASM_BLOCK
	cout<<"FileName:\t"<<FileName<<endl;
	cout<<"FileIndex:\t"<<FileIndex<<endl;
#endif

		// 确定文件索引与文件名相对应
		if(strcmp(G_Datafiles[FileIndex].FileName,FileName)){
			cout<<G_Datafiles[FileIndex].FileName<< "!= "<<FileName<<endl;
			return _ERROR_FILENAME;
		}

		if(G_Datafiles[FileIndex].pASMFile == NULL){ //ASM信息未载入到全局变量中
			m_pASMFile = new ASMFile;
		}else{
			m_pASMFile = G_Datafiles[FileIndex].pASMFile;
		}

#ifdef _DEBUG_ON_ASM_BLOCK
	cout<<"m_pASMFile->reload:\t"<<m_pASMFile->reload<<endl;
#endif
		if(m_pASMFile->reload == true){  					// 添加文件后相关信息要重新载入
			RetVal = GetASMFileInfo(FileName,myQuery);		// 先更新ASM数据文件信息
			if(RetVal <0 ){
				cout<<"GetASMFileInfo Error"<<endl;
				return RetVal;
			}else{
				m_pASMFile->reload = false;
			}
		}
#ifdef _DEBUG_ON_ASM_HIT
		else{
			cout<< "命中文件号:\t"<<FileIndex<<endl;
		}
#endif
		G_Datafiles[FileIndex].pASMFile = m_pASMFile;
	}else if (FileIndex ==0){ // 归档
		if (m_pASMFile == NULL){
			m_pASMFile = new ASMFile;
		}
		ClearASMFileInfo();
		RetVal = GetASMFileInfo(FileName,myQuery);
		if(RetVal <0 ){
			cout<<"GetASMFileInfo Error"<<endl;
			return RetVal;
		}
		m_pASMFile->bLogfile = true;
	}	

	m_FileAuOffset = 0;
	m_BlockRemain = m_pASMFile->Blocks;

	return RetVal;
}

int ASM::ClearASMFileInfo(){
	m_pASMFile->Clear();
	return _ASM_OK;
}

int ASM::GetBlocks(BYTE *pMem,BYTE8 StartBlock,int BlockNum)
{
	return GetBlocks(pMem,m_pASMFile,StartBlock,BlockNum);
}

int ASM::GetBlocks(BYTE *pMem,ASMFile *pFile,BYTE8 StartBlock,int BlockNum)
{
#ifdef _DEBUG_ON_ASM_BLOCK
	cout<<"GetBlocks started..."<<endl;
	cout<<"pFile->BlockSize:\t"<<pFile->BlockSize<<endl;
	cout<<"pFile->AU_Size:\t"<<pFile->AU_Size<<endl;
#endif

	int RetVal = _ASM_OK;
	int NumSize = pFile->AU_Size/pFile->BlockSize; //每个AU能有的块数
	int StartAu = StartBlock / NumSize; // NumSize一般为128
	int BlockInsideAU = StartBlock % NumSize;	//对128取余

#ifdef _DEBUG_ON_ASM_BLOCK
	cout<<"StartAu:\t"<<StartAu<<endl;
	cout<<"StartBlock:\t"<<StartBlock<<endl;
#endif

	if(StartAu <0 || StartAu >= pFile->TotelAUCount/pFile->DirectRedundancy){ //从0开始，要小于总的AU数
		cout<< "if(StartAu <0 || StartAu >= AuNum)"<<StartAu<<" AuNum:\t"<<pFile->TotelAUCount<<endl;
		return _ERROR_STARTAU;
	}

	if(StartBlock > pFile->Blocks || StartBlock+BlockNum > pFile->Blocks){
		cout<< "StartBlock > pFile->Blocks || StartBlock+BlockNum > pFile->Blocks"<<StartBlock<<"+"<<BlockNum<<" > "<<pFile->Blocks<<endl;
		return _ERROR_STARTAU;
	}

	XPTR  xptr;
	int TempFD = -1;
	int TempDiskNum = -1;
	RetVal = LogicalAUToPhysicalAU(StartAu,xptr,pFile,TempDiskNum,TempFD);
	if (RetVal <0){
		cout<<"GetAU LogicalAUToPhysicalAU Error:\t"<<endl;
		return RetVal;
	}

#ifdef _DEBUG_ON_ASM_BLOCK
	cout<<"xptr.au:\t"<<xptr.au<<endl;
	cout<<"xptr.disk:\t"<<xptr.disk<<endl;
#endif

	if(BlockInsideAU + BlockNum > NumSize){		//123+7>128  跳AU了，要分段拷数据，分Num1=5和Num2=2
		int Num1 = NumSize -BlockInsideAU;		// + m_BlockNums  
		int Num2 = BlockInsideAU + BlockNum - NumSize;
		RetVal = ASM2MEM(pMem,xptr.disk,xptr.au,BlockInsideAU,StartBlock,Num1);
		if (RetVal <0){
			cout<<"GetAU ASM2MEM Error:\t"<<endl;
			return RetVal;
		}

		XPTR  xptr2;
		RetVal = LogicalAUToPhysicalAU(StartAu+1,xptr2,pFile,TempDiskNum,TempFD);
		if (RetVal <0){
			cout<<"GetAU LogicalAUToPhysicalAU Error:\t"<<endl;
			return RetVal;
		}

		RetVal = ASM2MEM(
			pMem+m_pASMFile->BlockSize*Num1,
			xptr2.disk,
			xptr2.au,
			0,
			StartBlock+Num1,
			Num2);
	}else{
		RetVal = ASM2MEM(pMem,xptr.disk,xptr.au,BlockInsideAU,StartBlock,BlockNum);
	}

	return RetVal;

}

int ASM::GetOneAu(BYTE* AuPool,DWORD nSeq)
{
	if (m_pASMFile == NULL){
		return _ERROR_INIT;
	}
	
	if(m_FileAuOffset >= (m_pASMFile->TotelAUCount/m_pASMFile->DirectRedundancy)){ //超过总共AU数 
		return _ERROR_AU_EOF;
	};

	int RetVal = GetAU(AuPool,m_pASMFile,m_FileAuOffset,nSeq);

	if (RetVal > 0){
		// 计算还剩下多少块
		int BlocksInoneAU = m_pASMFile->AU_Size/m_pASMFile->BlockSize;
		if(m_BlockRemain>BlocksInoneAU){
			m_BlockRemain -= BlocksInoneAU;
		}else{
			m_BlockRemain = 0;
		}
		++m_FileAuOffset;
	}

	return RetVal;	

}

int ASM::GetAU(BYTE *pMem,int AUStart,int nSeq)
{
	return GetAU(pMem,m_pASMFile,AUStart,nSeq);
}

int ASM::GetAU(BYTE *pMem,ASMFile *pFile,int AUStart,int nSeq)
{
	int RetVal = _ASM_OK;
	int NumSize = pFile->AU_Size/pFile->BlockSize; //每个AU能有的块数
	int StartAu = AUStart;

	if(StartAu <0 || StartAu > pFile->TotelAUCount){ //从0开始，要小于总的AU数
		cout<< "if(StartAu <0 || StartAu >= AuNum)"<<StartAu<<" AuNum:\t"<<pFile->TotelAUCount<<endl;
		return _ERROR_STARTAU;
	}

	XPTR  xptr;
	int TempFD = -1;
	int TempDiskNum = -1;
	RetVal = LogicalAUToPhysicalAU(StartAu,xptr,pFile,TempDiskNum,TempFD);
	if (RetVal <0){
		cout<<"GetAU LogicalAUToPhysicalAU Error:\t"<<endl;
		return RetVal;
	}

	bool bFirstAU = AUStart==0?1:0;
	RetVal = ASM2MEM(pMem,xptr.disk,xptr.au,0,0,NumSize,nSeq,bFirstAU);

	return RetVal;
}

int ASM::GetRemainBlocks()
{
	return m_BlockRemain;
}

// BlockNum+(StartBlock%128) 要小于128，不一定为128，这里是例子 
// 开始块与块数都作为形参传入
// 返回值: 读到数据的大小,出错是为负数
// 如果 BlockNum = AU_Size/BlockSize 就表示读入一个AU
// nSeq 表示读入第几块AU

int ASM::ASM2MEM(BYTE* TheMem,int DiskNum,int AUNum,int BlockInsideAU,int StartBlockNum,int BlockNum,int nSeq,bool bFristBlock)
{
	if(NULL == TheMem){
		return _ERROR_ASM;
	}

	if(NULL == m_pGroup){
		cout<<"Error\tSetAsmValues First"<<endl;
		return _ERROR_ASM;
	}

	int FdMem = GetDisk(DiskNum);
	if(FdMem <= 0){
		cout<<"GetDisk Error"<<endl;
		return _ERROR_DISK;
	}

	int RetVal		= _ASM_OK;
	BYTE8 temp_AUsize = m_pASMFile->AU_Size;
	BYTE8 temp_AU	= AUNum;

	BYTE8 au_num	= temp_AUsize*temp_AU;
	BYTE8 readsize	= BlockNum*m_pASMFile->BlockSize;
	BYTE8 seekpos	= au_num + BlockInsideAU*m_pASMFile->BlockSize;

	int retry = 0;
	int stat = 0;
	bool bChuckSum = false;
	int RetCheck = 0;
	for(int i=0;i<CHECK_SUM_TIMES;i++){
		bChuckSum = true;
		int count = 0;
		do {
ASM_Check:
			BYTE8 pos = mylseek(FdMem,seekpos,SEEK_SET);
			if(pos != seekpos){
				cout<<"lseek(FdMem,seekpos,SEEK_SET) ERROR:"<<seekpos<<endl;
				return _ERROR_SEEK;
			}
#ifdef _DEBUG_ON_ASM_POS
			cout<<"lseek pos:\t"<<pos<<endl;
			cout<<"AUNum:\t"<<AUNum<<endl;
			cout<<"FdMem:\t"<<FdMem<<endl;
#endif
			if ((retry > 0 && stat == -1 && errno == EBUSY)) {
				fdsleep(1);
			}
			stat = myread(FdMem,TheMem,readsize);
			if(stat != -1){
				if (nSeq == 0){ // 读块
					RetCheck = CheckMem(TheMem,StartBlockNum,BlockNum,m_pASMFile->BlockSize);
				}else{
					RetCheck = CheckMem(TheMem,nSeq,0,m_pASMFile->BlockSize,bFristBlock);
				}
				if(RetCheck < 0){
					cout<<"ASM CheckBlocks ERROR，count:\t"<<count<<endl;
					if(count++>5 && count<30){
						fdsleep(5);
					}else if(count>=30){
						fdsleep(120);
					}
					goto ASM_Check;
				}
			} 
		} 
		while (stat == -1 && (errno==EBUSY || errno==EINTR || errno==EIO) && retry++<RETYE_TIME );

		if(stat == -1){
			RetVal = _ERROR_ASM_READ;
			break;
		} else {
			RetVal = readsize;
		}

		BYTE* p_block_head = TheMem;
		for (int j = 1; j <= BlockNum; j++) {			
			if (ORACLE_VERSION > _Oracle_9i 
				&& do_checksum(m_pASMFile->BlockSize, p_block_head) != 0) 
			{
				if (j>m_BlockRemain){
					bChuckSum = true;
				}else{
					cout<<"do_checksum error:BlockSize\t"<<m_pASMFile->BlockSize<<endl
						<<"j "<<j<<" m_BlockRemain "<<m_BlockRemain<<endl;
					RetVal = _ERROR_CHECKSUM;
					bChuckSum = false;
				}				
				break;
			}	
			p_block_head += m_pASMFile->BlockSize;
		}
		if (bChuckSum == false) {
			continue;
		}
		break;
	}
	if(readsize == RetVal && bChuckSum == true)
		return _ASM_OK;
	return RetVal;
}

int ASM::MEM2ASM()
{
	return _ASM_OK;
}

// 根据DiskNum 从全局变量中得到DISK结构体信息
int ASM::GetDisk(int DiskNum)
{
	DISK* pDisk = NULL;
	int FdMem = 0;
	if(DiskNum<0){
		cout<<"Error DiskNum"<<endl;
		return _ERROR_DISK;
	}

	int size = m_pDisks->size();
	if(DiskNum < m_pDisks->size()){
		if(m_pDisks->at(DiskNum).DiskNum == DiskNum){ // 下标索引刚好与磁盘号对应，一般来说都是这样的的情况
			pDisk = &(m_pDisks->at(DiskNum));
		}
	}

	if(pDisk == NULL){
		for(int i=0;i<size;i++){// 下标索引与磁盘号不对应 遍历这个容器
			if(m_pDisks->at(i).DiskNum == DiskNum){ 
				pDisk = &(m_pDisks->at(DiskNum));
				break;
			}
		}
	}

	if(NULL == pDisk){
		cout<<"GetDisk Error"<<endl;
		return _ERROR_DISK;
	}

	int *pFD = NULL;
	if(m_pASMFile->bLogfile == true){
		pFD = &(pDisk->Disklogfd);
	}else{
		pFD = &(pDisk->DiskFd);
	}

	BYTE8 curpos = mylseek(*pFD,0,SEEK_CUR);		// 用来测试文件描述符是否有效

	if(curpos == -1 || *pFD == 0){
		FdMem = myopen(pDisk->Path,MYOPEN_READ);
		if(FdMem > 2){// 0,1,2被系统占掉的
			*pFD = FdMem;
		}else{
			cout<<"Open "<<pDisk->Path<<" Error\n"<<endl;
			return _ERROR_OPEN;
		}
	}else{
		FdMem = *pFD;
	}

	return FdMem;
}

int ASM::CheckMem(BYTE *buf,DWORD StartIndex,int BlockNum,DWORD BlockSize,bool bFirst)
{
	int RetVal;
	if (BlockNum == 0){
		RetVal = CheckAUSequence(buf,StartIndex,BlockSize,bFirst);
	}else{
		RetVal = CheckBlocks(buf,StartIndex,BlockNum,BlockSize);
	}
	return RetVal;	
}

int ASM::CheckBlocks(BYTE *buf,int StartBlock,int BlockNum,DWORD BlockSize)
{
	int RetVal = _ASM_OK;
	DWORD BlockIndex = -1;
	for(int i=0;i<BlockNum;i++){
		memcpy(&BlockIndex,buf+i*BlockSize+ASM_BLOCK_NUM_OFFSET,sizeof(BlockIndex));
		BlockIndex = BlockIndex & ASM_BLOCK_BIT ;
		int BlockCMP = (StartBlock+i) & ASM_BLOCK_BIT ; //只比较最后_BLOCK_BIT位，大文件表空间使用32位的
		if(BlockIndex != BlockCMP){
			cout<<"ASM---CheckBlocks Error,Get Block Index:\t"<<BlockIndex<<"\tIt Should be:\t"<<(StartBlock+i)<<endl;
			cout<<"ASM---StartBlock:\t"<<StartBlock<<endl;
			cout<<"ASM---BlockNum:\t"<<BlockNum<<endl;
			RetVal = ASM_ERROR_CHECK_BLOCKINDEX;
			break;
		}
	}
	return RetVal;
}

int ASM::CheckAUSequence(BYTE *buf,DWORD Sequence,DWORD BlockSize,bool bFirst)
{
	int RetVal = _ASM_OK;
	DWORD offset = ASM_SEQ_OFFSET;//+BlockSize
	if(bFirst == true){
		offset += BlockSize;
	}
	DWORD LSN = 0;
	memcpy(&LSN,buf+offset,sizeof(LSN));
	if(LSN != Sequence){
		cout<<"LSN != Sequence:\t"<<LSN<<"\t"<<Sequence<<endl;
		RetVal = ASM_ERROR_CHECK_BLOCKSEQ;
	}
	return RetVal;
}

int ASM::GetAuSize()
{
	if (m_pGroup){
		return m_pGroup->AU_Size;
	}else{
		return 0;
	}	
}

int ASM::PrintDebug()
{
	cout<<"ASM 相关信息\n";
	cout<<"GroupNum:\t"<<m_pASMFile->GroupNum<<endl;
	cout<<"ASMFileIndex:\t"<<m_pASMFile->ASMFileIndex<<endl;
	cout<<"DataBlockSize:\t"<<m_pASMFile->BlockSize<<endl;
	cout<<"Blocks:\t"<<m_pASMFile->Blocks<<endl;
}

#endif
