
//Includes
//--------------
#include <stdlib.h>
#include <stdio.h>
#include <vector>

#include "DirectoryWatcher.h"


#ifdef WIN32

#include <windows.h>
#include <tchar.h>

typedef DWORD ulong;
typedef WORD ushort;
typedef UINT32 uint;

#endif



//Variables
//----------------
bool bChangedDir = true;
ulong dwWaitStatus;
HANDLE dwChangeHandles[HANDLE_SIZE]; 
LPWSTR strDirectory = NULL;


//pretty much stolen from msdn
// more @ https://msdn.microsoft.com/en-us/library/windows/desktop/aa365261%28v=vs.85%29.aspx

void RefreshDirectory(LPTSTR lpDir)
{
   // This is where you might place code to refresh your
   // directory listing, but not the subtree because it
   // would not be necessary.

   _tprintf(TEXT("Directory (%s) changed.\n"), lpDir);
   bChangedDir = true;
}
void ReloadDirectory()
{
	dwWaitStatus = WaitForMultipleObjects(HANDLE_SIZE, dwChangeHandles, 
			FALSE, 1); 
 
	//dwWaitStatus returns the number of which handle got activated.
	//0 == file access
	//1 == file rename
	switch (dwWaitStatus) 
	{ 
		case WAIT_OBJECT_0: 
			//File got accessed
			RefreshDirectory(strDirectory); 
			if ( FindNextChangeNotification(dwChangeHandles[0]) == FALSE )
			{
				printf("\n ERROR: FindNextChangeNotification function failed.\n");
				ExitProcess(GetLastError()); 
			}
			break; 
 
		case WAIT_OBJECT_0 + 1: 
			//file got renamed 
			RefreshDirectory(strDirectory);
			if (FindNextChangeNotification(dwChangeHandles[1]) == FALSE )
			{
				printf("\n ERROR: FindNextChangeNotification function failed.\n");
				ExitProcess(GetLastError()); 
			}
			break; 
 
		case WAIT_TIMEOUT:
			// A timeout occurred, this would happen if some value other 
			// than INFINITE is used in the Wait call and no changes occur.
			// In a single-threaded environment you might not want an
			// INFINITE wait.
 
			//printf("\nNo changes in the timeout period.\n");
			break;

		default: 
			printf("\n ERROR: Unhandled dwWaitStatus(%d).\n",dwWaitStatus);
			ExitProcess(GetLastError());
			break;
	}
}
HANDLE* WatchDirectory(void)
{
	TCHAR lpDrive[4];
	TCHAR lpFile[_MAX_FNAME] = L"entries";
	TCHAR lpDirectory[MAX_PATH];
	TCHAR lpExt[_MAX_EXT] = L".txt";


	//Get Running Directory
#ifdef WIN32
	
	wchar_t buffer[MAX_PATH];
    GetModuleFileName( NULL, buffer, MAX_PATH );

#else
	char strDirectory[FILENAME_MAX];
	char szTmp[32];
	int len = 0;

	sprintf(szTmp, "/proc/%d/exe", getpid());
	len = strlen(szTemp);

	int bytes = MIN(readlink(szTmp, strDirectory, len), len - 1);
	if(bytes >= 0)
		pBuf[bytes] = '\0';

	std::string ret(strDirectory);
#endif


	_tsplitpath_s(buffer, lpDrive, 4, lpDirectory, MAX_PATH, NULL, 0, NULL, 0);

	swprintf(buffer, MAX_PATH,L"%s%s",lpDrive,lpDirectory);

	lpDrive[2] = (TCHAR)'\\';
	lpDrive[3] = (TCHAR)'\0';

	strDirectory = buffer;



	for(int i = 0;i< HANDLE_SIZE;i++)
	{
		//Set the handles. 
		//first is file access
		//second is renaming
		int handleCommand = 0;
		switch(i)
		{
			case 0:
				handleCommand = FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_FILE_NAME;
				break;
			case 1:
				handleCommand = FILE_NOTIFY_CHANGE_FILE_NAME;
				break;
			//default : do nothing
			default:
				break;
		}
		if(handleCommand != 0)
		{
			dwChangeHandles[i] = FindFirstChangeNotification( 
			strDirectory,                         // directory to watch 
			FALSE,                         // do not watch subtree 
			handleCommand); // watch file access 

			//validate handle
			if(dwChangeHandles[i] == NULL || dwChangeHandles[i] == INVALID_HANDLE_VALUE)
			{
				printf("\n ERROR: FindFirstChangeNotification function failed on handle %d.\n",i);
				return NULL;//ExitProcess(GetLastError()); 
			}
		}
	}
	return dwChangeHandles;
}
