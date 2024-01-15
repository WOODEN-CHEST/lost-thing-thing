#pragma once

#include "LttString.h"

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
int Logger_Initialize(const char* path);

_Bool Logger_IsInitialized();

int Logger_Close();

int Logger_Log(Logger_LogLevel level, const char* string);

int Logger_LogInfo(const char* string);

int Logger_LogWarning(const char* string);

int Logger_LogError(const char* string);

int Logger_LogCritical(const char* string);