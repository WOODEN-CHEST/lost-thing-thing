#pragma once

#include "LttString.h"

// Structures.
enum Logger_LogLevelEnum
{
	Info = 0,
	Warning = 2,
	Error = 3,
	Critical = 4
};

typedef enum Logger_LogLevelEnum Logger_LogLevel;


// Functions.
int Logger_Initialize(char* path);

int Logger_Close();

int Logger_Log(Logger_LogLevel level, char* string);

int Logger_LogInfo(char* string);

int Logger_LogWarning(char* string);

int Logger_LogError(char* string);

int Logger_LogCritical(char* string);