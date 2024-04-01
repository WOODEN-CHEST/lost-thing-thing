#pragma once
#include "Logger.h"
#include "LTTErrors.h"
#include "LTTServerResourceManager.h"
#include "ConfigFile.h"

// Types.
typedef struct ServerContextStruct
{
	const char* RootPath;
	ErrorContext Errors;
	LoggerContext Logger;
	ServerConfig Configuration;
	ServerResourceContext Resources;
} ServerContext;


// Functions.
ServerContext* LTTServerC_GetCurrentContext();