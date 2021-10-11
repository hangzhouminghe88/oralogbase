#ifndef LGC_RECORDHANDLER_H
#define LGC_RECORDHANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lgc_Defines.h"
#include "lgc_Structure.h"

class LGC_RedoRecord;

class LGC_RecordHandler
{
private:
	//member variables
	LGC_RedoRecord* m_pRedoRecord;
public:
	//constructor and desctructor
	LGC_RecordHandler(LGC_RedoRecord* pRedoRecord);
	~LGC_RecordHandler();

public:
	//public member functions
	int handle();
	
};
#endif
