// DnsServer.cpp : Defines the entry point for the console application.
//

//INCLUDES
//--------------
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <fstream>
//#include <windows.h>

#include "stdafx.h"
#include "DnsEntries.h"
#include "Console.h"

//DEFINES
//--------------
#define PORT 53



//VARIABLES
//--------------
SOCKET s;

//FUNCTIONS
//--------------

int LoadEntries()
{
	if(EntriesChanged())
	{
		printf_colour(TEXT_WHITE,"loading DNS entries...\n");
		if ( DNS_Entries.size() > 0)
		{
			for (unsigned int i = 0; i < DNS_Entries.size(); i++)
			{
				unsigned int IP = DNS_Entries[i].IP;

				if ( htonl(47) != 47 ) 
				{
					// Little endian. FML
					IP = _byteswap_ulong(DNS_Entries[i].IP);
				}

				unsigned char ip_hex[4] = {
									(IP & 0xFF000000) >> 24,
									(IP & 0xFF0000) >> 16,
									(IP & 0xFF00) >> 8,
									(IP & 0xFF)
				};
			
				printf_colour(TEXT_YELLOW,"\tHostname = %s\n",DNS_Entries[i].Hostname.c_str());
				printf_colour(TEXT_CYAN,"\tIP = %d.%d.%d.%d - %02X.%02X.%02X.%02X\n",ip_hex[0],ip_hex[1],ip_hex[2],ip_hex[3],ip_hex[0],ip_hex[1],ip_hex[2],ip_hex[3]);
				if(strlen(DNS_Entries[i].comment.c_str()) != 0)
				{
					printf_colour(TEXT_GREEN,"\t--->\t%s\n",DNS_Entries[i].comment.c_str());
				}
				printf("\n");
			}
		}
		else
		{
			printf_colour(TEXT_RED,"No Entries Found!\n");
		}
	}
	return DNS_Entries.size();
}
std::wstring s2ws(const std::string& str)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// wide char to multi byte:
std::string ws2s(const std::wstring& wstr)
{
    int size_needed = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), int(wstr.length() + 1), 0, 0, 0, 0); 
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), int(wstr.length() + 1), &strTo[0], size_needed, 0, 0); 
    return strTo;
}
void exit_app(int exit_code)
{
	Dns_Cleanup();
    closesocket(s);
	KillConsole();
    WSACleanup();
	exit(exit_code);
}
int _tmain(int argc, _TCHAR* argv[])
{
    struct sockaddr_in server, si_other;
    int slen , recv_len;
    unsigned char buf[2048];
    WSADATA wsaData;
 
    slen = sizeof(si_other) ;

	//Init Console stuff
	InitConsole();

	/*for(int i = 0;i < argc;i++)
	{
		std::wstring ws = argv[i];
        std::string arg(ws.begin(), ws.end());  // C++ std::string TCHAR is wchar_t

		printf("arg %i = %s\n",i,arg.c_str());
	}*/
     
    //Initialise winsock
    printf_colour(TEXT_WHITE,"\nInitialising DNS server...");
    if (WSAStartup(MAKEWORD(2,2),&wsaData) != 0)
    {
        printf_colour(TEXT_RED,"Winsock Init Failed. Error Code : %d",WSAGetLastError());
        exit_app(EXIT_FAILURE);
    }
     
    //Create a socket
    if((s = socket(AF_INET , SOCK_DGRAM , 0 )) == INVALID_SOCKET)
    {
        printf_colour(TEXT_RED,"Could not create socket : %d" , WSAGetLastError());
    }

	//Prepare the sockaddr_in structure
    server.sin_family =  AF_INET;     // don't care IPv4 or IPv6 //AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( PORT );

	//Bind
    if( bind(s ,(struct sockaddr *)&server , sizeof(server)) == SOCKET_ERROR)
    {
        printf_colour(TEXT_RED,"Bind failed with error code : %d" , WSAGetLastError());
        exit_app(EXIT_FAILURE);
    }
	printf_colour(TEXT_GREEN,"Initialised!\n");


	//load the entries.txt
	InitDNSWatcher();
	LoadEntries();
	

	//keep listening for data
	/*_getch();
	exit_app(0);*/

	printf("Waiting for data...\n");
    while(1)
    {
        fflush(stdout);

        //clear the buffer by filling null, it might have previously received data
        memset(buf,'\0', 2048);
         
        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(s, (char*)buf, 2048, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
        {
            printf("recvfrom() failed with error code : %d" , WSAGetLastError());
            exit_app(EXIT_FAILURE);
        }
         
        //print details of the client/peer and the data received
        printf("Received packet from %s:%d (0x%04X)\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port),recv_len);
		DisplayPacket(buf,recv_len);

		//check if we need to reload entries before awnsering
		LoadEntries();

		if(buf[2] == 1 && buf[5] == 1)
		{
			char str[1024];
			memset(str,0,1024);
			int pos = 0x0C;
			for(int i = 0;i<1024;i++)//= &buf[13];
			{
				if(i != 0)
					sprintf(str,"%s.",str);
				strncpy((char*)&str[strlen(str)],(char*)&buf[pos+1],buf[pos]);
				
				//printf("hostname part #%d : %x - %x - %s\n",i,buf[pos],pos,str);
				pos += 1 + buf[pos];
				if(pos >= recv_len - 5)
				{
					//printf("end of DNS packet\n");
					//str[strlen(str)-1] = 0x00;
					break;
				}
			}
			printf("dealing with DNS question for : %s\n",str);
			hostent * hostname = gethostbyname(str);
			if(hostname == NULL)
			{
				printf_colour(TEXT_RED,"%s is unavailable\n", str);
				printf_colour(TEXT_RED,"TODO : implement DNS not found\n");
			}
			else
			{
				in_addr * address = NULL;
				for(unsigned int i = 0;i < DNS_Entries.size();i++)
				{
					if(strncmp(DNS_Entries[i].Hostname.c_str(),str,strlen(str)) == 0)
					{
						printf_colour(TEXT_GREEN,"%s detected! redirecting...\n",DNS_Entries[i].Hostname.c_str());
						address = (in_addr*)&DNS_Entries[i].IP;
						break;
					}
				}
				if(address == NULL)
				{
					address = (in_addr * )hostname->h_addr;
				}
				char ip_address[100];
				memset(ip_address,0,100);
				strncpy(ip_address,inet_ntoa(* address),100);
				//ip_address = inet_ntoa(* address);
				unsigned int IP = inet_addr(ip_address);
				//fucking endians...
				if ( htonl(47) != 47 ) {
				  // Little endian. FML
					IP = _byteswap_ulong(IP);
				}
				printf("IP : %s (%02X.%02X.%02X.%02X)\n",ip_address,(IP & 0xFF000000) >> 24,(IP & 0xFF0000) >> 16,(IP & 0xFF00) >> 8,(IP & 0xFF));
				
				//send reply
				unsigned char reply[1024];
				int send_len = recv_len + 0x10;
				memset(reply,0,1024);
				memcpy(reply,buf,recv_len);
				//now to change the reply and add the IP...
				reply[2] = 0x81;
				reply[3] = 0x80;
				reply[7] = 0x01;

				//append reply command
				memcpy((char*)&reply[recv_len],"\xC0\x0C",0x2);
				//append type and class which we copy from the request
				memcpy((char*)&reply[recv_len+2],(char*)&reply[recv_len-4],0x4);
				//append TTL. just making up number here XD
				memset((char*)&reply[recv_len+9],0x08,0x01);
				//append data lenght. this is a IPv4 ip so 4 bytes
				memset((char*)&reply[recv_len+11],0x04,0x01);
				//append IP
				memcpy((char*)&reply[recv_len+12],address,0x04);

				printf("sending packet to %s:%d (0x%04X)\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port),send_len);
				DisplayPacket(reply,send_len);
				if (sendto(s, (char*)reply, send_len, 0, (struct sockaddr*) &si_other, slen) == SOCKET_ERROR)
				{
					printf("sendto() failed with error code : %d" , WSAGetLastError());
					exit_app(EXIT_FAILURE);
				}

			}
		}
		printf("\n\n");
    }
	exit_app(0);
	return 0;
}

