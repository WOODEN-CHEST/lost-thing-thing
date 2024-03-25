#include <stdbool.h>
#include "LttCommon.h"
#include "File.h"
#include <stdio.h>
#include "HttpListener.h"
#include <signal.h>
#include <Stdlib.h>
#include "ConfigFile.h"
#include "Directory.h"
#include "Memory.h"
#include "LTTServerC.h"
#include "Logger.h"
#include "GHDF.h"
#include "LTTChar.h"


// Macros.
#define SERVER_GLOBAL_DATA_FILENAME "global_data" GHDF_FILE_EXTENSION

#define GLOBAL_DATA_ID_POST_ID 1
#define GLOBAL_DATA_ID_ACCOUNT_ID 2


// Static variables.
static ServerContext s_currentContext;


// Static functions.
static const char* GenerateDefaultGlobalServerData(ServerContext* context)
{
	context->Resources.AvailablePostID = 0;
	context->Resources.AvailableAccountID = 0;
}

static const char* ReadGlobalServerData(ServerContext* context)
{
	if (!File_Exists(context->GlobalDataFilePath))
	{
		Logger_LogWarning("Global server data doesn't exist, using default one.");
		GenerateDefaultGlobalServerData(context);
		return;
	}

	GHDFCompound DataCompound;
	if (GHDFCompound_ReadFromFile(context->GlobalDataFilePath, &DataCompound) != ErrorCode_Success)
	{
		char* Message = (char*)Memory_SafeMalloc(sizeof(char) * 256);
		sprintf(Message, "Failed to read global server data even though it exists: %s", Error_GetLastErrorMessage());
		return Message;
	}

	// Pain error checking every value.
	GHDFEntry* Entry = GHDFCompound_GetEntry(&DataCompound, GLOBAL_DATA_ID_POST_ID);
	if (!Entry)
	{
		return "Post ID entry not found.";
	}
	if (Entry->ValueType != GHDFType_ULong)
	{
		return "Post ID entry not of type unsigned long.";
	}
	context->Resources.AvailablePostID = Entry->Value.SingleValue.ULong;

	Entry = GHDFCompound_GetEntry(&DataCompound, GLOBAL_DATA_ID_ACCOUNT_ID);
	if (!Entry)
	{
		return "Account ID entry not found.";
	}
	if (Entry->ValueType != GHDFType_ULong)
	{
		return "Account ID entry not of type unsigned long.";
	}
	context->Resources.AvailableAccountID = Entry->Value.SingleValue.ULong;

	GHDFCompound_Deconstruct(&DataCompound);
}

static void SaveGlobalServerData()
{
	GHDFCompound Compound;
	GHDFCompound_Construct(&Compound, COMPOUND_DEFAULT_CAPACITY);
	GHDFPrimitive Value;

	Value.ULong = LTTServerC_GetCurrentContext()->Resources.AvailablePostID;
	GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_ULong, GLOBAL_DATA_ID_POST_ID, Value);

	Value.ULong = LTTServerC_GetCurrentContext()->Resources.AvailableAccountID;
	GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_ULong, GLOBAL_DATA_ID_ACCOUNT_ID, Value);

	if (GHDFCompound_WriteToFile(LTTServerC_GetCurrentContext()->GlobalDataFilePath, &Compound) != ErrorCode_Success)
	{
		Logger_LogCritical(Error_GetLastErrorMessage());
	}
	GHDFCompound_Deconstruct(&Compound);
}

static const char* CreateContext(ServerContext* context, const char* serverExecutablePath)
{
	Error_Construct(&context->Errors);
	context->RootPath = Directory_GetParentDirectory(serverExecutablePath);
	context->GlobalDataFilePath = Directory_CombinePaths(context->RootPath, SERVER_GLOBAL_DATA_FILENAME);

	context->Logger.LogFile = NULL;
	const char* ReturnErrorMessage = Logger_ConstructContext(&context->Logger, context->RootPath);
	if (ReturnErrorMessage)
	{
		return ReturnErrorMessage;
	}

	ResourceManager_ConstructContext(&context->Resources, context->RootPath);

	ReturnErrorMessage = ReadGlobalServerData(context);
	if (ReturnErrorMessage)
	{
		return ReturnErrorMessage;
	}

	return NULL;
}

static void CloseContext()
{
	SaveGlobalServerData();

	Logger_Close();
	Memory_Free(LTTServerC_GetCurrentContext()->RootPath);
	Memory_Free(LTTServerC_GetCurrentContext()->GlobalDataFilePath);
	ResourceManager_CloseContext();
}

// Functions.
int main(int argc, const char** argv)
{
	// Create context.
	const char* ReturnErrorMessage = CreateContext(&s_currentContext, argv[0]);
	if (ReturnErrorMessage)
	{
		if (s_currentContext.Logger.LogFile)
		{
			Logger_Close();
		}
		
		printf("Failed to create server context: %s", ReturnErrorMessage);
		return EXIT_FAILURE;
	}
	Logger_LogInfo("Created global server context");


	// Load config.
	ServerConfig Config;
	char* ConfigPath = Directory_CombinePaths(LTTServerC_GetCurrentContext()->RootPath, SERVER_CONFIG_FILE_NAME);
	ServerConfig_Read(ConfigPath, &Config);
	Logger_LogInfo("Read config");

	// Start server.
	ErrorCode Error = HttpListener_Listen();
	if (Error != ErrorCode_Success)
	{
		Logger_LogInfo(Error_GetLastErrorMessage());
	}

	// Stop server.
	Logger_LogInfo("Server closed.");
	CloseContext();

	return 0;
}

ServerContext* LTTServerC_GetCurrentContext()
{
	return &s_currentContext;
}