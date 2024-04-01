#pragma once
#include "Logger.h"
#include "LTTErrors.h"
#include "LTTServerResourceManager.h"
#include "ConfigFile.h"

// Types.
typedef struct ServerContextStruct
{
	const char* RootPath;
	const char* GlobalDataFilePath;
	ServerConfig Configuration;
	LoggerContext Logger;
	ErrorContext Errors;
	ServerResourceContext Resources;
} ServerContext;


// Functions.
ServerContext* LTTServerC_GetCurrentContext();