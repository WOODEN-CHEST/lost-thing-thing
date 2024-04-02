#include "LTTServerC.h"
#include <stdbool.h>
#include "File.h"
#include <stdio.h>
#include "HttpListener.h"
#include <signal.h>
#include <Stdlib.h>
#include "Directory.h"
#include "Memory.h"
#include "Logger.h"
#include "GHDF.h"
#include "LTTChar.h"
#include "IDCodepointHashMap.h"


// Static variables.
static ServerContext s_currentContext;


// Static functions.
static const char* CreateContext(ServerContext* context, const char* serverExecutablePath)
{
	// Errors.
	Error_ConstructContext(&context->Errors);

	// Paths.
	context->RootPath = Directory_GetParentDirectory(serverExecutablePath);

	// Logger.
	const char* ReturnErrorMessage = Logger_ConstructContext(&context->Logger, context->RootPath);
	if (ReturnErrorMessage)
	{
		return ReturnErrorMessage;
	}
	Logger_LogInfo("Initialized logger.");

	// Configuration.
	char* ConfigPath = Directory_CombinePaths(LTTServerC_GetCurrentContext()->RootPath, SERVER_CONFIG_FILE_NAME);
	ServerConfig_Read(ConfigPath, &LTTServerC_GetCurrentContext()->Configuration);
	Memory_Free(ConfigPath);
	Logger_LogInfo("Read configuration.");

	// Database.
	if (ResourceManager_ConstructContext(&context->Resources, context->RootPath) != ErrorCode_Success)
	{
		return Error_GetLastErrorMessage();
	}
	Logger_LogInfo("Constructed database context.");

	return NULL;
}

static void CloseContext()
{
	ResourceManager_CloseContext(&LTTServerC_GetCurrentContext()->Resources);
	Logger_CloseContext(&LTTServerC_GetCurrentContext()->Logger);
	Memory_Free(LTTServerC_GetCurrentContext()->RootPath);
	Error_CloseContext(&LTTServerC_GetCurrentContext()->Errors);
}

static void Test(IDCodepointHashMap* map)
{
	FILE* File = File_Open("C:\\Users\\User\\Desktop\\test.txt", FileOpenMode_Read);
	char* Text = File_ReadAllText(File);
	File_Close(File);

	int ID = 0;
	int StringStartIndex = 0;
	for (int i = 0; Text[i] != '\0'; i++)
	{
		if (Text[i] == '\n')
		{
			Text[i] = '\0';
			i++;

			IDCodepointHashMap_AddID(map, Text + StringStartIndex, ID);
			StringStartIndex = i;
			ID++;
		}
	}
}

static int RunServer(const char* executablePath)
{
	// Create context.
	const char* ReturnErrorMessage = CreateContext(&s_currentContext, executablePath);
	if (ReturnErrorMessage)
	{
		printf("Failed to create server context: %s", ReturnErrorMessage);
		return EXIT_FAILURE;
	}

	UserAccount Account;
	//AccountManager_TryCreateUser(&Account, "Kristofers", "Cernavskis", "kristofers.cernavskis@marupe.edu.lv", "1234");
	//AccountManager_TryCreateUser(&Account, "Keigers", "Klibers", "KeigersKlibers@gmail.com", "1234");
	//AccountManager_GetAccount(&Account, 0);


	// Start server.
 	ErrorCode Error = HttpListener_Listen(LTTServerC_GetCurrentContext()->Configuration.Address);
	if (Error != ErrorCode_Success)
	{
		Logger_LogError(Error_GetLastErrorMessage());
	}

	// Stop server.
	Logger_LogInfo("Server closed.");
	CloseContext();

	return EXIT_SUCCESS;
}

// Functions.
int main(int argc, const char** argv)
{
	return RunServer(argv[0]);
}

ServerContext* LTTServerC_GetCurrentContext()
{
	return &s_currentContext;
}