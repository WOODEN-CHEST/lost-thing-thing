#include "Logger.h"
#include "File.h"
#include "ErrorCodes.h"
#include "Directory.h"
#include <stdbool.h>
#include <time.h>


// Variables.
static FILE* LogFile = NULL;


// Static functons.
static int Logger_AddDateTime(StringBuilder* builder)
{
	time_t CurrentTime = time(NULL);

	struct tm* DateTime = localtime(&CurrentTime);

	char Data[50];

	AtringBuilder_Append()
}

// Functions.
int Logger_Initialize(char* path)
{
	if (LogFile != NULL)
	{
		return;
	}
	if (path == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	Directory_CreateAll(path);
	File_Delete(path);
	int Result = File_Open(path, Write);

	return Result;
}

int Logger_Close()
{
	File_Close(LogFile);
	LogFile = NULL;
}

int Logger_Log(Logger_LogLevel level, char* string)
{
	if (LogFile == NULL)
	{
		return ILLEGAL_OPERATION_ERRCODE;
	}

	StringBuilder Builder;
	StringBuilder_Construct(&Builder, DEFAULT_STRING_BUILDER_CAPACITY);

	Logger_AddDateTime(&Builder);
}

int Logger_LogInfo(char* string)
{
	return Logger_Log(Info, string);
}

int Logger_LogWarning(char* string)
{
	return Logger_Log(Warning, string);
}

int Logger_LogError(char* string)
{
	return Logger_Log(Error, string);
}

int Logger_LogCritical(char* string)
{
	return Logger_Log(Critical, string);
}