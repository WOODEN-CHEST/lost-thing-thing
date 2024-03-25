#include "LTTServerResourceManager.h"
#include "Directory.h"
#include "File.h"
#include "LTTServerC.h"
#include "Memory.h"
#include "LTTHTML.h"


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

}