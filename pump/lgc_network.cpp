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
#include "lgc_api.h"

LGC_NetWork::LGC_NetWork()
{
	m_host = NULL;
	m_port = 0;
	m_fd = 0;
	m_listenFd = 0;

	return;
}

LGC_NetWork::LGC_NetWork(int acceptFd)
{
	m_host = NULL;
	m_port = 0;
	m_fd = acceptFd;
	m_listenFd = 0;	
}

LGC_NetWork::~LGC_NetWork()
{
	if(m_host){
		delete[] m_host;
		m_host = NULL;
	}

	if(m_fd > 0){
		close(m_fd);
		m_fd = -1;
	}
	if(m_listenFd > 0){
		close(m_listenFd);
		m_listenFd = -1;
	}

	return;
}

bool LGC_NetWork::listen(int port)
{
	bool bRet = true;
	int iFuncRet = 0;
	const int listenQueueLen = 10;

	if(m_listenFd > 0){
		close(m_listenFd);
		m_listenFd = -1;
	}

	this->setListenInfo(port);

	//create listen socket 
	m_listenFd = ::socket(AF_INET, SOCK_STREAM,0);
	if(m_listenFd < 0){
		lgc_errMsg("create socket failed:%s\n", strerror(errno));
		bRet = false;
		return bRet;
	}

	struct sockaddr_in serverAddr;
	const socklen_t addrLen = sizeof(struct sockaddr_in);
	
	memset(&serverAddr,0,addrLen);
	
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port   = htons(port);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	//bind the listen socket to one server address 
	iFuncRet = ::bind(m_listenFd, (struct sockaddr*)&serverAddr, addrLen);
	if(iFuncRet < 0){
		lgc_errMsg("bind failed:%s\n", strerror(errno));
		bRet = false;
		goto errOut;
	}
	
	//start lieten
	iFuncRet = ::listen(m_listenFd,listenQueueLen);
	if(iFuncRet < 0){
		lgc_errMsg("listen failed:%s\n", strerror(errno));
		bRet = false;
		goto errOut;
	}
	
	//success
	bRet = true;
errOut:
	return bRet;
}


LGC_NetWork* LGC_NetWork::accept()
{

	struct sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(struct sockaddr_in);
	memset(&clientAddr, 0, addrLen);
	
	int acceptFd = 0;

	acceptFd = ::accept(m_listenFd,(struct sockaddr*)&clientAddr,&addrLen );	
	if(acceptFd < 0){
		lgc_errMsg("accept failed:%s\n", strerror(errno));
		return NULL;
	}	
	
	LGC_NetWork* pAcceptNetWork = new LGC_NetWork(acceptFd);
	if(pAcceptNetWork == NULL){
		lgc_errMsg("new failed:%s\n", strerror(errno));
		return NULL;
	}

	//success
errOut:
	return pAcceptNetWork;
}

/*
bool LGC_NetWork::accept()
{
	bool bRet = true;
	int iFuncRet = 0;

	struct sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(struct sockaddr_in);
	memset(&clientAddr, 0, addrLen);

	iFuncRet = ::accept(m_listenFd,(struct sockaddr*)&clientAddr,&addrLen );	
	if(iFuncRet < 0){
		fprintf(stderr, "accept failed:%s\n", strerror(errno));
		bRet = false;
		goto errOut;	
	}	
	
	//success
	bRet = true;
errOut:
	return bRet;
}
*/

bool LGC_NetWork::connect(const char* host, int port)
{
	bool bRet = true;
	int iFuncRet = 0;

	if(m_fd > 0){
		close(m_fd);
		m_fd = -1;
	}	
	
	this->setConnectInfo(host,port);

	//create connect socket
	m_fd = ::socket(AF_INET, SOCK_STREAM,0);
	if(m_fd < 0){
		lgc_errMsg("create socket failed:%s\n", strerror(errno));
		return false;
	}

	struct sockaddr_in serverAddr;
	socklen_t addrLen = sizeof(struct sockaddr_in);
	
	memset(&serverAddr, 0, sizeof(struct sockaddr_in));
	
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_aton(host, &serverAddr.sin_addr);

	//connect to target host 
	iFuncRet = ::connect(m_fd,(struct sockaddr*)&serverAddr,addrLen);
	if(iFuncRet < 0){
		lgc_errMsg("connect failed:%s\n", strerror(errno));
		bRet = false;
		goto errOut;
	}

	//success
	bRet = true;	
errOut:
	return bRet;	
}

int LGC_NetWork::send(char* buf, int size)
{
	int bytesSended = 0;
	
	bytesSended = ::send(m_fd,buf,size,0);
	if(bytesSended < 0){
		lgc_errMsg("send failed:%s\n", strerror(errno));
		return -1;
	}
	
	return bytesSended;
}

int LGC_NetWork::recv(char* buf, int size)
{
	int bytesRecved = 0;

	bytesRecved = ::recv(m_fd, buf,size,0);
	if(bytesRecved < 0){
		lgc_errMsg("recv failed:%s\n",strerror(errno));
		return -1;
	}

	return bytesRecved;
}


//private
void LGC_NetWork::setConnectInfo(const char* serverHost, int port)
{
	return;
}

void LGC_NetWork::setListenInfo(int port)
{
	return;
}
