#pragma once
#include "HttpListener.h"
#include <stddef.h>


// Types.
typedef enum ResourceResultEnum
{
	ResourceResult_Successful,
	ResourceResult_NotFouund,
	ResourceResult_Invalid,
	ResourceResult_Unauthorized
} ResourceResult;


// Functions.
void ResourceManager_Initialize(const char* serverRootPath);

void ResourceManager_Shutdown(const char* serverRootPath);

ResourceResult  ResourceManager_Get(const char* target, const char* data, HttpCookie* cookieArray, size_t cookieCount, const char** result);

ResourceResult  ResourceManager_Post(const char* target, const char* data, HttpCookie* cookieArray, size_t cookieCount);