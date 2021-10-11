

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "lgc_network.h"
#include "lgc_sendMedia.h"
#include "lgc_api.h"
#include "lgc_param.h"

LGC_SendMedia::LGC_SendMedia(const char* mediaFileName, LGC_NetWork* pSocket)
{
	m_pSocket = pSocket;
	m_fileName = new char[strlen(mediaFileName)+1];
	if(!m_fileName){
		lgc_errMsg("new error:%s\n", strerror(errno));
		exit(1);
	}
	strcpy(m_fileName,mediaFileName);

	return;
}

LGC_SendMedia::LGC_SendMedia(LGC_NetWork* pSocket)
{
	m_pSocket = pSocket;
	m_fileName = NULL;

	return;
}



LGC_SendMedia::~LGC_SendMedia()
{
	if(m_fileName){
		delete[] m_fileName;
		m_fileName = NULL;
	}

	return;
}


void LGC_SendMedia::setSocket(LGC_NetWork* pSocket)
{
	m_pSocket = pSocket;
	return;
}
void LGC_SendMedia::setFileName(const char* mediaFileName)
{
	if(m_fileName){
		delete[] m_fileName;
		m_fileName = NULL;
	}

	m_fileName = new char[strlen(mediaFileName)+1];
	if(!m_fileName){
		lgc_errMsg("new error:%s\n", strerror(errno));
		exit(1);
	}
	strcpy(m_fileName,mediaFileName);
	return;
}

bool LGC_SendMedia::sendMediaFile()
{
	bool bRet = true;
	FILE *mediaFp = NULL;
	
	char buf[512] = {0};
	const int bufSize=512;
	int bytesReaded = 0;
	int bytesSended = 0;
	bool isEOF = false;

	//open media file 
	mediaFp = fopen(m_fileName,"rb");
	if(mediaFp == NULL){
		lgc_errMsg("open file failed:fileName=%s errStr=%s\n", m_fileName,strerror(errno));
		return false;
	}
	

	//send media file header info
	LGC_MediaFileHeader mediaFileHeader;
	strcpy(mediaFileHeader.fileName, m_fileName);
	mediaFileHeader.fileNameLen = strlen(m_fileName);
	fseek(mediaFp, 0, SEEK_END);
	mediaFileHeader.fileSize = ftell(mediaFp);
	fseek(mediaFp,0,SEEK_SET);
	bRet = this->sendMediaFileHeader(&mediaFileHeader);
	if(!bRet){
		if(mediaFp){
			fclose(mediaFp);
			mediaFp = NULL;
		}
		lgc_errMsg("sendMediaFileHeader failed \n");
		return false;
	}

	//recv response from server
	char *successStr = "SUCCESS";
	bytesReaded = m_pSocket->recv(buf,strlen(successStr));
	if(bytesReaded != strlen(successStr)  || strncmp(successStr,buf,bytesReaded) != 0){
		lgc_errMsg("recv success str failed:bytesReaded=%d fileName=%s\n",bytesReaded, m_fileName);
		bRet = false;
		goto errOut;
	}

	//send mediaFile contents
	do{
		bytesReaded = fread(buf,1,bufSize,mediaFp);
		if(bytesReaded < 0){//read failed
			lgc_errMsg("fread failed:%s \n", strerror(errno));
			bRet = false;
			goto errOut;
		}else if(bytesReaded < bufSize ){//read EOF
			isEOF = true;
		}

		bytesSended =m_pSocket->send(buf,bytesReaded);
		if(bytesSended != bytesReaded){//send failed
			lgc_errMsg("send failed\n");
			bRet = false;
			goto errOut;
		}

	}while(!isEOF);
	//have sended mediaFile and successfully

	//recv response from server
	successStr = "SUCCESS";
	bytesReaded = m_pSocket->recv(buf,strlen(successStr));
	if(bytesReaded != strlen(successStr)  || strncmp(successStr,buf,bytesReaded) != 0){
		lgc_errMsg("recv success str failed:bytesReaded=%d fileName=%s\n",bytesReaded, m_fileName);
		bRet = false;
		goto errOut;
	}

	bRet = true;

errOut:
	if(mediaFp){
		fclose(mediaFp);
	}
	return bRet;
}


bool LGC_SendMedia::sendMediaFileHeader(const LGC_MediaFileHeader* pMediaFileHeader)
{
	int bytesToSend = sizeof(LGC_MediaFileHeader);
	int bytesSended = 0;

	bytesSended = m_pSocket->send((char*)pMediaFileHeader,bytesToSend);
	if(bytesSended != bytesToSend){//send failed
		lgc_errMsg("sendMediaFileHeader failed\n");
		return false;
	}
	return true;
}


///////////////////////////// LGC_ReceiveMedia //////////////////////////////////
LGC_ReceiveMedia::LGC_ReceiveMedia(LGC_NetWork* pSocket)
{
	m_fileName = NULL;	
	m_pSocket = pSocket;

	return;
}

LGC_ReceiveMedia::~LGC_ReceiveMedia()
{
	if(m_fileName){
		delete[] m_fileName;
		m_fileName = NULL;
	}
	return;
}

