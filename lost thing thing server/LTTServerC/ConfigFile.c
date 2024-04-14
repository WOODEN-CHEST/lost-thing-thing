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

static Error SetAddress(ServerConfig* config, const char* address)
{
	if ((String_LengthBytes(address) + 1) > sizeof(config->Address))
	{
		return Error_CreateError(ErrorCode_InvalidConfigFile, "IP address too long in config file.");
	}

	String_CopyTo(address, config->Address);
	return Error_CreateSuccess();
}

static Error HandleConfigurationKeyValuePar(ServerConfig* config, Logger* logger, const char* key, const char* value)
{
	if (String_EqualsCaseInsensitive(key, KEY_EMAIL_DOMAIN))
	{
		AddDomain(config, value);
	}
	else if (String_EqualsCaseInsensitive(key, KEY_ADDRESS))
	{
		Error ReturnedError = SetAddress(config, value);
		if (ReturnedError.Code != ErrorCode_Success)
		{
			return ReturnedError;
		}
	}
	else
	{
		char Message[KEY_BUFFER_SIZE * 2];
		sprintf(Message, "Unknown configuration key found: \"%s\"", key);
		Logger_LogWarning(logger, Message);
	}

	return Error_CreateSuccess();
}

static Error ParseConfiguration(ServerConfig* config, Logger* logger, const char* configText)
{
	const char* ShiftedText = configText;

	while (*ShiftedText != '\0')
	{
		char Key[KEY_BUFFER_SIZE];
		ShiftedText = ParseUntil(ShiftedText, Key, sizeof(Key), VALUE_ASSIGNMENT_OPERATOR);

		if (*ShiftedText != VALUE_ASSIGNMENT_OPERATOR)
		{
			return Error_CreateError(ErrorCode_InvalidConfigFile, "Did not find value assignment operator '=' after key in config file.");
		}
		ShiftedText++;

		char Value[VALUE_BUFFER_SIZE];
		ShiftedText = ParseUntil(ShiftedText, Value, sizeof(Value), '\n');
		
		Error ReturnedError = HandleConfigurationKeyValuePar(config, logger, Key, Value);
		if (ReturnedError.Code != ErrorCode_Success)
		{
			return ReturnedError;
		}

		if (*ShiftedText == '\n')
		{
			ShiftedText++;
		}
	}

	return Error_CreateSuccess();
}

static void LoadDefaultConfig(ServerConfig* config)
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
Error ServerConfig_Read(ServerContext* serverContext, const char* configPath)
{
	ServerConfigConstruct(serverContext->Configuration);

	FILE* File = File_Open(configPath, FileOpenMode_Read, NULL);
	if (!File)
	{
		LoadDefaultConfig(serverContext->Configuration);
		Logger_LogWarning(serverContext->Logger, "No config file found, loading default config.");
		return Error_CreateSuccess();
	}

	const char* FileData = File_ReadAllText(File, NULL);
	File_Close(File);

	if (!FileData)
	{
		Logger_LogWarning(serverContext->Logger, "Config file found, but failed to read it's contents. Loading default config.");
		LoadDefaultConfig(serverContext->Configuration);
		return Error_CreateSuccess();
	}

	Error ReturnedError = ParseConfiguration(serverContext->Configuration, serverContext->Logger, FileData);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		Logger_LogWarning(serverContext->Logger, ReturnedError.Message);
		ServerConfig_Deconstruct(serverContext->Configuration); // Get rid of any data left while trying to parse config.
		ServerConfigConstruct(serverContext->Configuration); // Prepare for default config.
		LoadDefaultConfig(serverContext->Configuration);
	}

	return Error_CreateSuccess();
}

void ServerConfig_Deconstruct(ServerConfig* config)
{
	for (size_t i = 0; i < config->AcceptedDomainCount; i++)
	{
		Memory_Free((char*)config->AcceptedDomains[i]);
	}
	Memory_Free(config->AcceptedDomains);
}