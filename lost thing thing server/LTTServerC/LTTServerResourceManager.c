#include "LTTServerResourceManager.h"
#include "Directory.h"
#include "File.h"
#include "LTTServerC.h"
#include "Memory.h"


// Macros.
#define DIR_NAME_DATABASE "data"
#define DIR_NAME_SOURCE "source"


// Static functions.



// Functions.
void ResourceManager_ConstructContext(ServerResourceContext* context, const char* dataRootPath)
{
	context->DatabaseRootPath = Directory_Combine(dataRootPath, DIR_NAME_DATABASE);
	context->SourceRootPath = Directory_Combine(dataRootPath, DIR_NAME_SOURCE);
}

void ResourceManager_CloseContext()
{
	ServerResourceContext* Context = &LTTServerC_GetCurrentContext()->Resources;
	Memory_Free(Context->DatabaseRootPath);
	Memory_Free(Context->SourceRootPath);
}

ResourceResult ResourceManager_Get(const char* target, const char* data, HttpCookie* cookieArray, size_t cookieCount, const char** result)
{
	*result = "{\"text\":\"Hello World!\"}";
	return ResourceResult_Successful;
}

ResourceResult ResourceManager_Post(const char* target, const char* data, HttpCookie* cookieArray, size_t cookieCount)
{
	return ResourceResult_Successful;
}