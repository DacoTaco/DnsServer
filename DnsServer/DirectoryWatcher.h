#ifndef _DIRECTORYWATCHER_H_
#define _DIRECTORYWATCHER_H_

#ifdef WIN32

#include <windows.h>

#endif


//DEFINES
//------------------
#define HANDLE_SIZE 1


extern bool bChangedDir;

void ReloadDirectory();
HANDLE* WatchDirectory(void);

#endif