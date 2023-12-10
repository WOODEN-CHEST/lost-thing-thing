#pragma once
#include "LttString.h"
#include <stdio.h>

// Structures.
struct FileStreamStruct
{
	string* Path;
	FILE* File;
};

typedef struct FileStreamStruct FileStream;

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
int File_DeleteFile(string* path);

FileStream* File_Open(string* path, File_OpenMode mode);

int File_Flush(FileStream* this);

int File_Close(FileStream* this);

int File_Write(FileStream* this, string* text);