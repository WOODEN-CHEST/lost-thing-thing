#include "ConfigFile.h"
#include "File.h"
#include "String.h"
#include "Memory.h"
#include <stdio.h>

// Macros.
#define DEFAULT_EMAIL_DOMAIN "marupe.edu.lv"
#define DEFAULT_ADDRESS "127.0.0.1"


// Static functions.
static void LoadDefaultConfig(ServerConfig* config)
{
	config->AcceptedEmailDomainCount = 1;
	char** DomainNameArray = (char**)Memory_SafeMalloc(sizeof(char**));
	DomainNameArray[0] = DEFAULT_EMAIL_DOMAIN;
	config->AcceptedEmailDomains = DomainNameArray;
	String_CopyTo(DEFAULT_ADDRESS, config->Address);
}


// Functions.
ErrorCode ServerConfig_Read(const char* configPath, ServerConfig* config)
{
	FILE* File = File_Open(configPath, FileOpenMode_Read);
	if (File == NULL)
	{
		LoadDefaultConfig(config);
		return ErrorCode_Success;
	}

	const char* FileData = File_ReadAllText(File);
	File_Close(File);

	if (!FileData)
	{
		LoadDefaultConfig(config);
	}
	return ErrorCode_Success;
}