#include "File.h"
#include "LttString.h"
#include "LttErrors.h"
#include "Memory.h"
#include <stdio.h>
#include <stdbool.h>
#include <String.h>
#include <sys/stat.h>
#include <Windows.h>


// Macros.
#define MODE_STRING_LENGTH 3


// Static functions.
static char* AllocateMemoryForFileRead(FILE* file, size_t* fileSize, size_t extraBufferSize)
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

	*fileSize = (size_t)Length;
	char* Data = Memory_SafeMalloc((size_t)Length + extraBufferSize);
	return Data;
}


// Functions.
FILE* File_Open(const char* path, File_OpenMode mode)
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

int File_ReadByte(FILE* file)
{
	Error_ClearError();

	int Result = fgetc(file);
	if (Result == EOF)
	{
		Error_SetError(ErrorCode_IO, "File_ReadByte: Failed to read byte.");
		return 0;
	}
	return Result;
}

size_t File_Read(FILE* file, char* dataBuffer, size_t count)
{
	return fread(dataBuffer, sizeof(char), count, file);
}

char* File_ReadAllText(FILE* file)
{
	size_t FileSize;
	char* Buffer = AllocateMemoryForFileRead(file, &FileSize, 1);
	if (!Buffer)
	{
		Error_SetError(ErrorCode_IO, "File_ReadAllText: Failed to create buffer for file data.");
		return NULL;
	}

	size_t Result = fread(Buffer, sizeof(char), FileSize, file);

	Buffer[Result] = '\0';
	return Buffer;
}

char* File_ReadAllData(FILE* file, size_t* dataLength)
{
	size_t FileSize;
	char* Buffer = AllocateMemoryForFileRead(file, &FileSize, 0);
	*dataLength = fread(Buffer, sizeof(char), FileSize, file);
	return Buffer;
}

ErrorCode File_Write(FILE* file, const char* data, size_t dataLength)
{
	size_t Result = fwrite(data, 1, dataLength, file);;
	return Result != EOF ? ErrorCode_Success : Error_SetError(ErrorCode_IO, "File_Write: Failed to write bytes to file.");
}

ErrorCode File_WriteText(FILE* file, const char* string)
{
	int Result = fputs(string, file);
	return Result != EOF ? ErrorCode_Success : Error_SetError(ErrorCode_IO, "File_WriteString: Failed to write string to file.");
}

ErrorCode File_WriteByte(FILE* file, int byte)
{
	int Result = fputc(byte, file);
	return Result == byte ? ErrorCode_Success : Error_SetError(ErrorCode_IO, "File_WriteByte: Failed to write byte");
}

ErrorCode File_Flush(FILE* file)
{
	int Success = fflush(file);

	return Success == 0 ? ErrorCode_Success : Error_SetError(ErrorCode_IO, "File_Flush: Failed to flush file.");
}

ErrorCode File_Close(FILE* file)
{
	int Success = fclose(file);

	return Success == 0 ? ErrorCode_Success : Error_SetError(ErrorCode_IO, "File_Close: Failed to close file.");
}

ErrorCode File_Delete(const char* path)
{
	int Result = remove(path);

	if (!Result)
	{
		return ErrorCode_Success;
	}
	return Error_SetError(ErrorCode_IO, "File_Delete: Failed to delete file.");
}

_Bool File_Exists(const char* path)
{
	struct stat FileStats;
	return !stat(path, &FileStats);
}

ErrorCode File_Move(const char* sourcePath, const char* destinationPath)
{
	return MoveFileA(sourcePath, destinationPath) ? ErrorCode_Success : Error_SetError(ErrorCode_IO, "File_Move: Failed to move file");
}