#ifndef _LGC_FILELIST_H
#define _LGC_FILELIST_H

#include <stdio.h>
#include <stdlib.h>


#include <vector>
#include <list>

#include "Defines.h"

using namespace std;


struct LGC_MediaFileInfo
{
	char fileName[156];

	DWORD getSequence() const;

};
typedef struct LGC_MediaFileInfo LGC_MediaFileInfo;

class LGC_FileList
{
private:
	list<LGC_MediaFileInfo> m_file_container;
	list<LGC_MediaFileInfo>::iterator m_current_it;

public:
	LGC_FileList();
	~LGC_FileList();

public:
	bool loadAllMediaFile();
	LGC_MediaFileInfo* getNextMediaFile();
	bool removeAllSendedFile();


public:
	void view();
private:
	bool push_back(const LGC_MediaFileInfo& fileInfo);
	void sort();

};
#endif

