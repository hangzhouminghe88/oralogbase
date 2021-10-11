#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <arpa/inet.h>

#include "lgc_network.h"
#include "lgc_param.h"
#include "lgc_sendMedia.h"
#include "lgc_fileList.h"
#include "lgc_ckpt.h"
#include "lgc_api.h"


using namespace std;

int network_main(int args, char** argv);
int param_main(int args, char** argv);

int recvMediaFile_main(int args, char** argv);
int sendMediaFile_main(int args, char** argv);
int listMediaFile_main(int argc, char** argv);
int sendAllMediaFile_main(int argc, char** argv);
int recvAllMediaFile_main(int argc, char** argv);

int main(int argc, char** argv)
{
	//recvMediaFile_main(argc,argv);
	//sendMediaFile_main(argc,argv);
	//listMediaFile_main(argc,argv);
	sendAllMediaFile_main(argc, argv);
//	recvAllMediaFile_main(argc, argv);

	return 0;
}

int recvAllMediaFile_main(int argc, char** argv)
{
	bool bFuncRet = true;
	int iRet = 0;
	
	//load param file
	bFuncRet = LGC_Param::getInstance()->loadParaFile("./conf/pump.ini");
	if(!bFuncRet){
		lgc_errMsg("loadParamFile failed\n");
		exit(1);
	}
	//get listen port
	int listenPort = LGC_Param::getInstance()->getListenPort();
	
	LGC_NetWork socket_listen;
	
	//start listen
	bFuncRet = socket_listen.listen(listenPort);
	if(!bFuncRet){
		lgc_errMsg("listen failed\n");
		return -1;	
	}	

	LGC_NetWork* pAcceptSocket = NULL;

	// accept connect from client
	pAcceptSocket = socket_listen.accept();
	if(pAcceptSocket == NULL){
		lgc_errMsg("accept failed\n");
		return -1;
	}

	LGC_ReceiveMedia recvMedia(pAcceptSocket);
	
	//recv mediafile from client
	while(true){
		bFuncRet = recvMedia.recvMediaFile();
		if(!bFuncRet){
			lgc_errMsg("recvMediaFile failed \n");
			return -1;
		}
	}

}

int sendAllMediaFile_main(int argc, char** argv)
{
	bool b;
	bool bFuncRet = true;


	
	//load param file
	b = LGC_Param::getInstance()->loadParaFile("./conf/pump.ini");
	if(!b){
		lgc_errMsg("load param file failed\n");
		return -1;
	}
	//get MediaFileDir 
	const char* mediaDir = LGC_Param::getInstance()->getMediaFileDir();
	//get Target listen port
	int port = LGC_Param::getInstance()->getTargetPort();
	//get target host ip
	const char* host = LGC_Param::getInstance()->getTargetHost();	
	
	//init network
	LGC_NetWork netWork;
	
	//connect to target host
	bFuncRet = netWork.connect(host, port);
	if(!bFuncRet){//connect failed
		lgc_errMsg("connect failed:host=%s port=%u\n", host, port);
		return -1;
	}

	LGC_SendMedia sendMedia(&netWork);
	

	//list media file
	LGC_FileList fileList;
	LGC_MediaFileInfo* pMediaFileInfo = NULL;
	
	string filePath;

	while(true){
		//load all media file info
		bFuncRet = fileList.loadAllMediaFile();
		if(!bFuncRet){
			lgc_errMsg("loadAllMediaFile failed \n");
			return -1;
		}
		//get next media file info 
		while( (pMediaFileInfo = fileList.getNextMediaFile()) != NULL)
		{
			fprintf(stdout, "main:%s\n", pMediaFileInfo->fileName);
			
			
			filePath.clear();
			filePath +=mediaDir;
			filePath += mediaDir[strlen(mediaDir)-1] == '/'? "":"/";
			filePath +=pMediaFileInfo->fileName;
			sendMedia.setFileName(filePath.c_str());
			//send the media file to target host
			bFuncRet=sendMedia.sendMediaFile();
			if(!bFuncRet){
				lgc_errMsg("sendMediaFile failed:%s\n", filePath.data());
				return -1;
			}
			
			fprintf(stdout, "sended:fileName=%s\n",filePath.c_str());
			
			//remove the media file in source because it have been successfully received by target host
			if(remove(filePath.data())){
				lgc_errMsg("remove file failed:%s \n", filePath.data());
				return -1;
			}
			
			//update lastCommitSCN in ckpt file 
			//LGC_CKPT::getInstance()->setLastCommitSCN(pMediaFileInfo->getSCN());
		}
		
		// remove all sended file info in list of media file 
		fileList.removeAllSendedFile();

		sleep(1);
	}

	return 0;
}




