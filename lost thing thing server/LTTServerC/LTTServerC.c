#include <stdbool.h>
#include "LttCommon.h"
#include "File.h"
#include <stdio.h>

int main()
{
	//Main();

	char Path[] = "C:/Users/User/Desktop/test.txt";

	FILE* File = File_Open(Path,  Read);
	char* Text = File_ReadAllText(File);
	File_Close(File);

	String_ToUpperUTF8(Text);

	File = File_Open(Path, Write);
	File_WriteString(File, Text);
	File_Close(File);

	return 0;
}