#ifndef LGC_SENDMEDIA_H
#define LGC_SENDMEDIA_H


#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "lgc_api.h"
class LGC_NetWork;

struct LGC_MediaFileHeader
{
	char fileName[126];
	unsigned int fileNameLen;
	unsigned int fileSize;

	LGC_MediaFileHeader()
	{
		memset(fileName,0,126);
		fileNameLen = 0;
		fileSize = 0;
	}
	void convertEndian()
	{
//		::byteSwap(&fileNameLen, sizeof(fileNameLen));
//		::byteSwap(&fileSize, sizeof(fileSize));
	}
};

class LGC_SendMedia
{
private:
	char* m_fileName;
	LGC_NetWork* m_pSocket;
public:
	LGC_SendMedia(const char* mediaFileName, LGC_NetWork* pSocket);
	LGC_SendMedia(LGC_NetWork* pSocket);
	~LGC_SendMedia();

	void setFileName(const char* mediaFileName);
	void setSocket(LGC_NetWork* pSocket);

	bool sendMediaFile();

private:
	bool sendMediaFileHeader(const LGC_MediaFileHeader* pMediaFileHeader);
};

class LGC_ReceiveMedia
{
private:
	char* m_fileName;
	LGC_NetWork* m_pSocket;

public:
	LGC_ReceiveMedia(LGC_NetWork* pSocket);
	~LGC_ReceiveMedia();

	bool recvMediaFile();

private:
	bool recvMediaFileHeader(LGC_MediaFileHeader* pMediaFileHeader);
	
};

#endif
