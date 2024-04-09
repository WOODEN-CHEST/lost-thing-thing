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

ErrorCode Logger_Deconstruct(Logger* logger);

ErrorCode Logger_Log(Logger* logger, Logger_LogLevel level, const char* string);

ErrorCode Logger_LogInfo(Logger* logger, const char* string);

ErrorCode Logger_LogWarning(Logger* logger, const char* string);

ErrorCode Logger_LogError(Logger* logger, const char* string);

ErrorCode Logger_LogCritical(Logger* logger, const char* string);