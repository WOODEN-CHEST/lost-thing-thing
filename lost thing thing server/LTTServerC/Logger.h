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

typedef struct LoggerContextStruct
{
	FILE* LogFile;
	StringBuilder _logTextBuilder;
} LoggerContext;


// Functions.
char* Logger_ConstructContext(LoggerContext* context, const char* serverRootPath);

ErrorCode Logger_Close();

ErrorCode Logger_Log(Logger_LogLevel level, const char* string);

ErrorCode Logger_LogInfo(const char* string);

ErrorCode Logger_LogWarning(const char* string);

ErrorCode Logger_LogError(const char* string);

ErrorCode Logger_LogCritical(const char* string);