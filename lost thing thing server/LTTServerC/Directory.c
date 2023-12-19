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
	char* Path = String_CreateCopy(path);

	for (int i = 0; Path[i] != '\0'; i++)
	{
		if (IsPathSeparator(Path[i]))
		{
			char ValueAtIndex = Path[i];
			Path[i] = '\0';
			Directory_Create(Path);
			Path[i] = ValueAtIndex;
		}
	}

	return 0;
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

char* Directory_GetParentDirectory(char* path)
{
	if (path == NULL)
	{
		return NULL;
	}

	char* PathCopy = String_CreateCopy(path);

	int LastSeparatorIndex = 0;
	for (int i = 0; PathCopy[i] != '\0'; i++)
	{
		if (IsPathSeparator(PathCopy[i]))
		{
			LastSeparatorIndex = i;
		}
	}

	PathCopy[LastSeparatorIndex] = '\0';
	return PathCopy;
}

char* Directory_Combine(char* path1, char* path2)
{
	if ((path1 == NULL) || (path2 == NULL))
	{
		return NULL;
	}

	StringBuilder Builder;
	StringBuilder_Construct(&Builder, DEFAULT_STRING_BUILDER_CAPACITY);

	StringBuilder_Append(&Builder, path1);
	StringBuilder_AppendChar(&Builder, PathSeparator);
	StringBuilder_Append(&Builder, path2);

	return Builder.Data;
}