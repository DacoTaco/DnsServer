#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <winsock2.h>
#include <thread>
#include <direct.h>

#include "rapidxml.hpp"
#include "rapidxml_utils.hpp"

#include "DnsEntries.h"
#include "Console.h"

#ifdef WIN32

#include <windows.h>
#include <tchar.h>

typedef DWORD ulong;
typedef WORD ushort;
typedef UINT32 uint;

#endif


using namespace rapidxml;
using namespace std;


#define SETTINGS_FILE "DNS.xml"
#define LSETTINGS_FILE L"DNS.xml"
std::vector<DNS_Entry> DNS_Entries;
bool _init = false;
bool bThread_Exit = false;
bool bThread_Running = false;
bool bReloadEntries = true;
std::thread tDnsThread;
HANDLE Directory_handle;

int GetDnsEntries(void)
{
	if(!_init)
	{
		return 0;
	}

	//clean list if its already loaded
	if(DNS_Entries.size() > 0)
	{
		DNS_Entries.clear();
	}

	//XML READING
	try {
		rapidxml::file<> xmlFile(SETTINGS_FILE); // Default template is char
		rapidxml::xml_document<> doc;
		doc.parse<0>(xmlFile.data());
		xml_node<>* entries_node = doc.first_node("DNSEntries");

		for(xml_node<>* node = entries_node->first_node("DNSEntry");node;node = node->next_sibling())
		{
			DNS_Entry temp;
			temp.Hostname = node->first_attribute("hostname")->value();
			temp.comment = node->first_attribute("comment")->value();
			unsigned int IP = 0;
				IP = inet_addr(node->first_attribute("ip")->value());
				//fucking endians...
				//for some odd reason in XML parsing we wont need the endian switching?!
				/*if ( htonl(47) != 47 ) {
				  // Little endian. FML
					IP = _byteswap_ulong(IP);
				}*/
				temp.IP = IP;
			string test = node->first_attribute("enabled")->value();
			if(strncmp(node->first_attribute("enabled")->value(),"true",4) == 0)
				temp.enabled = true;
			else
				temp.enabled = false;

			if(temp.enabled)
				DNS_Entries.push_back(temp);
		}
	}
	catch(...)
	{
		//we got an error processing the xml! D:
		bReloadEntries = true;
		return DNS_Entries.size();
	}

	bReloadEntries = true;
	return DNS_Entries.size();
}
void Dns_Cleanup()
{
	if(bThread_Running)
	{
		printf_colour(TEXT_WHITE,"DirectoryWatcher Thread kill ...\n");
		CancelIo(Directory_handle);
		bThread_Exit = true;
		tDnsThread.join();
	}
}
void Dns_Thread()
{
	bThread_Running = true;
	printf_colour(TEXT_WHITE,"DirectoryWatcher Thread started...\n");
	
	FILE_NOTIFY_INFORMATION returnData[1024];
	memset(returnData,0,1024);
	DWORD returnDataSize = 0; 
	LPDWORD ret_Value = 0;

	OVERLAPPED overlapped_structure;
	memset(&overlapped_structure, 0, sizeof(overlapped_structure));

	ReadDirectoryChangesW(	Directory_handle, returnData, sizeof(returnData), 
									FALSE, FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_DIR_NAME|FILE_NOTIFY_CHANGE_LAST_WRITE,
									&returnDataSize, &overlapped_structure,NULL
								);

	while(bThread_Exit == false)
	{
		if( ( !bThread_Exit ) && (returnData != NULL) && (wcslen(returnData[0].FileName) != 0) )
		{
			//wprintf(L"lol %s\n",returnData[0].FileName);

			std::wstring temp = returnData[0].FileName;


			if(temp == LSETTINGS_FILE)
			{
				printf_colour(TEXT_BLUE,"%s accessed, Reloading DNS Entries soon...\n",SETTINGS_FILE);
				//wait a bit so we are sure the file is written correctly!
				Sleep(200);
				GetDnsEntries();
			}


			//and reset everything :)
			memset(returnData,0,1024);
			returnDataSize = 0; 
			memset(&overlapped_structure, 0, sizeof(overlapped_structure));

			ReadDirectoryChangesW(	Directory_handle, returnData, sizeof(returnData), 
									FALSE, FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_DIR_NAME|FILE_NOTIFY_CHANGE_LAST_WRITE,
									&returnDataSize, &overlapped_structure,NULL
								);
		}

	}

	CloseHandle(Directory_handle);
	bThread_Exit = false;
	bThread_Running = false;
	printf_colour(TEXT_WHITE,"DirectoryWatcher Shutting Down...\n");
	return;
}
bool InitDNSWatcher(void)
{
	wchar_t _dir[FILENAME_MAX];
	GetCurrentDirectoryW( FILENAME_MAX,_dir);
	std::wstring dir(_dir);
	
	Directory_handle = ::CreateFile(dir.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED, NULL);
	

	if(Directory_handle != NULL)
	{
		_init = true;
		bReloadEntries = true;
		GetDnsEntries();
		tDnsThread = std::thread(Dns_Thread);
	}
	else
	{
		printf_colour(TEXT_RED,"ERROR STARTING DIRECTORY WATCHER\n");
		return false;
	}
	return true;
}
bool EntriesChanged()
{
	if(_init)
	{
		bool ret = bReloadEntries;
		if(bReloadEntries)
			bReloadEntries = false;
		return ret;
	}
	else
		return false;
}