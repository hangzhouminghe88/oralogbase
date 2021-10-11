
#include "MyLog.h"

void MyLogMes(const char *str, ...)
{
	va_list ap;
	
	FILE *fh = NULL;
	
#ifdef __STDC_WANT_SECURE_LIB__
	if(fopen_s(&fh, COMMLIB_DBG_FILE, "a") == 0)
#else
		if(fh = fopen(COMMLIB_DBG_FILE, "a"))
#endif
		{
			va_start(ap, str);
			vfprintf(fh, str, ap);
		//	fprintf(fh, "\n");
			va_end(ap);	
			fclose(fh);
			fh = NULL;
		}	
}

void MyLogMes2File(char *FileName,const char *str, ...)
{
	va_list ap;
	
	FILE *fh = NULL;
	
#ifdef __STDC_WANT_SECURE_LIB__
	if(fopen_s(&fh, FileName, "a") == 0)
#else
		if(fh = fopen(FileName, "a"))
#endif
		{
			va_start(ap, str);
			vfprintf(fh, str, ap);
			fprintf(fh, "\n");
			va_end(ap);	
			fclose(fh);
			fh = NULL;
		}	
		
}

