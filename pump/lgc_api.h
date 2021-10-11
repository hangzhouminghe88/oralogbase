#ifndef LGC_API
#define LGC_API

#include <stdio.h>
#include <string.h>
#include <list>

#define lgc_logMsg(level,fmd,...) lgc_logMsg_s(level,__FILE__, __LINE__,fmd, ##__VA_ARGS__)
#define lgc_errMsg(fmd,...) lgc_errMsg_s(__FILE__, __LINE__,fmd, ##__VA_ARGS__)
#define lgc_scnMsg(fmd, ...) lgc_scnMsg_s(__FILE__, __LINE__, fmd, ##__VA_ARGS__)

int lgc_logMsg_s(int level, char *file, long line, char *fmd,...);
int lgc_errMsg_s(char *file, long line, char *fmd,...);
int lgc_scnMsg_s(char* file, long line, char *fmd, ...);

void byteSwap(void* buf, const int size);
#endif



