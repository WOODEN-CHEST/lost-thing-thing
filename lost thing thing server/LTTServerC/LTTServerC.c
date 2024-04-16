#include "LTTServerC.h"
#include <stdbool.h>
#include "File.h"
#include <stdio.h>
#include "HttpListener.h"
#include <Stdlib.h>
#include "Directory.h"
#include "Memory.h"
#include "Logger.h"
#include "IDCodepointHashMap.h"
#include "ConfigFile.h"
#include "LTTServerResourceManager.h"
#include "LTTAccountManager.h"
#include "Logger.h"
#include "LTTPostManager.h"
#include "LTTSMTP.h"


// Static variables.
static ServerContext s_currentContext;


// Static functions
static void CloseContext(ServerContext* context)
{
	if (context->AccountContext)
	{
		AccountManager_Deconstruct(context->AccountContext);
		Memory_Free(context->AccountContext);
	}
	if (context->Resources)
	{
		ResourceManager_Deconstruct(context->Resources);
		Memory_Free(context->Resources);
	}
	if (context->Configuration)
	{
		ServerConfig_Deconstruct(context->Configuration);
		Memory_Free(context->Configuration);
	}
	if (context->Logger)
	{
		Logger_LogInfo(context->Logger, "Closing logger context.");
		Logger_Deconstruct(context->Logger);
		Memory_Free(context->Logger);
	}
	if (context->ServerRootPath)
	{
		Memory_Free((char*)context->ServerRootPath);
	}
	if (context->PostContext)
	{
		PostManager_Deconstruct(context->PostContext);
		Memory_Free(context->PostContext);
	}
}

static Error CreateContext(ServerContext* context, const char* serverExecutablePath)
{
	Memory_Set((char*)context, sizeof(ServerContext), 0);

	// Paths.
	context->ServerRootPath = Directory_GetParentDirectory(serverExecutablePath);

	// Logger.
	context->Logger = (Logger*)Memory_SafeMalloc(sizeof(Logger));
	Error ReturnedError = Logger_Construct(context->Logger, context->ServerRootPath);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	Logger_LogInfo(context->Logger, "Initialized logger.");

	// Configuration.
	context->Configuration = (ServerConfig*)Memory_SafeMalloc(sizeof(ServerConfig));
	char* ConfigPath = Directory_CombinePaths(context->ServerRootPath, SERVER_CONFIG_FILE_NAME);
	ReturnedError = ServerConfig_Read(context, ConfigPath);
	Memory_Free(ConfigPath);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		CloseContext(context);
		return ReturnedError;
	}
	Logger_LogInfo(context->Logger, "Read configuration.");

	// Database.
	context->Resources = (ServerResourceContext*)Memory_SafeMalloc(sizeof(ServerResourceContext));
	ResourceManager_Construct(context->Resources, context->ServerRootPath);

	context->AccountContext = (DBAccountContext*)Memory_SafeMalloc(sizeof(DBAccountContext));
	ReturnedError = AccountManager_Construct(context);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		CloseContext(context);
		return ReturnedError;
	}

	context->PostContext = (DBPostContext*)Memory_SafeMalloc(sizeof(DBPostContext));
	ReturnedError = PostManager_Construct(context);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		CloseContext(context);
		return ReturnedError;
	}

	Logger_LogInfo(context->Logger, "Constructed database context.");

	

	return Error_CreateSuccess();
}

static void CreateDummyAccounts(ServerContext* context)
{
	Error ReturnedError;
	AccountManager_TryCreateAccount(context, "John", "Doe", "john.doe@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Alice", "Smith", "alice.smith@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Michael", "Johnson", "michael.johnson@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Emily", "Brown", "emily.brown@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Daniel", "Martinez", "daniel.martinez@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Emma", "Garcia", "emma.garcia@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "William", "Davis", "william.davis@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Olivia", "Rodriguez", "olivia.rodriguez@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "James", "Wilson", "james.wilson@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Sophia", "Lopez", "sophia.lopez@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Benjamin", "Hernandez", "benjamin.hernandez@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Mia", "Lee", "mia.lee@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Elijah", "Walker", "elijah.walker@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Charlotte", "Perez", "charlotte.perez@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Ethan", "Hall", "ethan.hall@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Amelia", "Young", "amelia.young@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Alexander", "Scott", "alexander.scott@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Harper", "Green", "harper.green@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Henry", "Adams", "henry.adams@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Abigail", "Baker", "abigail.baker@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Sebastian", "Campbell", "sebastian.campbell@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Avery", "Evans", "avery.evans@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Joseph", "Clark", "joseph.clark@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Sofia", "Nelson", "sofia.nelson@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Jackson", "Thomas", "jackson.thomas@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Madison", "Hill", "madison.hill@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "David", "Mitchell", "david.mitchell@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Chloe", "Roberts", "chloe.roberts@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Carter", "Carter", "carter.carter@marupe.edu.lv", "123456789", &ReturnedError);
	AccountManager_TryCreateAccount(context, "Lillian", "Flores", "lillian.flores@marupe.edu.lv", "123456789", &ReturnedError);
}

static int RunServer(const char* executablePath)
{
	// Create context.
	ServerContext Context;

	Error ReturnedError = CreateContext(&Context, executablePath);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		printf("Failed to create server context: %s", ReturnedError.Message);
		return EXIT_FAILURE;
	}

	ReturnedError = HttpListener_Listen(&Context);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		Logger_LogCritical(Context.Logger, ReturnedError.Message);
		Error_Deconstruct(&ReturnedError);
	}


	// Stop server.
	Logger_LogInfo(Context.Logger, "Server closing.");
	CloseContext(&Context);

	return EXIT_SUCCESS;
}

// Functions.
int main(int argc, const char** argv)
{
	return RunServer(argv[0]);
}