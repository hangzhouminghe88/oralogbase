
#ifndef _TW_FILE_H
#define _TW_FILE_H

#include "Defines.h"

#include <stdio.h>
#include <stdlib.h>
#include <string>

#ifdef WIN32
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

#ifdef WIN32
	#define TW_OPEN_READ (_O_BINARY|_O_RDONLY)
	#define TW_OPEN_WRITE (_O_BINARY|_O_WRONLY)
	#define TW_OPEN_READ_WRITE (_O_BINARY|_O_RDWR)
#else

	#define TW_OPEN_READ O_RDONLY
//	#define TW_OPEN_READ O_RDONLY|O_DIRECT
	#define TW_OPEN_WRITE O_WRONLY
	#define TW_OPEN_READ_WRITE O_RDWR
	#define TW_OPEN_CREAT O_CREAT
#endif
int tw_open(const char* fileName, int openFlag);
void tw_close(int fd);
int tw_read(int fd, void*buf, int count);
int tw_write(int fd, void* buf, int count);
int tw_read_times(int fd, void* buf, int count);
int tw_write_times(int fd, void* buf, int count);
BYTE8 tw_lseek(int fd, BYTE8 offset, int whence);
bool tw_fileIsCreated(const char* fileName);
bool tw_createFile(const char* fileName, mode_t mode);
bool tw_removeFile(const char* fileName);
#endif

