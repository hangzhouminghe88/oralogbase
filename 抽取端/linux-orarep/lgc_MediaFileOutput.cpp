#include "lgc_MediaFileOutput.h"
#include "lgc_api.h"
#include <errno.h>

//.....................................
//class LGC_MediaFileOutput
//功能: 将数据输出到mediaFile中
//.....................................

Mutex LGC_MediaFileOutput::s_mutex;
LGC_MediaFileOutput* LGC_MediaFileOutput::s_instance = NULL;

//constructor and desctructor
/*
*构造函数
*/
LGC_MediaFileOutput::LGC_MediaFileOutput()
{
	m_dirName = NULL;
	m_fp = NULL;

	return;
}

/*
*析构函数
*/
LGC_MediaFileOutput::~LGC_MediaFileOutput()
{
	if(m_fp){
		fclose(m_fp);
		m_fp = NULL;
	}

	if(m_dirName){
		delete[] m_dirName;
		m_dirName = NULL;
	}
	return;
}

//public member functions
/*
*将字符串以行的形式输出
*/
int LGC_MediaFileOutput::writeLine(const char* line)
{
	if(this->open() < 0 ){
		lgc_errMsg("open failed \n");
		return -1;
	}

	fprintf(m_fp, "%s\n", line);
	fflush(m_fp);
	return 0;
}

void
LGC_MediaFileOutput::setMediaFileDir(const char* dirName)
{
	if(m_dirName != NULL || dirName == NULL ){
		lgc_errMsg("mediaFileDirName should null \n");
		exit(1);
	}

	const unsigned int dirNameLen = strlen(dirName);
	m_dirName = new char[dirNameLen+1];
	if(m_dirName == NULL){
		lgc_errMsg("new failed \n");
		exit(1);
	}
	memset(m_dirName, 0, dirNameLen+1);

	strcpy(m_dirName, dirName);

	return;
}

int 
LGC_MediaFileOutput::open()
{
	if(m_fp == NULL ){
		string mediaFileName = this->getMediaFileName();
		m_fp = fopen(mediaFileName.data(), "w+");
		if(m_fp == NULL){
			lgc_errMsg("fopen failed \n");
			return -1;
		}
	}
	
	//success
	return 0;
}

string 
LGC_MediaFileOutput::getMediaFileName()
{
	if(m_dirName == NULL ){
		lgc_errMsg("m_dirName should not null \n");
		exit(1);
	}

	string mediaFileName = m_dirName;
	mediaFileName		+= "/";
	mediaFileName       += "trail.txt";
	
	return mediaFileName;
}
//static member functions
/*
*获取单实例模式的唯一对象
*/
LGC_MediaFileOutput* LGC_MediaFileOutput::getInstance()
{
	if(s_instance == NULL){
		s_mutex.Lock();
		if(s_instance == NULL){
			s_instance = new LGC_MediaFileOutput;
			if(s_instance == NULL){
				lgc_errMsg("new failed \n");
				exit(1);
			}
		}
		s_mutex.Unlock();
	}

	return s_instance;
}

