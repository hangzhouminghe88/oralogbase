#ifndef LGC_CHANGEHANDLER_H
#define LGC_CHANGEHANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lgc_Defines.h"
#include "lgc_Structure.h"

class LGC_Change;

class LGC_ChangeHandler
{
private:
	//member variables
	LGC_Change* m_pChange;
public:
	//constructor and desctructor
	LGC_ChangeHandler(LGC_Change* pChange);
	~LGC_ChangeHandler();

public:
	//public member functions
	int handle();

private:
	int  addChangeToTrsct();

public:
	//static member functions
	static bool isBeginTrsctChange(LGC_Change* pChange);
};
#endif
