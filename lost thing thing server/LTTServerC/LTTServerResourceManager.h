#pragma once
#include "HttpListener.h"
#include <stddef.h>


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


// Functions.
void ResourceManager_ConstructContext(ServerResourceContext* context, const char* dataRootPath);

void ResourceManager_CloseContext();

ResourceResult  ResourceManager_Get(const char* target, const char* data, HttpCookie* cookieArray, size_t cookieCount, const char** result);

ResourceResult  ResourceManager_Post(const char* target, const char* data, HttpCookie* cookieArray, size_t cookieCount);