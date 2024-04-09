#pragma once
#include <stddef.h>
#include "LttErrors.h"


// Macros.
#define SERVER_CONFIG_FILE_NAME "server.cfg"


// Types.
typedef struct ServerConfigStruct
{
	char Address[64];

	const char** AcceptedDomains;
	size_t AcceptedDomainCount;
	size_t _acceptedDomainCapacity;
} ServerConfig;


// Functions.
Error ServerConfig_Read(ServerConfig* config, Logger* logger, const char* configPath);

void ServerConfig_Deconstruct(ServerConfig* config);