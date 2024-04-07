#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Directory.h"
#include "LttString.h"
#include "Memory.h"
#include "LTTErrors.h"
#include <sys/stat.h>
#include <stdbool.h>


// Macros.
#define DIR_NAME_MAX_LENGTH 512
#define EXTENSION_SPEARATOR '.'


// Functions.
_Bool Directory_Exists(const char* path)
{
	struct stat DirectoryStats;
	int Result = stat(path, &DirectoryStats);

	return Result == 0;
}

ErrorCode Directory_Create(const char* path)
{
	bool Result = CreateDirectoryA(path, NULL);
	return Result ? ErrorCode_Success : Error_SetError(ErrorCode_IO, "Directory_Create: Failed to create directory.");
}

ErrorCode Directory_CreateAll(const char* path)
{
	if (path[0] == '\0')
	{
		return ErrorCode_Success;
	}
	char* Path = String_CreateCopy(path);

	for (int i = 0; Path[i] != '\0'; i++)
	{
		if (Directory_IsPathSeparator(Path[i]))
		{
			char ValueAtIndex = Path[i];
			Path[i] = '\0';
			CreateDirectoryA(Path, NULL);
			Path[i] = ValueAtIndex;
		}
	}
	Directory_Create(path);

	Memory_Free(Path);

	return ErrorCode_Success;
}

ErrorCode Directory_Delete(const char* path)
{
	bool Result = RemoveDirectoryA(path);
	return Result ? ErrorCode_Success : Error_SetError(ErrorCode_IO, "Directory_Delete: Failed to delete directory.");
}

char* Directory_GetParentDirectory(const char* path)
{
	char* PathCopy = String_CreateCopy(path);

	int LastSeparatorIndex = 0;
	for (int i = 0; PathCopy[i] != '\0'; i++)
	{
		if (Directory_IsPathSeparator(PathCopy[i]))
		{
			LastSeparatorIndex = i;
		}
	}

	PathCopy[LastSeparatorIndex] = '\0';
	return PathCopy;
}

char* Directory_CombinePaths(const char* path1, const char* path2)
{
	StringBuilder Builder;
	StringBuilder_Construct(&Builder, DEFAULT_STRING_BUILDER_CAPACITY);

	StringBuilder_Append(&Builder, path1);
	StringBuilder_AppendChar(&Builder, PATH_SEPARATOR);
	StringBuilder_Append(&Builder, path2);

	return Builder.Data;
}

char* Directory_GetName(const char* path)
{
	int LastSeparatorIndex = 0;
	int Length = 0;
	for (; path[Length] != '\0'; Length++)
	{
		if (Directory_IsPathSeparator(path[Length]))
		{
			LastSeparatorIndex = Length;
		}
		Length++;
	}

	return String_SubString(path, LastSeparatorIndex + 1, Length);
}

char* Directory_ChangePathExtension(const char* path, const char* newExtension)
{
	StringBuilder Builder;
	StringBuilder_Construct(&Builder, DEFAULT_STRING_BUILDER_CAPACITY);
	StringBuilder_Append(&Builder, path);

	if (String_EndsWith(Builder.Data, newExtension))
	{
		return Builder.Data;
	}

	long long InsertionIndex = -1;
	for (long long i = (long long)Builder.Length - 1; (i >= 0) && !Directory_IsPathSeparator(Builder.Data[i]); i--)
	{
		if (Builder.Data[i] == EXTENSION_SPEARATOR)
		{
			InsertionIndex = i;
			break;
		}
	}

	if (InsertionIndex != -1)
	{
		StringBuilder_Remove(&Builder, InsertionIndex, Builder.Length);
	}
	StringBuilder_Append(&Builder, newExtension);
	return Builder.Data;
}