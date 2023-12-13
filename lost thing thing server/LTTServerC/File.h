#pragma once
#include "LttString.h"
#include <stdio.h>

// Structures.
enum File_OpenModeEnum
{
	Read = 1,
	Write = 2,
	Append = 3,
	ReadUpdate = 4,
	WriteUpdate = 5,
	AppendUpdate = 6,
};

typedef enum File_OpenModeEnum File_OpenMode;


// Functions.
FILE* File_Open(char* path, File_OpenMode mode);

int File_Write(FILE* file, char* data, size_t dataLength);

int File_WriteString(FILE* file, char* string);

int File_Flush(FILE* file);

int File_Close(FILE* file);

int File_Delete(char* path);

char* File_ReadAllText(FILE* file);

bool File_Exists(char* path);