#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>

#include "Console.h"

CRITICAL_SECTION critical;
bool critical_init = false;

void InitConsole()
{
	InitializeCriticalSection(&critical);
}
void KillConsole()
{
	DeleteCriticalSection(&critical); 
}
void printf_colour(char colour, char* Format,...)
{
	/*int count;
	for (count=0; count<257; count++)
     {   SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), count);
          printf ("This color combination has the number of %i\n",count);
     }*/

	EnterCriticalSection(&critical);

	WORD m_ConsoleAttr = 0;
	CONSOLE_SCREEN_BUFFER_INFO   csbi;
	HANDLE hConsole;

	char astr[2048];
	int size = 0;
	memset(astr,0,sizeof(astr));
	va_list ap;

	//retrieve and save the current attributes
	hConsole=GetStdHandle(STD_OUTPUT_HANDLE);
	if(GetConsoleScreenBufferInfo(hConsole, &csbi))
		m_ConsoleAttr = csbi.wAttributes;

	va_start(ap,Format);
	size = vsnprintf( astr, 2047, Format, ap );
	va_end(ap);

	//reset colour...
	SetConsoleTextAttribute(hConsole, TEXT_NORMAL);

	SetConsoleTextAttribute(hConsole, colour);

	printf(astr);
	if(m_ConsoleAttr == 0)
		m_ConsoleAttr = 0x07;
	SetConsoleTextAttribute(hConsole, m_ConsoleAttr);

	LeaveCriticalSection(&critical);

	return;
}
void DisplayPacket( unsigned char* data, int size)
{
	if(data == NULL || size == 0)
		return;

	printf("(0000) ");
	int newline = 0;
	for ( int i = 0 ; i < size ; i++)
	{
		printf("%02X " , (unsigned)data[i] );
		if(i+1 >= newline+0x10 || i+1 >= size)
		{
			if(i+1 >= size)
			{
				//end of the packet. pad the data
				for(int z = (0x0F - i&0x0F);z > 0;z--)
				{
					printf("   ");//printf("%02x ",recv_len);
				}
			}

			//end of line! time to display ascii !
			printf(" | ");
			for(int y = newline;y < newline+0x10 ;y++)
			{
				if(data[y] < 0x20 || data[y] > 0x7E)
					printf(".");
				else
					printf("%c",data[y]);
				if(y+1 >= size)
					break;
			}
			if(i+1 < size)
			{
				newline += 0x10;
				printf("\n(%04x) ",newline);
			}
		}
	}
	printf("\n");

	return;
}