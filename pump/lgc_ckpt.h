#ifndef _LGC_CKPT_H
#define _LGC_CKPT_H

#include "Defines.h"
#include "Mutex.h"
#include "SimpleIni.h"

class LGC_CKPT
{
private:
	BYTE8 m_last_commitSCN;
	
	static Mutex s_mutex;
	static LGC_CKPT* s_instance;

private:
	LGC_CKPT();
public:
	~LGC_CKPT();
	static LGC_CKPT* getInstance();

public:
	BYTE8 getLastCommitSCN();
	void  setLastCommitSCN(BYTE8 scn);

};

#endif

