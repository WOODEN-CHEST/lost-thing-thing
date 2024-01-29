#pragma once

#include "LttString.h"
#include "LTTErrors.h"

// Structures.
enum Logger_LogLevelEnum
{
	LogLevel_Info,
	LogLevel_Warning,
	LogLevel_Error,
	LogLevel_Critical
};

typedef enum Logger_LogLevelEnum Logger_LogLevel;


// Functions.
ErrorCode Logger_Initialize(const char* path);

_Bool Logger_IsInitialized();

ErrorCode Logger_Close();

ErrorCode Logger_Log(Logger_LogLevel level, const char* string);

ErrorCode Logger_LogInfo(const char* string);

ErrorCode Logger_LogWarning(const char* string);

ErrorCode Logger_LogError(const char* string);

ErrorCode Logger_LogCritical(const char* string);