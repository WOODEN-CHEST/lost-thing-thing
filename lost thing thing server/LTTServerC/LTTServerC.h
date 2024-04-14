#pragma once
#include "LTTErrors.h"

// Types.
typedef struct ServerContextStruct
{
	const char* ServerRootPath;
	struct LoggerStruct* Logger;
	struct ServerConfigStruct* Configuration;
	struct ServerResourceContextStruct* Resources;
	struct DBAccountContextStruct* AccountContext;
	struct DBPostContextStruct* PostContext;
} ServerContext;