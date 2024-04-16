#pragma once
#include "HttpListener.h"
#include <stddef.h>
#include "LTTErrors.h"
#include "LttString.h"


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
	
} ServerResourceContext;

typedef struct ServerResourceRequestStruct
{
	const char* Target;
	const char* Data;
	HttpCookie* CookieArray;
	size_t CookieCount;
	StringBuilder* ResultStringBuilder;
} ServerResourceRequest;


// Functions.
void ResourceManager_Construct(ServerResourceContext* context, const char* dataRootPath);

void ResourceManager_Deconstruct(ServerResourceContext* context);

ResourceResult ResourceManager_Get(ServerContext* serverContext, ServerResourceRequest* request);

ResourceResult ResourceManager_Post(ServerContext* serverContext, ServerResourceRequest* request);