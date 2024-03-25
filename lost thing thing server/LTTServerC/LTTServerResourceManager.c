#include "LTTServerResourceManager.h"
#include "Directory.h"
#include "File.h"
#include "LTTServerC.h"
#include "Memory.h"
#include "LTTHTML.h"
#include <stdlib.h>


// Macros.
#define DIR_NAME_DATABASE "data"
#define DIR_NAME_SOURCE "source"

/* User account. */
#define ENTRY_ID_ACCOUNT_NAME 1
#define ENTRY_ID_ACCOUNT_SURNAME 2
#define ENTRY_ID_ACCOUNT_EMAIL 3
#define ENTRY_ID_ACCOUNT_PASSWORD 4
#define ENTRY_ID_ACCOUNT_PROFILE_IMAGE_ID 5
#define ENTRY_ID_ACCOUNT_POSTS 6


// Types.
typedef struct HTMLSource
{
	HTMLDocument BaseDocument;
	HTMLDocument LoggedInDocument;
	HTMLDocument LoggedOutDocument;
	const char* LoggedInData;
	const char* LoggedOutData;
} HTMLSource;

typedef struct CSSSourceFileStruct
{
	const char* Data;
	const char* EasterEggData;
} CSSSourceFile;

typedef struct JavascriptSourceFileStruct
{
	const char* Data;
} JavascriptSourceFile;


// Static functions.
static void AppendFolderNumberName(StringBuilder* builder, unsigned long long number)
{
	char NumberString[32];
	sprintf(NumberString, "%llu", number);
	StringBuilder_AppendChar(builder, PATH_SEPARATOR);
	StringBuilder_AppendChar(builder, NumberString);
}

/* Users. */
static unsigned long long GetAndUseUserID()
{
	unsigned long long ID = LTTServerC_GetCurrentContext()->Resources.AvailableAccountID;
	LTTServerC_GetCurrentContext()->Resources.AvailableAccountID += 1;
	return ID;
}

static void GetPathToUser(unsigned long long id)
{
	StringBuilder Builder;
	StringBuilder_Construct(&Builder);
	StringBuilder_Append(&Builder, LTTServerC_GetCurrentContext()->Resources.DatabaseRootPath)
}


// Functions.
void ResourceManager_ConstructContext(ServerResourceContext* context, const char* dataRootPath)
{
	context->DatabaseRootPath = Directory_CombinePaths(dataRootPath, DIR_NAME_DATABASE);
	context->SourceRootPath = Directory_CombinePaths(dataRootPath, DIR_NAME_SOURCE);
}

void ResourceManager_CloseContext()
{
	ServerResourceContext* Context = &LTTServerC_GetCurrentContext()->Resources;
	Memory_Free(Context->DatabaseRootPath);
	Memory_Free(Context->SourceRootPath);
}

ResourceResult ResourceManager_Get(ServerResourceRequest* request)
{
	return ResourceResult_Successful;
}

ResourceResult ResourceManager_Post(ServerResourceRequest* request)
{
	// --------------------------- REMOVE IN FINAL CODE OUTSIDE TESTS PLEASE!!! ---------------------------------- //
	if (String_Equals(request->Data, "stop"))      // <-------------
	{											// <-------------
		return ResourceResult_ShutDownServer;   // <-------------
	}											// <-------------
	// --------------------------- REMOVE IN FINAL CODE OUTSIDE TESTS PLEASE!!! ---------------------------------- //
	return ResourceResult_Successful;
}

ErrorCode ResourceManager_CreateAccountInDatabase(const char* name, const char* surname, const char* email, const char* password)
{
	unsigned long long ID = GetAndUseUserID();

	char* DataPath = 
}