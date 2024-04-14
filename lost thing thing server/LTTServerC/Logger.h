#pragma once

#include "LttString.h"
#include "LTTErrors.h"
#include <stdio.h>

// Structures.
enum Logger_LogLevelEnum
{
	LogLevel_Info,
	LogLevel_Warning,
	LogLevel_Error,
	LogLevel_Critical
};

typedef enum Logger_LogLevelEnum Logger_LogLevel;

typedef struct LoggerStruct
{
	FILE* LogFile;
	StringBuilder _logTextBuilder;
} Logger;


// Functions.
Error Logger_Construct(Logger* logger, const char* serverRootPath);

Error Logger_Deconstruct(Logger* logger);

Error Logger_Log(Logger* logger, Logger_LogLevel level, const char* string);

Error Logger_LogInfo(Logger* logger, const char* string);

Error Logger_LogWarning(Logger* logger, const char* string);

Error Logger_LogError(Logger* logger, const char* string);

Error Logger_LogCritical(Logger* logger, const char* string);