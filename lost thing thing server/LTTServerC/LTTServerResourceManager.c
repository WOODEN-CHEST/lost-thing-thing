#include "LTTServerResourceManager.h"
#include "Directory.h"
#include "File.h"
#include "LTTServerC.h"
#include "Memory.h"
#include "LTTHTML.h"
#include <stdlib.h>
#include "GHDF.h"
#include "LTTMath.h"
#include "IDCodepointHashMap.h"


// Macros.
#define DIR_NAME_DATABASE "data"
#define DIR_NAME_SOURCE "source"

#define DIR_NAME_ENTRIES "entries"
#define DIR_NAME_META "meta"

#define DIR_NAME_EMAILS "emails"
#define DIR_NAME_ACCOUNTS "accounts"
#define DIR_NAME_POSTS "posts"

#define PASSWORD_HASH_LENGTH 16



#define USER_ID_FOLDER_NAME_DIVIDER 10000

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
/* Data verification. */
static ErrorCode VerifyName(const char* name)
{
	return ErrorCode_Success;
}

static ErrorCode VerifyPassword(const char* password)
{
	return ErrorCode_Success;
}

static ErrorCode VerifyEmail(const char* email)
{
	return ErrorCode_Success;
}


/* Password. */
static void GeneratePasswordHash(long long* longArray, const char* password) // World's most secure hash, takes a whole 10ms to crack.
{
	int PasswordLength = (int)String_LengthBytes(password);

	for (int i = 0; i < PASSWORD_HASH_LENGTH; i++)
	{
		for (int j = 0; j < PasswordLength; j++)
		{
			longArray[i] = (long long)((sinf((float)password[i]) * (float)password[i])
				+ (cbrtf((float)((int)password[i] * i))) );
		}
	}

	//for (int i = 0; i < PASSWORD_HASH_LENGTH; i++)
	//{

	//}

	//for (int i = 0; i < PASSWORD_HASH_LENGTH; i++)
	//{

	//}
}


/* Paths. */
static void AppendFolderNumberName(StringBuilder* builder, unsigned long long number)
{
	char NumberString[32];
	sprintf(NumberString, "%llu", number / USER_ID_FOLDER_NAME_DIVIDER);
	StringBuilder_Append(builder, NumberString);
}

static const char* GetPathToIDFile(unsigned long long id, const char* dirName)
{
	StringBuilder Builder;
	StringBuilder_Construct(&Builder, DEFAULT_STRING_BUILDER_CAPACITY);

	StringBuilder_Append(&Builder, LTTServerC_GetCurrentContext()->Resources.DatabaseRootPath);

	StringBuilder_AppendChar(&Builder, PATH_SEPARATOR);
	StringBuilder_Append(&Builder, dirName);

	StringBuilder_AppendChar(&Builder, PATH_SEPARATOR);
	StringBuilder_Append(&Builder, DIR_NAME_ENTRIES);

	StringBuilder_AppendChar(&Builder, PATH_SEPARATOR);
	AppendFolderNumberName(&Builder, id);

	StringBuilder_AppendChar(&Builder, PATH_SEPARATOR);
	char NumberString[32];
	sprintf(NumberString, "%llu", id);
	StringBuilder_Append(&Builder, NumberString);
	StringBuilder_Append(&Builder, GHDF_FILE_EXTENSION);

	return Builder.Data;
}


/* Users. */
static unsigned long long GetAndUseAccountID()
{
	unsigned long long ID = LTTServerC_GetCurrentContext()->Resources.AvailableAccountID;
	LTTServerC_GetCurrentContext()->Resources.AvailableAccountID += 1;
	return ID;
}

static void GenerateMetaInfoForEmail(const char* email)
{

}

/* Posts. */
static unsigned long long GetAndUsePostID()
{
	unsigned long long ID = LTTServerC_GetCurrentContext()->Resources.AvailablePostID;
	LTTServerC_GetCurrentContext()->Resources.AvailablePostID += 1;
	return ID;
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
	if (String_Equals(request->Data, "stop"))   // <-------------
	{											// <-------------
		return ResourceResult_ShutDownServer;   // <-------------
	}											// <-------------
	// --------------------------- REMOVE IN FINAL CODE OUTSIDE TESTS PLEASE!!! ---------------------------------- //
	return ResourceResult_Successful;
}

ErrorCode ResourceManager_CreateAccountInDatabase(const char* name, const char* surname, const char* email, const char* password)
{
	unsigned long long ID = GetAndUseAccountID();
	const char* DataPath = GetPathToIDFile(ID, DIR_NAME_ACCOUNTS);
	const char* DataDirectoryPath = Directory_GetParentDirectory(DataPath);

	if (File_Exists(DataPath))
	{
		return Error_SetError(ErrorCode_InvalidArgument, "Resource Manager attempted to create account which already exists (file match).");
	}

	Directory_CreateAll(DataDirectoryPath);

	if ((VerifyName(name) != ErrorCode_Success) || (VerifyName(surname) != ErrorCode_Success)
		|| (VerifyPassword(password) != ErrorCode_Success) || (VerifyEmail(email) != ErrorCode_Success))
	{
		return Error_GetLastErrorCode();
	}

	long long PasswordHash[PASSWORD_HASH_LENGTH];
	GeneratePasswordHash(PasswordHash, password);
	GHDFPrimitive* PrimitiveHashArray = (GHDFPrimitive*)Memory_SafeMalloc(sizeof(GHDFPrimitive) * PASSWORD_HASH_LENGTH);
	for (int i = 0; i < PASSWORD_HASH_LENGTH; i++)
	{
		PrimitiveHashArray[i].Long = PasswordHash[i];
	}

	GHDFPrimitive Value;
	GHDFCompound UserDataCompound;
	GHDFCompound_Construct(&UserDataCompound, COMPOUND_DEFAULT_CAPACITY);

	Value.String = name;
	GHDFCompound_AddSingleValueEntry(&UserDataCompound, GHDFType_String, ENTRY_ID_ACCOUNT_NAME, Value);
	Value.String = surname;
	GHDFCompound_AddSingleValueEntry(&UserDataCompound, GHDFType_String, ENTRY_ID_ACCOUNT_SURNAME, Value);
	Value.String = email;
	GHDFCompound_AddSingleValueEntry(&UserDataCompound, GHDFType_String, ENTRY_ID_ACCOUNT_EMAIL, Value);

	GHDFCompound_AddArrayEntry(&UserDataCompound, GHDFType_Long, ENTRY_ID_ACCOUNT_PASSWORD, PrimitiveHashArray, PASSWORD_HASH_LENGTH);

	Value.ULong = 0;
	GHDFCompound_AddSingleValueEntry(&UserDataCompound, GHDFType_ULong, ENTRY_ID_ACCOUNT_PROFILE_IMAGE_ID, Value);

	if (GHDFCompound_WriteToFile(DataPath, &UserDataCompound) != ErrorCode_Success)
	{
		GHDFCompound_Deconstruct(&UserDataCompound);
		return Error_GetLastErrorCode();
	}

	GHDFCompound_Deconstruct(&UserDataCompound);
	Memory_Free(DataPath);
	Memory_Free(DataDirectoryPath);

	GenerateMetaInfoForEmail(email);
}