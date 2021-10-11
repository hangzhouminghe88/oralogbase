#include "lgc_ckpt.h"

Mutex LGC_CKPT::s_mutex;
LGC_CKPT* LGC_CKPT::s_instance = NULL;

LGC_CKPT::LGC_CKPT()
{
	m_last_commitSCN = 0;
	return;
}

LGC_CKPT::~LGC_CKPT()
{
	return;
}

LGC_CKPT* LGC_CKPT::getInstance()
{
	if( s_instance == NULL){
		s_mutex.Lock();
		if(s_instance == NULL){
			s_instance = new LGC_CKPT();
			if(s_instance == NULL){
				fprintf(stderr, "new failed\n");
				exit(1);
			}
		}
		s_mutex.Unlock();
	}

	return s_instance;
}

BYTE8 LGC_CKPT::getLastCommitSCN()
{
	return m_last_commitSCN;
}
	
void  LGC_CKPT::setLastCommitSCN(BYTE8 scn)
{
	m_last_commitSCN = scn;
	return;
}


