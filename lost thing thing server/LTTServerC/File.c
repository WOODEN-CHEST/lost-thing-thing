#include "File.h"
#include "LttString.h"
#include "Errors.h"
#include "Memory.h"
#include <stdio.h>
#include <stdbool.h>
#include <String.h>
#include <sys/stat.h>


// Macros.
#define MODE_STRING_LENGTH 3


// Static functions.
static char* AllocateMemoryForFileRead(FILE* file)
{
	long long CurPosition = ftell(file);
	if (CurPosition == -1)
	{
		Error_SetError(ErrorCode_IO, "(File) AllocateMemoryForFileRead: Failed to tell current position.");
		return NULL;
	}

	int Result = fseek(file, 0, SEEK_END);
	if (Result)
	{
		Error_SetError(ErrorCode_IO, "File_ReadAllText: Failed to seek to file end.");
		return NULL;
	}

	long long Length = ftell(file);
	if (Length == -1)
	{
		Error_SetError(ErrorCode_IO, "File_ReadAllText: Failed to tell the file's stream position.");
		return NULL;
	}

	Result = fseek(file, 0, SEEK_SET);
	if (Result)
	{
		Error_SetError(ErrorCode_IO, "File_ReadAllText: Failed to seek to file start.");
		return NULL;
	}
}


// Functions.
FILE* File_Open(char* path, File_OpenMode mode)
{
	char OpenMode[MODE_STRING_LENGTH];
	switch (mode)
	{
		case FileOpenMode_Read:
			strcpy_s(OpenMode, MODE_STRING_LENGTH, "r");
			break;

		case FileOpenMode_Write:
			strcpy_s(OpenMode, MODE_STRING_LENGTH, "w");
			break;

		case FileOpenMode_Append:
			strcpy_s(OpenMode, MODE_STRING_LENGTH, "a");
			break;

		case FileOpenMode_ReadBinary:
			strcpy_s(OpenMode, MODE_STRING_LENGTH, "rb");
			break;

		case FileOpenMode_WriteBinary:
			strcpy_s(OpenMode, MODE_STRING_LENGTH, "wb");
			break;

		case FileOpenMode_AppendBinary:
			strcpy_s(OpenMode, MODE_STRING_LENGTH, "ab");
			break;

		case FileOpenMode_ReadUpdate:
			strcpy_s(OpenMode, MODE_STRING_LENGTH, "r+");
			break;

		case FileOpenMode_WriteUpdate:
			strcpy_s(OpenMode, MODE_STRING_LENGTH, "w+");
			break;

		case FileOpenMode_AppendUpdate:
			strcpy_s(OpenMode, MODE_STRING_LENGTH, "a+");
			break;

		default:
			Error_SetError(ErrorCode_InvalidArgument, "File_Open: Invalid open mode.");
			return NULL;
	}

	FILE* File = fopen(path, OpenMode);
	if (File == NULL)
	{
		Error_SetError(ErrorCode_IO, "File_Open: Failed to open file.");
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
	int Result = fseek(file, 0, SEEK_END);
	if (Result)
	{
		Error_SetError(ErrorCode_IO, "File_ReadAllText: Failed to seek to file end.");
		return NULL;
	}

	long long Length = ftell(file);
	if (Length == -1)
	{
		Error_SetError(ErrorCode_IO, "File_ReadAllText: Failed to tell the file's stream position.");
		return NULL;
	}

	Result = fseek(file, 0, SEEK_SET);
	if (Result)
	{
		Error_SetError(ErrorCode_IO, "File_ReadAllText: Failed to seek to file start.");
		return NULL;
	}

	char* Buffer = Memory_SafeMalloc(Length + 1);
	Result = fread(Buffer, sizeof(char), Length, file);

	Buffer[Length] = '\0';
	return Buffer;
}

char* File_ReadAllData(FILE* file, size_t* dataLength)
{

}

bool File_Exists(char* path)
{
	struct stat FileStats;
	return !stat(path, &FileStats);
}

ErrorCode File_Delete(char* path)
{
	int Result = remove(path);

	if (!Result)
	{
		return ErrorCode_Success;
	}
	return Error_SetError(ErrorCode_IO, "File_Delete: Failed to delete file.");
}