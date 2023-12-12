#include "Directory.h"
#include "ErrorCodes.h"
#include "ArrayList.h"
#include "LttString.h"
#include "Memory.h"
#include <Windows.h>
#include <sys/stat.h>

#define DIR_NAME_MAX_LENGTH 512

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

int Directory_CreateAll(char* path)
{
	if (path == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	if (path[0] == '\0')
	{
		return 0;
	}

	for (int i = 0; path[i] != '\0'; i++)
	{
		if (IsPathSeparator(path[i]))
		{
			char ValueAtIndex = path[i];
			path[i] = '\0';
			Directory_Create(path);
			path[i] = ValueAtIndex;
		}
	}

	Directory_Create(path);

	return 0;
}

ArrayList* Directory_GetDirectoriesInPath(char* path)
{
	if (path == NULL)
	{
		return NULL;
	}

	ArrayList* List = ArrayList_Construct2();

	char DirectoryName[DIR_NAME_MAX_LENGTH] = "\0";
	int PathIndex = 0, DirIndex = 0;
	for (; path[PathIndex] != '\0'; PathIndex++, DirIndex++)
	{
		if (DirIndex >= DIR_NAME_MAX_LENGTH)
		{
			return NULL;
		}

		if ((path[PathIndex] == '\\') || (path[PathIndex] == '/'))
		{
			DirectoryName[DirIndex] = '\0';
			ArrayList_Add(List, String_CreateCopy(DirectoryName));

			DirIndex = -1;
			DirectoryName[0] = '\0';
		}
		else
		{
			DirectoryName[DirIndex] = path[PathIndex];
		}
	}

	if (DirectoryName[0] != '\0')
	{
		DirectoryName[DirIndex] = '\0';
		ArrayList_Add(List, String_CreateCopy(DirectoryName));
	}

	return List;
}

int Directory_Delete(char* path)
{
	if (path == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	bool Result = RemoveDirectory(path);
	return Result ? 0 : IO_ERROR_ERRCODE;
}