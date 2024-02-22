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


// Static variables.
static ServerContext s_currentContext;


// Static functions.
static char* CreateContext(ServerContext* context, const char* serverExecutablePath)
{
	ErrorContext_Construct(&context->Errors);
	char* RootDirectory = Directory_GetParentDirectory(serverExecutablePath);
	context->RootPath = RootDirectory;

	const char* ReturnErrorMessage = Logger_ConstructContext(&context->Logger, RootDirectory);
	if (ReturnErrorMessage)
	{
		return ReturnErrorMessage;
	}

	ResourceManager_ConstructContext(&context->Resources, RootDirectory);

	return NULL;
}

// Functions.
int main(int argc, const char** argv)
{
	// Create context.
	const char* ReturnErrorMessage = CreateContext(&s_currentContext, argv[0]);
	if (ReturnErrorMessage)
	{
		printf("Failed to create server context: %s", ReturnErrorMessage);
		return EXIT_FAILURE;
	}
	Logger_LogInfo("Created global server context");


	// Load config.
	ServerConfig Config;
	char* ConfigPath = Directory_Combine(LTTServerC_GetCurrentContext()->RootPath, SERVER_CONFIG_FILE_NAME);
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
	Logger_Close();
	Memory_Free(LTTServerC_GetCurrentContext()->RootPath);
	ResourceManager_CloseContext();

	return 0;
}

ServerContext* LTTServerC_GetCurrentContext()
{
	return &s_currentContext;
}