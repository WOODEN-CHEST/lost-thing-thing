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
FILE* File_Open(const char* path, File_OpenMode mode, Error* error);

int File_ReadByte(FILE* file, Error* error);

size_t File_Read(FILE* file, char* dataBuffer, size_t count);

char* File_ReadAllText(FILE* file, Error* error);

char* File_ReadAllData(FILE* file, size_t* dataLength, Error* error);

Error File_Write(FILE* file, const char* data, size_t dataLength);

Error File_WriteText(FILE* file, const char* string);

Error File_WriteByte(FILE* file, int byte);

Error File_Flush(FILE* file);

Error File_Close(FILE* file);

void File_Delete(const char* path);

_Bool File_Exists(const char* path);

Error File_Move(const char* sourcePath, const char* destinationPath);