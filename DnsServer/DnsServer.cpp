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

#include "stdafx.h"

#include "DnsEntries.h"
#include "DnsPacket.h"
#include "Console.h"

//DEFINES
//--------------
#define PORT 53

//VARIABLES
//--------------
SOCKET s;

//FUNCTIONS
//--------------

unsigned int ToBigEndian(unsigned int value)
{
	if (htonl(47) != 47)
		return _byteswap_ulong(value);
	return value;
}

int LoadEntries()
{
	if(EntriesChanged())
	{
		printf_colour(TEXT_WHITE,"loading DNS entries...\n");
		if ( DNS_Entries.size() > 0)
		{
			for (unsigned int i = 0; i < DNS_Entries.size(); i++)
			{
				unsigned char* ip_hex = DNS_Entries[i].IP;
			
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
		char ip_address_str[INET6_ADDRSTRLEN];
		unsigned char ip_address[16];
		DnsResponseCodes responseCode = DnsResponseCodes::NameError;
		printf_colour(TEXT_WHITE, "Received packet from %s:%d (0x%04X)\n", inet_ntop(AF_INET, &si_other.sin_addr, ip_address_str, sizeof(ip_address_str)), ntohs(si_other.sin_port), recv_len);
		DisplayPacket(buf,recv_len);
		printf("\n");

		//check if we need to reload entries before awnsering
		LoadEntries();

		DnsPacket dnsPacket;
		dnsPacket.Parse(buf, recv_len);

		if(!dnsPacket.IsResponse() && dnsPacket.RequestCount() > 0)
		{
			char hostname[1024];
			memset(hostname, 0, 1024);
			dnsPacket.Hostname(hostname, 1024);
			printf_colour(TEXT_WHITE, "dealing with DNS question for : '%s'\n", hostname);
			memset(ip_address_str, 0, INET6_ADDRSTRLEN);
			memset(ip_address, 0, sizeof(ip_address));

			//look through our settings
			auto it = std::find_if(DNS_Entries.begin(), DNS_Entries.end(), [&](const DNS_Entry& obj) 
				{
					return strncmp(obj.Hostname.c_str(), hostname, strlen(hostname)) == 0;
				});
			if (it != DNS_Entries.end())
			{
				DNS_Entry entry = (DNS_Entry)(*it);
				printf_colour(TEXT_GREEN, "%s detected! redirecting...\n", entry.Hostname.c_str());
				memcpy(ip_address, entry.IP, sizeof(entry.IP));
				responseCode = DnsResponseCodes::NoError;
			}
			else //resolve hostname if we didn't have it in our settings.
			{
				int status;
				struct addrinfo hints;
				struct addrinfo* hostname_info;  // will point to the results

				memset(&hints, 0, sizeof hints); // make sure the struct is empty
				hints.ai_family = AF_INET;     // don't care IPv4 or IPv6
				hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
				hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

				if ((status = getaddrinfo(hostname, NULL, &hints, &hostname_info)) != 0 && status != 11001)
				{
					printf_colour(TEXT_RED, "getaddrinfo error: %s(%d)\n", strerror(status), status);
					responseCode = DnsResponseCodes::ServerError;
				}
				else
				{
					printf_colour(TEXT_NORMAL, "IP addresses for %s:\n", hostname);
					char ip[INET6_ADDRSTRLEN];
					memset(ip, 0, INET6_ADDRSTRLEN);

					for (addrinfo* p = hostname_info; p != NULL; p = p->ai_next)
					{
						void* addr;
						char* ipver;
						if (p->ai_family == AF_INET) //IPv4
						{
							struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
							addr = &(ipv4->sin_addr);
							ipver = "IPv4";
							if (responseCode != DnsResponseCodes::NoError)
							{
								memcpy(ip_address, addr, sizeof(4));
								responseCode = DnsResponseCodes::NoError;
							}
						}
						else //IPv6
						{
							struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
							addr = &(ipv6->sin6_addr);
							ipver = "IPv6";
						}

						// convert the IP to a string and print it:
						inet_ntop(p->ai_family, addr, ip, sizeof ip);
						printf_colour(TEXT_NORMAL, "\t%s: %s\n", ipver, ip);
					}
					freeaddrinfo(hostname_info);
				}
			}

			//Reply to DNS Request			
			if (responseCode != DnsResponseCodes::NoError)
			{
				printf_colour(TEXT_RED, "%s is unavailable\n", hostname);
				printf_colour(TEXT_RED, "IP not found. Sending error.\n");
				dnsPacket.SetError(responseCode);
			}
			else
			{
				inet_ntop(AF_INET, ip_address, ip_address_str, INET6_ADDRSTRLEN);
				printf_colour(TEXT_WHITE, "replying : %s (%02X.%02X.%02X.%02X)\n", ip_address_str, ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
				dnsPacket.AddResponse(ip_address, 4); 
			}

			unsigned char reply[2048];
			int send_len = dnsPacket.Build(reply, sizeof(reply));
			printf("sending packet to %s:%d (0x%04X)\n", ip_address_str, ntohs(si_other.sin_port),send_len);
			DisplayPacket(reply,send_len);
			if (sendto(s, (char*)reply, send_len, 0, (struct sockaddr*) &si_other, slen) == SOCKET_ERROR)
			{
				printf("sendto() failed with error code : %d" , WSAGetLastError());
				exit_app(EXIT_FAILURE);
			}
		}
		printf("\n");
    }
	exit_app(0);
	return 0;
}

