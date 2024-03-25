#pragma once
#include "Logger.h"
#include "LTTErrors.h"
#include "LTTServerResourceManager.h"

// Types.
typedef struct ServerContextStruct
{
	const char* RootPath;
	const char* GlobalDataFilePath;
	LoggerContext Logger;
	ErrorContext Errors;
	ServerResourceContext Resources;
} ServerContext;


// Functions.
ServerContext* LTTServerC_GetCurrentContext();