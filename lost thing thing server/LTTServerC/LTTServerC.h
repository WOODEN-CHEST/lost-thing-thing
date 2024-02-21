#pragma once
#include "Logger.h"
#include "LTTErrors.h"

// Types.
typedef struct ServerContextStruct
{
	const char* RootPath;
	LoggerContext Logger;
	ErrorContext Errors;
} ServerContext;


// Functions.
ServerContext* LTTServerC_GetCurrentContext();