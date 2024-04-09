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
static char* AllocateMemoryForFileRead(FILE* file, size_t* fileSize, size_t extraBufferSize, Error* error)
{
	long long CurPosition = ftell(file);
	if (CurPosition == -1)
	{
		*error = Error_CreateError(ErrorCode_IO, "(File) AllocateMemoryForFileRead: Failed to tell current position.");
		return NULL;
	}

	int Result = fseek(file, 0, SEEK_END);
	if (Result)
	{
		*error = Error_CreateError(ErrorCode_IO, "(File) AllocateMemoryForFileRead: Failed to seek to file end.");
		return NULL;
	}

	long long Length = ftell(file);
	if (Length == -1)
	{
		*error = Error_CreateError(ErrorCode_IO, "(File) AllocateMemoryForFileRead: Failed to tell the file's stream position.");
		return NULL;
	}

	Result = fseek(file, 0, SEEK_SET);
	if (Result)
	{
		*error = Error_CreateError(ErrorCode_IO, "(File) AllocateMemoryForFileRead: Failed to seek to file start.");
		return NULL;
	}

	*fileSize = (size_t)Length;
	char* Data = Memory_SafeMalloc((size_t)Length + extraBufferSize);
	*error = Error_CreateSuccess();
	return Data;
}


// Functions.
FILE* File_Open(const char* path, File_OpenMode mode, Error* error)
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
			if (error)
			{
				*error = Error_CreateError(ErrorCode_InvalidArgument, "File_Open: Invalid open mode.");
			}
			return NULL;
	}

	FILE* File = fopen(path, OpenMode);
	if (!File && error)
	{
		*error = Error_CreateError(ErrorCode_IO, "File_Open: Failed to open file.");
	}
	else if (error)
	{
		*error = Error_CreateSuccess();
	}

	return File;
}

int File_ReadByte(FILE* file, Error* error)
{
	int Result = fgetc(file);
	if (Result == EOF)
	{
		if (error)
		{
			*error = Error_CreateError(ErrorCode_IO, "File_ReadByte: Failed to read byte.");
		}
		return 0;
	}

	if (error)
	{
		*error = Error_CreateSuccess();
	}

	return Result;
}

size_t File_Read(FILE* file, char* dataBuffer, size_t count)
{
	return fread(dataBuffer, sizeof(char), count, file);
}

char* File_ReadAllText(FILE* file, Error* error)
{
	size_t FileSize;
	Error ReturnedError;
	char* Buffer = AllocateMemoryForFileRead(file, &FileSize, 1, &ReturnedError);

	if (ReturnedError.Code != ErrorCode_Success)
	{
		if (error)
		{
			*error = ReturnedError;
		}
		return NULL;
	}

	size_t Result = fread(Buffer, sizeof(char), FileSize, file);

	Buffer[Result] = '\0';

	if (error)
	{
		*error = Error_CreateSuccess();
	}

	return Buffer;
}

char* File_ReadAllData(FILE* file, size_t* dataLength, Error* error)
{
	size_t FileSize;
	Error ReturnedError;
	char* Buffer = AllocateMemoryForFileRead(file, &FileSize, 0, &ReturnedError);

	if (ReturnedError.Code != ErrorCode_Success)
	{
		if (error)
		{
			*error = ReturnedError;
		}
		return NULL;
	}

	*dataLength = fread(Buffer, sizeof(char), FileSize, file);

	if (error)
	{
		*error = Error_CreateSuccess();
	}
	
	return Buffer;
}

Error File_Write(FILE* file, const char* data, size_t dataLength)
{
	size_t Result = fwrite(data, 1, dataLength, file);;
	return Result != EOF ? Error_CreateSuccess() : Error_CreateError(ErrorCode_IO, "File_Write: Failed to write bytes to file.");
}

Error File_WriteText(FILE* file, const char* string)
{
	int Result = fputs(string, file);
	return Result != EOF ? Error_CreateSuccess() : Error_CreateError(ErrorCode_IO, "File_WriteString: Failed to write string to file.");
}

Error File_WriteByte(FILE* file, int byte)
{
	int Result = fputc(byte, file);
	return Result == byte ? Error_CreateSuccess() : Error_CreateError(ErrorCode_IO, "File_WriteByte: Failed to write byte");
}

Error File_Flush(FILE* file)
{
	int Success = fflush(file);

	return Success == 0 ? Error_CreateSuccess() : Error_CreateError(ErrorCode_IO, "File_Flush: Failed to flush file.");
}

Error File_Close(FILE* file)
{
	int Success = fclose(file);

	return Success == 0 ? Error_CreateSuccess() : Error_CreateError(ErrorCode_IO, "File_Close: Failed to close file.");
}

void File_Delete(const char* path)
{
	remove(path);
}

_Bool File_Exists(const char* path)
{
	struct stat FileStats;
	return !stat(path, &FileStats);
}

Error File_Move(const char* sourcePath, const char* destinationPath)
{
	return MoveFileA(sourcePath, destinationPath) ? Error_CreateSuccess() : Error_CreateError(ErrorCode_IO, "File_Move: Failed to move file");
}