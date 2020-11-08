#ifndef _DNSENTRIES_H_
#define _DNSENTRIES_H_

//Includes!
//--------------


//structs!
//--------------
struct DNS_Entry
{
	unsigned char IP[4];
	std::string Hostname;
	std::string comment;
	bool enabled;
};

//Variables!
//--------------
extern std::vector<DNS_Entry> DNS_Entries;

//Functions!
//--------------

bool InitDNSWatcher(void);
bool EntriesChanged(void);
void Dns_Cleanup();

#endif