#include "File.h"
#include "LttString.h"
#include "ErrorCodes.h"
#include "Memory.h"
#include <stdio.h>
#include <stdbool.h>
#include <String.h>

#define MODE_STRING_LENGTH 3

// Functions.
int File_DeleteFile(string* path)
{

}

FileStream* File_Open(string* path, File_OpenMode mode)
{
	if ((path == NULL) || (mode == NULL))
	{
		return NULL;
	}

	char OpenMode[MODE_STRING_LENGTH];
	switch (mode)
	{
		case Read:
			strcpy_s(OpenMode, MODE_STRING_LENGTH, "r");
			break;

		case Write:
			strcpy_s(OpenMode, MODE_STRING_LENGTH, "w");
			break;

		case Append:
			strcpy_s(OpenMode, MODE_STRING_LENGTH, "a");
			break;

		case ReadUpdate:
			strcpy_s(OpenMode, MODE_STRING_LENGTH, "r+");
			break;

		case WriteUpdate:
			strcpy_s(OpenMode, MODE_STRING_LENGTH, "w+");
			break;

		case AppendUpdate:
			strcpy_s(OpenMode, MODE_STRING_LENGTH, "a+");
			break;

		default:
			return NULL;
	}

	FILE* File;
	fopen_s(&File, path->Data, OpenMode);
	if (File == NULL)
	{
		return NULL;
	}

	FileStream* Stream = SafeMalloc(sizeof(FileStream));
	Stream->Path = path;
	Stream->File = File;

	return Stream;
}

int File_Flush(FileStream* this)
{
	if (this == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	int Success = fflush(this->File);

	if (Success != 0)
	{
		return IO_ERROR_ERRCODE;
	}
	return 0;
}

int File_Close(FileStream* this)
{
	if (this == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	int Success = fclose(this->File);

	if (Success != 0)
	{
		return IO_ERROR_ERRCODE;
	}
	return 0;
}

bool File_FileExists(string* path)
{
	
}

int File_Write(FileStream* this, string* text)
{
	if ((this == NULL) || (text == NULL))
	{
		return NULL_REFERENCE_ERRCODE;
	}

	int Success = fprintf(this->File, text->Data);
	if (Success < 0)
	{
		return IO_ERROR_ERRCODE;
	}

	return 0;
}