#ifndef LGC_API
#define LGC_API

#include <stdio.h>
#include <string.h>
#include <list>

#include "Defines.h"
#include "tw_rwDataBlocks.h"
#include "OciQuery.h"
#include "tw_api.h"
#include "lgc_structure.h"


void getDataStr(WORD dataLen, char* pDataSrc, char* prtStr);

//#define lgc_logMsg(level,fmd,...) lgc_logMsg_s(level,__FILE__, __LINE__,fmd, ##__VA_ARGS__)
//#define lgc_errMsg(fmd,...) lgc_errMsg_s(__FILE__, __LINE__,fmd, ##__VA_ARGS__)
//#define lgc_scnMsg(fmd, ...) lgc_scnMsg_s(__FILE__, __LINE__, fmd, ##__VA_ARGS__)

#define lgc_check(expr) (expr)
#define lgc_logMsg(level,fmd,...) 
#define lgc_errMsg(fmd,...) lgc_errMsg_s(__FILE__, __LINE__,fmd, ##__VA_ARGS__)
#define lgc_scnMsg(fmd, ...) 

int lgc_logMsg_s(int level, char *file, long line, char *fmd,...);
int lgc_errMsg_s(char *file, long line, char *fmd,...);
int lgc_scnMsg_s(char* file, long line, char *fmd, ...);

#endif


