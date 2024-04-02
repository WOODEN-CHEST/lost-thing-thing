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

/* Mega source-code file which hold it all together, somehow. */

// Macros.
#define DIR_NAME_DATABASE "data"
#define DIR_NAME_SOURCE "source"
#define DIR_NAME_POSTS "posts"
#define SERVER_GLOBAL_DATA_FILENAME "global_data" GHDF_FILE_EXTENSION

#define USER_ID_FOLDER_NAME_DIVIDER 10000

#define DEFAULT_AVAILABLE_ACCOUNT_ID 0;
#define DEFAULT_AVAILABLE_POST_ID 0;
#define DEFAULT_AVAILABLE_ACCOUNT_IMAGE_ID 1;
#define ENTRY_ID_POST_ID 1
#define ENTRY_ID_ACCOUNT_ID 2
#define ENTRY_ID_ACCOUNT_IMAGE_ID 3



/* User account. */



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
/* Global data. */
static void ReadGlobalData(const char* filePath, unsigned long long* accountID, unsigned long long* accountImageID, unsigned long long* postID)
{
	*accountID = DEFAULT_AVAILABLE_ACCOUNT_ID;
	*postID = DEFAULT_AVAILABLE_POST_ID;
	*accountImageID = DEFAULT_AVAILABLE_ACCOUNT_IMAGE_ID;

	if (!File_Exists(filePath))
	{
		Logger_LogWarning("Failed to find global data file, using default data.");
		return;
	}

	GHDFCompound Compound;
	if (GHDFCompound_ReadFromFile(filePath, &Compound) != ErrorCode_Success)
	{
		Logger_LogWarning("Failed to read global data file even though it exists, using default data.");
		return;
	}

	GHDFEntry* Entry;
	if (GHDFCompound_GetVerifiedEntry(&Compound, ENTRY_ID_POST_ID, &Entry, GHDFType_ULong, "Global Post ID") != ErrorCode_Success)
	{
		Logger_LogWarning(Error_GetLastErrorMessage());
		return;
	}
	*postID = Entry->Value.SingleValue.ULong;

	if (GHDFCompound_GetVerifiedEntry(&Compound, ENTRY_ID_ACCOUNT_ID, &Entry, GHDFType_ULong, "Global Account ID") != ErrorCode_Success)
	{
		Logger_LogWarning(Error_GetLastErrorMessage());
		return;
	}
	*accountID = Entry->Value.SingleValue.ULong;

	if (GHDFCompound_GetVerifiedEntry(&Compound, ENTRY_ID_ACCOUNT_ID, &Entry, GHDFType_ULong, "Account Image ID") != ErrorCode_Success)
	{
		Logger_LogWarning(Error_GetLastErrorMessage());
		return;
	}
	*accountImageID = Entry->Value.SingleValue.ULong;
}

static void SaveGlobalServerData(ServerResourceContext* context)
{
	GHDFCompound Compound;
	GHDFCompound_Construct(&Compound, COMPOUND_DEFAULT_CAPACITY);
	GHDFPrimitive Value;

	Value.ULong = context->AvailablePostID;
	GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_ULong, ENTRY_ID_POST_ID, Value);

	Value.ULong = context->AccountContext.AvailableAccountID;
	GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_ULong, ENTRY_ID_ACCOUNT_ID, Value);

	Value.ULong = context->AccountContext.AvailableImageID;
	GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_ULong, ENTRY_ID_ACCOUNT_IMAGE_ID, Value);

	if (GHDFCompound_WriteToFile(context->GlobalDataFilePath, &Compound) != ErrorCode_Success)
	{
		Logger_LogCritical(Error_GetLastErrorMessage());
	}
	GHDFCompound_Deconstruct(&Compound);
}


