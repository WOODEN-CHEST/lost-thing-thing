#include "Directory.h"
#include <stdio.h>
#include <sys/stat.h>
#include <fileapi.h>
#include <stdbool.h>
#include "ErrorCodes.h"

// FUnctions.
// Functions.
bool Directory_Exists(char* path)
{
	if (path == NULL)
	{
		return false;
	}

	struct stat DirectoryStats;
	int Result = stat(path, &DirectoryStats);

	return Result == 0;
}

int Directory_Create(char* path)
{
	if (path == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	bool Result = CreateDirectoryA(path, NULL);

	return Result ? 0 : IO_ERROR_ERRCODE;
}

int Directory_Delete(char* path)
{
	
}