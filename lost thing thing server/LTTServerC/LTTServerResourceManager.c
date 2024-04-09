#include "LTTServerResourceManager.h"
#include "Directory.h"
#include "File.h"
#include "LTTServerC.h"
#include "Memory.h"
#include "LTTHTML.h"
#include <stdlib.h>
#include "GHDF.h"
#include "IDCodepointHashMap.h"
#include "LTTChar.h"
#include "LTTAccountManager.h"


// Macros.
#define DIR_NAME_DATABASE "data"
#define DIR_NAME_SOURCE "source"
#define DIR_NAME_POSTS "posts"

#define USER_ID_FOLDER_NAME_DIVIDER 10000


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
const char* ResourceManager_GetPathToIDFile(const char* rootPath, unsigned long long id)
{
	StringBuilder Builder;
	StringBuilder_Construct(&Builder, DEFAULT_STRING_BUILDER_CAPACITY);

	StringBuilder_Append(&Builder, rootPath);

	StringBuilder_AppendChar(&Builder, PATH_SEPARATOR);
	char NumberString[32];
	sprintf(NumberString, "%llu", id / USER_ID_FOLDER_NAME_DIVIDER);
	StringBuilder_Append(&Builder, NumberString);

	StringBuilder_AppendChar(&Builder, PATH_SEPARATOR);
	sprintf(NumberString, "%llu", id);
	StringBuilder_Append(&Builder, NumberString);
	StringBuilder_Append(&Builder, GHDF_FILE_EXTENSION);

	return Builder.Data;
}

void ResourceManager_Construct(ServerResourceContext* context, const char* dataRootPath)
{
	context->DatabaseRootPath = Directory_CombinePaths(dataRootPath, DIR_NAME_DATABASE);
	context->SourceRootPath = Directory_CombinePaths(dataRootPath, DIR_NAME_SOURCE);

	Directory_CreateAll(context->DatabaseRootPath);
	Directory_CreateAll(context->SourceRootPath);

	return Error_CreateSuccess();
}

void ResourceManager_Deconstruct(ServerResourceContext* context)
{
	SaveGlobalServerData(context);
	Memory_Free((char*)context->DatabaseRootPath);
	Memory_Free((char*)context->SourceRootPath);
}

ResourceResult ResourceManager_Get(ServerContext* context, ServerResourceRequest* request)
{
	return ResourceResult_Successful;
}

ResourceResult ResourceManager_Post(ServerContext* context, ServerResourceRequest* request)
{
	// --------------------------- REMOVE IN FINAL CODE OUTSIDE TESTS PLEASE!!! ---------------------------------- //
	if (String_Equals(request->Data, "stop"))   // <-------------
	{											// <-------------
		return ResourceResult_ShutDownServer;   // <-------------
	}											// <-------------
	// --------------------------- REMOVE IN FINAL CODE OUTSIDE TESTS PLEASE!!! ---------------------------------- //
	return ResourceResult_Successful;
}