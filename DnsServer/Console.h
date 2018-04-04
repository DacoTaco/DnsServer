#ifndef _CONSOLE_H_
#define _CONSOLE_H_


#define TEXT_DARK_BLUE 0x01
#define TEXT_DARK_GREEN 0x02
#define TEXT_DARK_CYAN 0x03
#define TEXT_DARK_RED 0x04
#define TEXT_DARK_PURPLE 0x05
#define TEXT_DARK_YELLOW 0x06
#define TEXT_NORMAL 0x07
#define TEXT_DARK_GREY 0x08
#define TEXT_BLUE 0x09
#define TEXT_GREEN 0x0A
#define TEXT_CYAN 0x0B
#define TEXT_RED 0x0C
#define TEXT_PURPLE 0x0D
#define TEXT_YELLOW 0x0E
#define TEXT_WHITE 0x0F
#define TEXT_DARK_DARK_BLUE 0x10



void InitConsole();
void KillConsole();
void printf_colour(char colour, char* Format,...);
void DisplayPacket( unsigned char* data, int size);

#endif