#pragma once
#include <stddef.h>
#include "LttErrors.h"


// Macros.
#define SERVER_CONFIG_FILE_NAME "server.cfg"


// Types.
typedef struct ServerConfigStruct
{
	const char Address[32];
	const char** AcceptedEmailDomains;
	size_t AcceptedEmailDomainCount;
} ServerConfig;


// Functions.
ErrorCode ServerConfig_Read(const char* configPath, ServerConfig* config);