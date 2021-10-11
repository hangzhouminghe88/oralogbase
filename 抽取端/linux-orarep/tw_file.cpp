#include "tw_file.h"
#include "tw_api.h"


int tw_open(const char* fileName, int openFlag)
{
	int iRet=-1;
	
#ifdef WIN32
#else
	iRet=open(fileName,openFlag);
#endif
	return iRet;
}
void tw_close(int fd)
{
#ifdef WIN32
#else
	close(fd);
#endif
	return;
}
int tw_read(int fd, void* buf, int count)
{
	int iRet=-1;
#ifdef WIN32
#else
	iRet=read(fd,buf,count);
#endif
	return iRet;
}

int tw_write(int fd, void* buf, int count)
{
	int iRet=-1;
#ifdef WIN32
#else
	iRet=write(fd,buf,count);
	if(iRet!=count){
		printf("errInfo: %s \n",strerror(errno));
	}
#endif
	return iRet;
}

int tw_write_times(int fd, void* buf,int count)
{
	int iRet = -1;
	const int retryTimes = 3;
	int retry;
	int bytesWrited;

	retry = 0;
	do {
			if ((retry > 0 && errno == EBUSY)) {
				tw_sleep(1);
			}
			bytesWrited = tw_write(fd,buf,count);
	} while (bytesWrited == -1 && (errno == EBUSY || errno == EINTR || errno == EIO) && retry++ <retryTimes );

	iRet = bytesWrited;
errOut:
	return iRet;
}

int tw_read_times(int fd, void* buf, int count)
{
	int iRet = -1;
	const int retryTimes = 3;
	int retry;
	int bytesReaded;

	retry = 0;
	do {
			if ((retry > 0 && errno == EBUSY)) {
				tw_sleep(1);
			}
			bytesReaded= tw_read(fd,buf,count);
	} while (bytesReaded == -1 && (errno == EBUSY || errno == EINTR || errno == EIO) && retry++ <retryTimes );

	iRet = bytesReaded;
errOut:
	return iRet;
}

UBYTE8 tw_lseek(int fd, UBYTE8 offset, int whence)
{
	UBYTE8 ullRet=-1;
#ifdef WIN32
#else
	ullRet=lseek64(fd,offset,whence);
#endif
	return ullRet;
}

bool tw_fileIsCreated(const char* fileName)
{
	bool bRet = false;

	int flag = O_RDONLY|O_CREAT|O_EXCL;
	int fd = 0;

	fd = open(fileName, flag);
	if(fd == EEXIST){//file created
		bRet = true;
	}else{//file not created
		bRet = false;
	}

errOut:
	if(fd> 0){
		close(fd);
	}

	return bRet;
}

/*
bool tw_createFile(const char* fileName, mode_t mode)
{
	bool bRet = true;
	int fd = 0;

	int flag = O_RDONLY|O_CREAT;

	fd = open(fileName,flag,mode);
	if(fd > 0){// create file success
		bRet = true;
	}else{//create file failed
		bRet = false;
	}

	
errOut:

	if(fd>0){
		close(fd);
	}

	return bRet;
}
*/


/*
bool tw_createFile(const char* fileName, mode_t mode)
{
	bool bRet = true;
	FILE* fp = 0;

	int flag = O_RDONLY|O_CREAT;

	fp = fopen(fileName,"a+");
	if(fp ){// create file success
		bRet = true;
	}else{//create file failed
		bRet = false;
	}

	
errOut:

	if(fp){
		fclose(fp);
	}

	return bRet;
}
*/


bool tw_createFile(const char* fileName, mode_t mode)
{
	bool bRet = true;
	int fd = 0;


	int flag = O_RDONLY|O_CREAT;
	
	mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |S_IROTH;
	fd = open(fileName,flag,mode);
	if(fd > 0){// create file success
		bRet = true;
	}else{//create file failed
		bRet = false;
	}

//	fprintf(stdout, "tw_createFile: %s \n", fileName);
//	fflush(stdout);
errOut:

	if(fd>0){
		close(fd);
	}

	return bRet;
}


bool tw_removeFile(const char* fileName)
{
	bool bRet = true;
	int iFuncRet = 0;

	iFuncRet = unlink(fileName);

	if(iFuncRet){
		bRet = false;
	}else{
		bRet = true;
	}
	
	fprintf(stdout, "tw_removeFile: %s \n", fileName);
	fflush(stdout);

errOut:
	return bRet;
}

