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
	unsigned long long AvailablePostID;
	unsigned long long AvailableAccountID;
} ServerResourceContext;

typedef struct ServerResourceRequestStruct
{
	const char* Target;
	const char* Data;
	HttpCookie* CookieArray;
	size_t* CookieCount;
	char** ResultBody;
} ServerResourceRequest;


// Functions.
void ResourceManager_ConstructContext(ServerResourceContext* context, const char* dataRootPath);

void ResourceManager_CloseContext();

ResourceResult ResourceManager_Get(ServerResourceRequest* request);

ResourceResult ResourceManager_Post(ServerResourceRequest* request);

ErrorCode ResourceManager_CreateAccountInDatabase(const char* name, const char* surname, const char* email, const char* password);