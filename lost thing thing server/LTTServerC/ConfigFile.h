#pragma once
#include <stddef.h>
#include "LttErrors.h"
#include "LTTServerC.h"


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
Error ServerConfig_Read(ServerContext* serverContext, const char* configPath);

void ServerConfig_Deconstruct(ServerConfig* config);