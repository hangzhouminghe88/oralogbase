#ifndef LGC_MEDIAFILEOUTPUT_H
#define LGC_MEDIAFILEOUTPUT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <list>
using namespace std;

#include "lgc_Defines.h"
#include "lgc_Structure.h"
#include "Mutex.h"


class LGC_MediaFileOutput
{
private:
	//member variables
	char* m_dirName;
	FILE* m_fp;

	static Mutex s_mutex;
	static LGC_MediaFileOutput* s_instance;

public:
	//constructor and desctructor
	LGC_MediaFileOutput();
	~LGC_MediaFileOutput();

public:
	//public member functions
	int writeLine(const char* line);
	void setMediaFileDir(const char* dirName);

private:
	int open();
    string getMediaFileName();

public:
	//static member functions
	static LGC_MediaFileOutput* getInstance();
};
#endif

