#pragma once
#include "LTTErrors.h"

// Types.
typedef struct ServerContextStruct
{
	const char* RootPath;
	struct LoggerStruct* Logger;
	struct ServerConfigStruct* Configuration;
	struct ServerResourceContextStruct* Resources;
	struct DBAccountContextStruct* AccountContext;
} ServerContext;