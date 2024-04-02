#include "ConfigFile.h"
#include "File.h"
#include "String.h"
#include "Memory.h"
#include <stdio.h>
#include "Logger.h"
#include "LTTErrors.h"
#include <stddef.h>
#include "LTTChar.h"

// Macros.
#define DEFAULT_EMAIL_DOMAIN "marupe.edu.lv"
#define DEFAULT_ADDRESS "127.0.0.1"

#define DOMAIN_LIST_CAPACITY 4
#define DOMAIN_LIST_GROWTH 2

#define KEY_BUFFER_SIZE 128
#define VALUE_BUFFER_SIZE 256

#define VALUE_ASSIGNMENT_OPERATOR '='

#define KEY_ADDRESS "address"
#define KEY_EMAIL_DOMAIN "email-domain"


// Static functions.
/* Parsing functions. */
static const char* ParseUntil(const char* text, char* parsedBuffer, size_t bufferSize, char targetCharacter)
{
	size_t Index;
	for (Index = 0; (Index < (bufferSize - 1)) && (text[Index] != '\0'); Index++)
	{
		if (text[Index] == targetCharacter)
		{
			break;
		}
		parsedBuffer[Index] = text[Index];
	}
	parsedBuffer[Index] = '\0';

	return text + Index;
}

static const char* SkipWhitespace(const char* text)
{
	int Index;
	for (Index = 0; (text[Index] != '\0') && Char_IsWhitespace(text + Index); Index++)
	{
		continue;
	}

	return text + Index;
}

/* Loading config. */
static void EnsureDomainCapacity(ServerConfig* config, size_t capacity)
{
	if (config->_acceptedDomainCapacity >= capacity)
	{
		return;
	}

	while (config->_acceptedDomainCapacity < capacity)
	{
		config->_acceptedDomainCapacity *= DOMAIN_LIST_GROWTH;
	}
	config->AcceptedDomains = (char**)Memory_SafeRealloc(config->AcceptedDomains, sizeof(char**) * config->_acceptedDomainCapacity);
}

static void AddDomain(ServerConfig* config, const char* domain)
{
	EnsureDomainCapacity(config, config->AcceptedDomainCount + 1);
	config->AcceptedDomains[config->AcceptedDomainCount] = String_CreateCopy(domain);
	config->AcceptedDomainCount += 1;
}

static ErrorCode SetAddress(ServerConfig* config, const char* address)
{
	if ((String_LengthBytes(address) + 1) > sizeof(config->Address))
	{
		return Error_SetError(ErrorCode_InvalidConfigFile, "IP address too long in config file.");
	}

	String_CopyTo(address, config->Address);
	return ErrorCode_Success;
}

static ErrorCode HandleConfigurationKeyValuePar(ServerConfig* config, const char* key, const char* value)
{
	if (String_Equals(key, KEY_EMAIL_DOMAIN))
	{
		AddDomain(config, value);
	}
	else if (String_Equals(key, KEY_ADDRESS))
	{
		if (SetAddress(config, value) != ErrorCode_Success)
		{
			return Error_GetLastErrorCode();
		}
	}
	else
	{
		char Message[KEY_BUFFER_SIZE * 2];
		sprintf(Message, "Unknown configuration key found: \"%s\"", key);
		Logger_LogWarning(Message);
	}

	return ErrorCode_Success;
}

static ErrorCode ParseConfiguration(ServerConfig* config, const char* configText)
{
	const char* ShiftedText = configText;

	while (*ShiftedText != '\0')
	{
		char Key[KEY_BUFFER_SIZE];
		ShiftedText = ParseUntil(ShiftedText, Key, sizeof(Key), VALUE_ASSIGNMENT_OPERATOR);

		if (*ShiftedText != VALUE_ASSIGNMENT_OPERATOR)
		{
			return Error_SetError(ErrorCode_InvalidConfigFile, "Did not find value assignment operator '=' after key in config file.");
		}
		ShiftedText++;

		char Value[VALUE_BUFFER_SIZE];
		ShiftedText = ParseUntil(ShiftedText, Value, sizeof(Value), '\n');
		
		if (HandleConfigurationKeyValuePar(config, Key, Value) != ErrorCode_Success)
		{
			return Error_GetLastErrorCode();
		}

		if (*ShiftedText == '\n')
		{
			ShiftedText++;
		}
	}

	return ErrorCode_Success;
}

static ErrorCode LoadDefaultConfig(ServerConfig* config)
{
	config->AcceptedDomainCount = 1;
	config->AcceptedDomains[0] = String_CreateCopy(DEFAULT_EMAIL_DOMAIN);

	String_CopyTo(DEFAULT_ADDRESS, config->Address);
}

static void ServerConfigConstruct(ServerConfig* config)
{
	config->AcceptedDomainCount = 0;
	config->AcceptedDomains = (char**)Memory_SafeMalloc(sizeof(char**) * DOMAIN_LIST_CAPACITY);
	config->_acceptedDomainCapacity = DOMAIN_LIST_CAPACITY;
	config->Address[0] = '\0';
}


// Functions.
ErrorCode ServerConfig_Read(const char* configPath, ServerConfig* config)
{
	ServerConfigConstruct(config);

	FILE* File = File_Open(configPath, FileOpenMode_Read);
	if (!File)
	{
		LoadDefaultConfig(config);
		Logger_LogWarning("No config file found, loading default config.");
		return ErrorCode_Success;
	}

	const char* FileData = File_ReadAllText(File);
	File_Close(File);

	if (!FileData)
	{
		Logger_LogWarning("Config file found, but failed to read it's contents. Loading default config.");
		LoadDefaultConfig(config);
	}

	if (ParseConfiguration(config, FileData) != ErrorCode_Success)
	{
		ServerConfig_Deconstruct(config); // Get rid of any data left while trying to parse config.
		ServerConfigConstruct(config); // Prepare for default config.
		LoadDefaultConfig(config);
	}

	return ErrorCode_Success;
}

void ServerConfig_Deconstruct(ServerConfig* config)
{
	for (size_t i = 0; i < config->AcceptedDomainCount; i++)
	{
		Memory_Free(config->AcceptedDomains[i]);
	}
	Memory_Free(config->AcceptedDomains);
}