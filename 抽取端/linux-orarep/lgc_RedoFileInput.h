#ifndef LGC_REDOFILEINPUT_H
#define LGC_REDOFILEINPUT_H

#include <iostream>
#include <list>
#include <string>
#include <sstream>

#include "lgc_Defines.h"
#include "lgc_Structure.h"
#include "lgc_RedoFile.h"

using namespace std;

class LGC_RedoFile;
class OciQuery;
class LGC_RedoFileInfoList;

class LGC_RedoFileInput
{
private:
	//member variables
	LGC_RedoFileInfoList* m_pRedoFileInfoList;
	OciQuery* m_pQuery;

private:
	//constructor and desctructor
	LGC_RedoFileInput(LGC_RedoFileInfoList* pRedoFileInfoList,OciQuery* pQuery);
public:	
	~LGC_RedoFileInput();

public:
	//public member functions
	int getNextRedoFile(LGC_RedoFile** ppRedoFile);

private:
	//private member functions

public:
	//static functions
	static LGC_RedoFileInput* createRedoFileInput(int threadId, BYTE8 startSCN, OciQuery* pQuery);
};

//....................................
//tool classes for LGC_RedoFileInput
//....................................

class LGC_RedoFileInfoList
{
	//LGC_RedoFileInput is LGC_RedoFileInfoList's
	//friend class
	friend class LGC_RedoFileInput;
private:
	//member variables
	int m_threadId;
	BYTE8 m_startSCN;
	OciQuery* m_pQuery;
	
	list<LGC_RedoFileInfo> m_archiveFile_list;
	list<LGC_RedoFileInfo> m_onlineFile_list;
	
	DWORD m_curSequence;
private:
	//constructor and desctructor
	LGC_RedoFileInfoList(int threadId, BYTE8 startSCN, OciQuery* pQuery);
public:
	~LGC_RedoFileInfoList();

public:
	//public member functions
	int getNextRedoFileInfo(LGC_RedoFileInfo* pRedoFileInfo);
	
public:
	int redoFileInfo_pushBack(const LGC_RedoFileInfo& archiveFileInfo);

private:
	//private member functions
	int init();
	int loadLeftRedoFileInfo();
	
private:
	//static functions
	static LGC_RedoFileInfoList* createRedoFileInfoList(int threadId, BYTE8 startSCN, OciQuery* pQuery);
};
#endif
