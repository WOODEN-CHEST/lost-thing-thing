#include <stdbool.h>
#include "LttCommon.h"
#include "File.h"
#include <stdio.h>

int main()
{
	char Data[3];
	Data[0] = 0xC4;
	Data[1] = 0xB7;
	Data[2] = 0;

	char Path[] = "C:\\Users\\User\\Desktop\\test.txt";
	FILE* File;
	//File_WriteString(File, Data);
	//File_Close(File);

	File = File_Open(Path, Read);
	char* Stuff = File_ReadAllText(File);
	int Count = String_LengthCodepointsUTF8(Stuff);

	return 0;
}