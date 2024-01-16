#pragma once
#include "LttString.h"
#include <stdio.h>


// Structures.
enum File_OpenModeEnum
{
	FileOpenMode_Read,
	FileOpenMode_Write,
	FileOpenMode_Append,
	FileOpenMode_ReadBinary,
	FileOpenMode_WriteBinary,
	FileOpenMode_AppendBinary,
	FileOpenMode_ReadUpdate,
	FileOpenMode_WriteUpdate,
	FileOpenMode_AppendUpdate
};

typedef enum File_OpenModeEnum File_OpenMode;


// Functions.
FILE* File_Open(char* path, File_OpenMode mode);

int File_ReadChar(FILE* file);

size_t File_Read(FILE* file, char* dataBuffer, size_t count);

char* File_ReadAllText(FILE* file);

char* File_ReadAllData(FILE* file, size_t* dataLength);

ErrorCode File_Write(FILE* file, char* data, size_t dataLength);

ErrorCode File_WriteText(FILE* file, char* string);

ErrorCode File_WriteByte(FILE* file, char byte);

ErrorCode File_Flush(FILE* file);

ErrorCode File_Close(FILE* file);

ErrorCode File_Delete(char* path);

_Bool File_Exists(char* path);