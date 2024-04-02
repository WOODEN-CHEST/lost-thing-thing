#pragma once
#include "HttpListener.h"
#include <stddef.h>
#include "IDCodepointHashMap.h"
#include "LTTErrors.h"
#include "LttString.h"
#include "LTTAccountManager.h"


// Types.
typedef enum ResourceResultEnum
{
	ResourceResult_Successful,
	ResourceResult_NotFound,
	ResourceResult_Invalid,
	ResourceResult_Unauthorized,
	ResourceResult_ShutDownServer,
} ResourceResult;

typedef struct ServerResourceContextStruct
{
	const char* SourceRootPath;
	const char* DatabaseRootPath;
	const char* GlobalDataFilePath;

	DBAccountContext AccountContext;

	unsigned long long AvailablePostID;
	IDCodepointHashMap PostTitleMap;
	
} ServerResourceContext;

typedef struct ServerResourceRequestStruct
{
	const char* Target;
	const char* Data;
	HttpCookie* CookieArray;
	size_t* CookieCount;
	StringBuilder** Result;
} ServerResourceRequest;


// Functions.
ErrorCode ResourceManager_ConstructContext(ServerResourceContext* context, const char* dataRootPath);

void ResourceManager_CloseContext(ServerResourceContext* context);

ResourceResult ResourceManager_Get(ServerResourceRequest* request);

ResourceResult ResourceManager_Post(ServerResourceRequest* request);

const char* ResourceManager_GetPathToIDFile(unsigned long long id, const char* dirName);