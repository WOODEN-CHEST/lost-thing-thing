#include "File.h"
#include "LttString.h"
#include "ErrorCodes.h"
#include "Memory.h"
#include <stdio.h>
#include <stdbool.h>
#include <String.h>
#include <sys/stat.h>

#define MODE_STRING_LENGTH 3

// Functions.
FILE* File_Open(char* path, File_OpenMode mode)
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
	fopen_s(&File, path, OpenMode);
	if (File == NULL)
	{
		return NULL;
	}

	return File;
}

int File_Write(FILE* file, char* data, size_t dataLength)
{
	if ((file == NULL) || (data == NULL))
	{
		return NULL_REFERENCE_ERRCODE;
	}

	size_t Result = fwrite(data, 1, dataLength, file);

	return Result != dataLength ? IO_ERROR_ERRCODE : 0;
}

int File_WriteString(FILE* file, char* string)
{
	if ((file == NULL) || (string == NULL))
	{
		return NULL_REFERENCE_ERRCODE;
	}

	int Result = fputs(string, file);

	return Result != EOF ? 0 : IO_ERROR_ERRCODE;
}

int File_Flush(FILE* file)
{
	if (file == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	int Success = fflush(file);

	if (Success != 0)
	{
		return IO_ERROR_ERRCODE;
	}
	return 0;
}

int File_Close(FILE* file)
{
	if (file == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	int Success = fclose(file);

	if (Success != 0)
	{
		return IO_ERROR_ERRCODE;
	}
	return 0;
}

char* File_ReadAllText(FILE* file)
{
	if (file == NULL)
	{
		return NULL;
	}

	int Result = fseek(file, 0, SEEK_END);
	if (Result != 0)
	{
		return NULL;
	}

	long long Length = ftell(file);
	if (Length == -1)
	{
		return NULL;
	}

	Result = fseek(file, 0, SEEK_SET);
	if (Result != 0)
	{
		return NULL;
	}

	char* Buffer = SafeMalloc(Length + 1);
	Result = fread_s(Buffer, Length, sizeof(char), Length, file);

	if (Result != Length)
	{
		FreeMemory(Buffer);
		return NULL;
	}

	Buffer[Length] = '\0';
	return Buffer;
}

bool File_Exists(char* path)
{
	if (path == NULL)
	{
		return false;
	}

	struct stat FileStats;
	int Result = stat(path, &FileStats);

	return Result == 0;
}

int File_Delete(char* path)
{
	if (path == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	int Result = remove(path);

	if (Result == 0)
	{
		return 0;
	}
	return IO_ERROR_ERRCODE;
}