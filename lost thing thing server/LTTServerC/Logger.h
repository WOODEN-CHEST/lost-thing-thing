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
char* LoggerContext_Construct(LoggerContext* context, const char* serverRootPath);

ErrorCode LoggerContext_Close();

ErrorCode LoggerContext_Log(Logger_LogLevel level, const char* string);

ErrorCode LoggerContext_LogInfo(const char* string);

ErrorCode LoggerContext_LogWarning(const char* string);

ErrorCode LoggerContext_LogError(const char* string);

ErrorCode LoggerContext_LogCritical(const char* string);