int listMediaFile_main(int argc, char** argv)
{
	bool b;

	LGC_FileList fileList;

	b = LGC_Param::getInstance()->loadParaFile("./conf/pump.ini");
	if(!b){
		fprintf(stderr, "load para file failed:%s\n", "./conf/pump.ini");
		return -1;
	}


	bool bFuncRet = false;
	int port = 9509;
	char* host = "10.10.10.180";
	LGC_NetWork netWork;
	
	bFuncRet = netWork.connect(host, port);
	if(!bFuncRet){//connect failed
		return -1;
	}


	fileList.loadAllMediaFile();

	//fileList.view();

	LGC_MediaFileInfo* pMediaFileInfo = NULL;

	while( (pMediaFileInfo = fileList.getNextMediaFile()) != NULL)
	{
		fprintf(stdout, "%s\n", pMediaFileInfo->fileName);
//		LGC_CKPT::getInstance()->setLastCommitSCN(pMediaFileInfo->getSCN());
	}

	fileList.removeAllSendedFile();


	fileList.loadAllMediaFile();

	//fileList.view();

	while( (pMediaFileInfo = fileList.getNextMediaFile()) != NULL)
	{
		fprintf(stdout, "%s\n", pMediaFileInfo->fileName);
		//LGC_CKPT::getInstance()->setLastCommitSCN(pMediaFileInfo->getSCN());
	}

	fileList.removeAllSendedFile();


	return 0;
}

int sendMediaFile_main(int args, char** argv)
{
	
	bool bFuncRet = false;
	
	int port = 9509;
	char* host = "10.10.10.180";
	LGC_NetWork netWork;
	
	bFuncRet = netWork.connect(host, port);
	if(!bFuncRet){//connect failed
		return -1;
	}

	char* fileName = "/home/tw/lgc/mediaFile/1_7_937_2198291_E.lgc";

	LGC_SendMedia sendMedia(fileName,&netWork);

	bFuncRet = sendMedia.sendMediaFile();
	if(!bFuncRet){
		return -1;
	}

	return 0;
}

int recvMediaFile_main(int args, char** argv)
{
	int iRet = 0;
	bool bFuncRet = false;

	int listenPort = 9509;
	LGC_NetWork socket_listen;
	
	bFuncRet = socket_listen.listen(listenPort);
	if(!bFuncRet){
		fprintf(stderr, "listen failed\n");
		return -1;	
	}	

	LGC_NetWork* pAcceptSocket = NULL;
	pAcceptSocket = socket_listen.accept();
	if(pAcceptSocket == NULL){
		return -1;
	}

	LGC_ReceiveMedia recvMedia(pAcceptSocket);

	recvMedia.recvMediaFile();

	return 0;
}

int param_main(char** argv, int args)
{
	int iRet = 0;
	bool b = true;

	LGC_Param* pParam = LGC_Param::getInstance();
	
	const char* paramFileName="./conf/pump.ini";
	b = pParam->loadParaFile(paramFileName);
	if( !b ){
		fprintf(stderr,"loadFile failed:%s\n",paramFileName);
		return -1;
	}

	const char* mediaDir = pParam->getMediaFileDir();
	fprintf(stdout, "mediaDir:%s\n", mediaDir);

	return 0;
}

int network_main(char** argv, int args)
{
	int iRet = 0;
	
	int port = 9509;
	char* host = "10.10.10.180";
	LGC_NetWork netWork;

	if(netWork.connect(host,port) == false){
		fprintf(stderr,"connect failed \n");
		return -1;
	}

	char buf[512] = {0};
	int bufSize = 512;

	int bytesRecved = 0;
	int bytesSended = 0;

	bytesRecved = netWork.recv(buf,bufSize);
	if(bytesRecved < 0){
		fprintf(stderr,"recv failed \n");
		return -1;
	}

	buf[bytesRecved] = 0;
	fprintf(stdout,"recv:%s\n", buf);
	fflush(stdout);
	
	strcpy(buf,"hello server");
	int len = strlen(buf);
	bytesSended = netWork.send(buf,len);
	if(bytesSended != len){
		fprintf(stderr, "send failed\n" );
		return -1;
	}

	return 0;
}
/*
int main(char** argv, int args)
{
	int iRet = 0;

	int port = 9509;
	char* host = "10.10.10.180";
	int sockFd = 0;

	sockFd = socket(AF_INET, SOCK_STREAM,0);
	if(sockFd<0){//creat socket fd failed
		fprintf(stderr, "create socket fd failed:%s\n", strerror(errno));
		return -1;
	} 

	struct sockaddr_in remote_addr;
	socklen_t sockAddrLen = sizeof(struct sockaddr_in);
	memset(&remote_addr, 0, sizeof(struct sockaddr_in));

	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port   = htons(port);
	inet_aton(host, &remote_addr.sin_addr);	

	
	iRet = connect(sockFd,(struct sockaddr*)&remote_addr,sockAddrLen);		
	if(iRet < 0){
		fprintf(stderr,"connect failed:%s\n",strerror(errno));
		return -1;
	}

	
	int bytesSended = 0;
	int bytesRecved = 0;
	char buf[512] = "hello tw";
	int bufSize = 512;
	int len =0;
	
	bytesRecved = recv(sockFd, buf,bufSize,0 );
	if(bytesRecved < 0){
		fprintf(stderr,"recv failed:%s\n", strerror(errno));
		return -1;
	}else if(bytesRecved == 0){
		fprintf(stdout, "no data received\n");
		fflush(stdout);
	}
	buf[bytesRecved]=0;
	fprintf(stdout,"recv:%s\n",buf);
	
	strcpy(buf,"hello tw");
	len= strlen(buf);
	bytesSended = send(sockFd,buf, len,0 );
	if(bytesSended < 0){
		fprintf(stderr, "send failed:%s\n", strerror(errno));
		return -1;
	}


	return 0;
}

*/