bool LGC_ReceiveMedia::recvMediaFile()
{
	bool bRet = true;

	FILE* mediaFp = NULL;
	
	char buf[512] = {0};
	const int bufSize = 512;
	int bytesToRecv = 0;
	int bytesReceived = 0;
	int bytesWrited   = 0;
	

	//recv mediaFileHeader info
	LGC_MediaFileHeader mediaFileHeader;
	bRet = this->recvMediaFileHeader(&mediaFileHeader);
	if(!bRet){//recv mediaFileHader info failed
		lgc_errMsg("recv mediafileheader info failed\n");
		return false;
	}
	char* successStr="SUCCESS";
	int bytesSended;
	bytesSended = m_pSocket->send(successStr, strlen(successStr));
	if(bytesSended != strlen(successStr)){
		lgc_errMsg("send success str failed:fileName=%s bytesSended=%d\n",mediaFileHeader.fileName, bytesSended);
		return false;
	}


	//open media file
	char *p = NULL;
	char fileName[126] = {0};
	const char* mediaFileDir = LGC_Param::getInstance()->getMediaFileDir();

	if(mediaFileDir[strlen(mediaFileDir)-1] == '/'){
		strcpy(fileName,mediaFileDir);
	}else{
		sprintf(fileName,"%s/",mediaFileDir);
	}

	if( (p=strrchr(mediaFileHeader.fileName,'/')) == NULL){
		lgc_errMsg("fileName invalid:%s\n", mediaFileHeader.fileName);
		return false;
	}
	p++;
	strcpy(fileName+strlen(fileName), p);
	
	p = strrchr(fileName, 'E');
	if(p == NULL){
		lgc_errMsg("fileName invalid:%s\n", fileName);
		return false;
	}
	*p = 'S';

	mediaFp = fopen(fileName, "wb+");
	if(!mediaFp){
		lgc_errMsg("fopen failed:fileName=%s errStr=%s\n", fileName,strerror(errno));
		return false;
	}
	
	int bytesTotalRecved = 0;
	int fileSize = mediaFileHeader.fileSize;

	bool isEOF = false;
	//receive all media file contents
	do{
		if(bytesTotalRecved >= fileSize ){//file all recved
			isEOF = true;
			break;
		}
		
		bytesToRecv = (fileSize - bytesTotalRecved) > bufSize? bufSize:(fileSize - bytesTotalRecved);

		bytesReceived = m_pSocket->recv(buf,bufSize);
		if(bytesReceived < 0){//receive failed
			lgc_errMsg("recv media file failed \n");
			bRet = false;
			goto errOut;
		}else if(bytesReceived == 0){//receive EOF
			isEOF = true;
			break;
		}

		bytesWrited = fwrite(buf,1,bytesReceived,mediaFp);
		if(bytesWrited != bytesReceived){//write failed
			lgc_errMsg("fwrite error\n");
			bRet = false;
			goto errOut;
		}

		bytesTotalRecved += bytesReceived;

	}while(!isEOF);
	if(bytesTotalRecved != fileSize){//recved bytes invalid
		lgc_errMsg("recved byte invalid \n");
		bRet = false;
		goto errOut;
	}
	if(mediaFp != NULL){
		fclose(mediaFp);
		mediaFp = NULL;
	}
	
	if( rename(fileName,mediaFileHeader.fileName) ){
		lgc_errMsg("rename failed:oldName=%s new name=%s\n", fileName, mediaFileHeader.fileName);
		bRet = false;
		goto errOut;

	}
	strcpy(fileName, mediaFileHeader.fileName);

	//have received all mediaFile contents and successfully
	successStr="SUCCESS";
	bytesSended = m_pSocket->send(successStr, strlen(successStr));
	if(bytesSended != strlen(successStr)){
		lgc_errMsg("send success str failed:fileName=%s bytesSended=%d\n",mediaFileHeader.fileName, bytesSended);
		bRet= false;
		goto errOut;
	}

	fprintf(stdout, "recved: fileName=%s\n", fileName);

	bRet = true;
errOut:
	if(mediaFp != NULL){
		fclose(mediaFp);
		mediaFp = NULL;
	}
	return bRet;
	
}


bool LGC_ReceiveMedia::recvMediaFileHeader(LGC_MediaFileHeader* pMediaFileHeader)
{
	int bytesToRecv = sizeof(LGC_MediaFileHeader);
	int bytesRecved = 0;

	bytesRecved = m_pSocket->recv((char*)pMediaFileHeader, bytesToRecv);
	if(bytesRecved != bytesToRecv){//recv failed
		lgc_errMsg("recv mediaFileHeader failed: bytesRecved=%d bytesToRecv=%d\n", bytesRecved, bytesToRecv);
		return false;
	}
	pMediaFileHeader->convertEndian();
	
	if(pMediaFileHeader->fileNameLen > 126){
		lgc_errMsg("media file name len invalid:pMediaFileHeader->fileNameLen=%d fileName=%s fileSize=%d\n",
			pMediaFileHeader->fileNameLen, pMediaFileHeader->fileName,pMediaFileHeader->fileSize);
		return false;
	}

	pMediaFileHeader->fileName[pMediaFileHeader->fileNameLen] = '\0';

	return true;
}