// Functions.
const char* ResourceManager_GetPathToIDFile(unsigned long long id, const char* dirName)
{
	StringBuilder Builder;
	StringBuilder_Construct(&Builder, DEFAULT_STRING_BUILDER_CAPACITY);

	StringBuilder_Append(&Builder, LTTServerC_GetCurrentContext()->Resources.DatabaseRootPath);

	StringBuilder_AppendChar(&Builder, PATH_SEPARATOR);
	StringBuilder_Append(&Builder, dirName);

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

ErrorCode ResourceManager_ConstructContext(ServerResourceContext* context, const char* dataRootPath)
{
	context->DatabaseRootPath = Directory_CombinePaths(dataRootPath, DIR_NAME_DATABASE);
	context->SourceRootPath = Directory_CombinePaths(dataRootPath, DIR_NAME_SOURCE);
	context->GlobalDataFilePath = Directory_CombinePaths(context->DatabaseRootPath, SERVER_GLOBAL_DATA_FILENAME);

	Directory_CreateAll(context->DatabaseRootPath);
	Directory_CreateAll(context->SourceRootPath);

	unsigned long long AvailableAccountID, AvailableAccountImageID, AvailablePostID;
	unsigned long long ;
	ReadGlobalData(context->GlobalDataFilePath, &AvailableAccountID, &AvailableAccountImageID, &AvailablePostID);

	if (AccountManager_ConstructContext(&context->AccountContext, AvailableAccountID) != ErrorCode_Success)
	{
		return Error_GetLastErrorCode();
	}

	return ErrorCode_Success;
}

void ResourceManager_CloseContext(ServerResourceContext* context)
{
	SaveGlobalServerData(context);
	AccountManager_CloseContext(&context->AccountContext);
	Memory_Free(context->DatabaseRootPath);
	Memory_Free(context->SourceRootPath);
	Memory_Free(context->GlobalDataFilePath);
}

ResourceResult ResourceManager_Get(ServerResourceRequest* request)
{
	return ResourceResult_Successful;
}

ResourceResult ResourceManager_Post(ServerResourceRequest* request)
{
	// --------------------------- REMOVE IN FINAL CODE OUTSIDE TESTS PLEASE!!! ---------------------------------- //
	if (String_Equals(request->Data, "stop"))   // <-------------
	{											// <-------------
		return ResourceResult_ShutDownServer;   // <-------------
	}											// <-------------
	// --------------------------- REMOVE IN FINAL CODE OUTSIDE TESTS PLEASE!!! ---------------------------------- //
	return ResourceResult_Successful;
}

//ErrorCode ResourceManager_CreateAccountInDatabase(const char* name, const char* surname, const char* email, const char* password)
//{
//	unsigned long long ID = GetAndUseAccountID();
//	const char* DataPath = ResourceManager_GetPathToIDFile(ID, DIR_NAME_ACCOUNTS);
//	const char* DataDirectoryPath = Directory_GetParentDirectory(DataPath);
//
//	if (File_Exists(DataPath))
//	{
//		return Error_SetError(ErrorCode_InvalidArgument, "Resource Manager attempted to create account which already exists (file match).");
//	}
//
//	Directory_CreateAll(DataDirectoryPath);
//
//	if ((VerifyName(name) != ErrorCode_Success) || (VerifyName(surname) != ErrorCode_Success)
//		|| (VerifyPassword(password) != ErrorCode_Success) || (VerifyEmail(email) != ErrorCode_Success))
//	{
//		return Error_GetLastErrorCode();
//	}
//
//	long long PasswordHash[PASSWORD_HASH_LENGTH];
//	GeneratePasswordHash(PasswordHash, password);
//	GHDFPrimitive* PrimitiveHashArray = (GHDFPrimitive*)Memory_SafeMalloc(sizeof(GHDFPrimitive) * PASSWORD_HASH_LENGTH);
//	for (int i = 0; i < PASSWORD_HASH_LENGTH; i++)
//	{
//		PrimitiveHashArray[i].Long = PasswordHash[i];
//	}
//
//	GHDFPrimitive Value;
//	GHDFCompound UserDataCompound;
//	GHDFCompound_Construct(&UserDataCompound, COMPOUND_DEFAULT_CAPACITY);
//
//	Value.String = name;
//	GHDFCompound_AddSingleValueEntry(&UserDataCompound, GHDFType_String, ENTRY_ID_ACCOUNT_NAME, Value);
//	Value.String = surname;
//	GHDFCompound_AddSingleValueEntry(&UserDataCompound, GHDFType_String, ENTRY_ID_ACCOUNT_SURNAME, Value);
//	Value.String = email;
//	GHDFCompound_AddSingleValueEntry(&UserDataCompound, GHDFType_String, ENTRY_ID_ACCOUNT_EMAIL, Value);
//
//	GHDFCompound_AddArrayEntry(&UserDataCompound, GHDFType_Long, ENTRY_ID_ACCOUNT_PASSWORD, PrimitiveHashArray, PASSWORD_HASH_LENGTH);
//
//	Value.ULong = 0;
//	GHDFCompound_AddSingleValueEntry(&UserDataCompound, GHDFType_ULong, ENTRY_ID_ACCOUNT_PROFILE_IMAGE_ID, Value);
//
//	if (GHDFCompound_WriteToFile(DataPath, &UserDataCompound) != ErrorCode_Success)
//	{
//		GHDFCompound_Deconstruct(&UserDataCompound);
//		return Error_GetLastErrorCode();
//	}
//
//	GHDFCompound_Deconstruct(&UserDataCompound);
//	Memory_Free(DataPath);
//	Memory_Free(DataDirectoryPath);
//}