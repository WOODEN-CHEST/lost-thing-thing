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
#include "LTTChar.h"

/* Mega source-code file which hold it all together, somehow. */

// Macros.
#define DIR_NAME_DATABASE "data"
#define DIR_NAME_SOURCE "source"
#define DIR_NAME_ACCOUNTS "accounts"
#define DIR_NAME_POSTS "posts"


#define MIN_NAME_LENGTH_CODEPOINTS 1
#define MAX_NAME_LENGTH_CODEPOINTS 128

#define MIN_PASSWORD_LENGTH_CODEPOINTS 4
#define MAX_PASSWORD_LENGTH_CODEPOINTS 256

#define MAX_EMAIL_LENGTH_CODEPOINTS 256
#define EMAIL_SPECIAL_CHAR_DOT '.'
#define EMAIL_SPECIAL_CHAR_UNDERSCORE '_'
#define EMAIL_SPECIAL_CHAR_DASH '-'
#define EMAIL_SPECIAL_CHAR_AT '@'


#define PASSWORD_HASH_LENGTH 16


#define USER_ID_FOLDER_NAME_DIVIDER 10000


/* User account. */
#define ENTRY_ID_ACCOUNT_NAME 1 // String
#define ENTRY_ID_ACCOUNT_SURNAME 2 // String
#define ENTRY_ID_ACCOUNT_EMAIL 3 // String
#define ENTRY_ID_ACCOUNT_PASSWORD 4 // ulong array with length PASSWORD_HASH_LENGTH
#define ENTRY_ID_ACCOUNT_PROFILE_IMAGE_ID 5 // ulong
#define ENTRY_ID_ACCOUNT_POSTS 6 // ulong array with length >= 1, MAY NOT BE PRESENT IF ACCOUNT HAS NO POSTS
#define ENTRY_ID_ACCOUNT_CREATION_TIME 7 // long
#define ENTRY_ID_ACCOUNT_ID 7 // ulong


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

/* Account. */
typedef struct UserAccountStruct
{
	unsigned long long ID;
	const char* Name;
	const char* Surname;
	long long Password[16];
	long long CreationTime;
	unsigned long long ProfileImageID;
	unsigned long long* Posts;
} UserAccount;


/* Posts. */


// Static functions.
/* GHDF. */
static ErrorCode GetVerifiedCompoundEntry(GHDFCompound* compound, 
	unsigned long long id, 
	GHDFType expectedType, 
	GHDFEntry** entry, 
	const char* errorInfo)
{
	GHDFEntry* Entry = GHDFCompound_GetEntry(compound, id);
	if (!Entry)
	{
		char Message[512];
		snprintf(Message, sizeof(Message), "Mandatory compound entry with ID %llu of type %d not found. %s",
			id, (int)expectedType, errorInfo);
		return Error_SetError(ErrorCode_DatabaseError, Message);
	}
	if (Entry->ValueType != expectedType)
	{
		char Message[512];
		snprintf(Message, sizeof(Message), "Compound entry with ID %llu has mismatched type. Expected %d, got %d. %s", id,
			(int)expectedType, (int)Entry->ValueType, errorInfo);
		return Error_SetError(ErrorCode_DatabaseError, Message);
	}

	return ErrorCode_Success;
}


/* Data verification. */
static ErrorCode VerifyName(const char* name)
{
	size_t Length = String_LengthCodepointsUTF8(name);
	if (Length > MAX_NAME_LENGTH_CODEPOINTS)
	{
		return Error_SetError(ErrorCode_InvalidRequest, "Name's length exceeds limits.");
	}
	else if (Length < MIN_NAME_LENGTH_CODEPOINTS)
	{
		return Error_SetError(ErrorCode_InvalidRequest, "Name is too short.");
	}

	for (size_t i = 0; name[i] != '\0'; i += Char_GetByteCount(name + i))
	{
		if (!(Char_IsLetter(name + i) || (name[i] == ' ') || Char_IsDigit(name + i)))
		{
			return Error_SetError(ErrorCode_InvalidRequest, "Name contains invalid characters");
		}
	}

	return ErrorCode_Success;
}

static ErrorCode VerifyPassword(const char* password)
{
	size_t Length = String_LengthCodepointsUTF8(password);
	if (Length > MAX_PASSWORD_LENGTH_CODEPOINTS)
	{
		return Error_SetError(ErrorCode_InvalidRequest, "Password's length exceeds limits.");
	}
	else if (Length < MIN_PASSWORD_LENGTH_CODEPOINTS)
	{
		return Error_SetError(ErrorCode_InvalidRequest, "Password is too short.");
	}

	for (size_t i = 0; password[i] != '\0'; i += Char_GetByteCount(password + i))
	{
		if (Char_IsWhitespace(password + i))
		{
			return Error_SetError(ErrorCode_InvalidRequest, "Password contains invalid characters");
		}
	}

	return ErrorCode_Success;
}

static inline bool IsSpecialEmailChar(const char* character)
{
	return (*character == EMAIL_SPECIAL_CHAR_DASH) || (*character == EMAIL_SPECIAL_CHAR_DOT) || (character == EMAIL_SPECIAL_CHAR_UNDERSCORE);
}

static inline bool IsValidEmailCharacter(const char* character)
{
	return Char_IsLetter(character) || Char_IsDigit(character) || IsSpecialEmailChar(character);
}

static const char* VerifyEmailPrefix(const char* email)
{
	size_t PrefixLength = 0;
	bool HadSpecialChar = false;
	const char* ShiftedString; 

	for (ShiftedString = email; (*ShiftedString != '\0') && (*ShiftedString != EMAIL_SPECIAL_CHAR_AT);
		ShiftedString += Char_GetByteCount(ShiftedString), PrefixLength++)
	{
		HadSpecialChar = IsSpecialEmailChar(ShiftedString);

		if ((HadSpecialChar && (PrefixLength == 0)) || !IsValidEmailCharacter(ShiftedString))
		{
			return NULL;
		}
	}

	if ((PrefixLength == 0) || HadSpecialChar)
	{
		return NULL;
	}

	return ShiftedString;
}

static const char* VerifyEmailDomain(const char* email)
{

}

static ErrorCode VerifyEmail(const char* email)
{
	if (String_LengthCodepointsUTF8(email) > MAX_EMAIL_LENGTH_CODEPOINTS)
	{
		return Error_SetError(ErrorCode_InvalidRequest, "Email's length exceeds limits.");
	}

	const char* ParsedEmailPosition = VerifyEmailPrefix(email);
	if (!ParsedEmailPosition)
	{
		return Error_SetError(ErrorCode_InvalidRequest, "Email prefix is invalid.");
	}
	
	if (*ParsedEmailPosition != EMAIL_SPECIAL_CHAR_AT)
	{
		return Error_SetError(ErrorCode_InvalidRequest, "Email missing @ symbol.");;
	}
	ParsedEmailPosition++;

	ParsedEmailPosition = VerifyEmailDomain(email);
	if (!ParsedEmailPosition)
	{
		return Error_SetError(ErrorCode_InvalidRequest, "Email domain is invalid.");
	}

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
			longArray[i] = (long long)((sinf(password[i]) *password[i]) + (cbrtf(password[i] * i)));
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
	AppendFolderNumberName(&Builder, id);

	StringBuilder_AppendChar(&Builder, PATH_SEPARATOR);
	char NumberString[32];
	sprintf(NumberString, "%llu", id);
	StringBuilder_Append(&Builder, NumberString);
	StringBuilder_Append(&Builder, GHDF_FILE_EXTENSION);

	return Builder.Data;
}


/* Accounts. */
static unsigned long long GetAndUseAccountID()
{
	unsigned long long ID = LTTServerC_GetCurrentContext()->Resources.AvailableAccountID;
	LTTServerC_GetCurrentContext()->Resources.AvailableAccountID += 1;
	return ID;
}

static ErrorCode GenerateMetaInfoForSingleAccount(unsigned long long id)
{
	GHDFCompound ProfileData;
	const char* FilePath = GetPathToIDFile(id, DIR_NAME_ACCOUNTS);
	if (GHDFCompound_ReadFromFile(FilePath, &ProfileData) != ErrorCode_Success)
	{
		Memory_Free(FilePath);
		return Error_GetLastErrorCode();
	}
	Memory_Free(FilePath);

	GHDFEntry* Entry = GHDFCompound_GetEntry(&ProfileData, ENTRY_ID_ACCOUNT_NAME);

	char AccountID[32];
	sprintf(AccountID, "Account ID is %llu", id);
	if (GetVerifiedCompoundEntry(&ProfileData, ENTRY_ID_ACCOUNT_NAME, GHDFType_String, &Entry, AccountID) != ErrorCode_Success)
	{
		GHDFCompound_Deconstruct(&ProfileData);
		return Error_GetLastErrorCode();
	}
	IDCodepointHashMap_AddID(&LTTServerC_GetCurrentContext()->Resources.AccountNameMap, Entry->Value.SingleValue.String, id);

	if (GetVerifiedCompoundEntry(&ProfileData, ENTRY_ID_ACCOUNT_SURNAME, GHDFType_String, &Entry, AccountID) != ErrorCode_Success)
	{
		GHDFCompound_Deconstruct(&ProfileData);
		return Error_GetLastErrorCode();
	}
	IDCodepointHashMap_AddID(&LTTServerC_GetCurrentContext()->Resources.AccountNameMap, Entry->Value.SingleValue.String, id);

	if (GetVerifiedCompoundEntry(&ProfileData, ENTRY_ID_ACCOUNT_EMAIL, GHDFType_String, &Entry, AccountID) != ErrorCode_Success)
	{
		GHDFCompound_Deconstruct(&ProfileData);
		return Error_GetLastErrorCode();
	}
	IDCodepointHashMap_AddID(&LTTServerC_GetCurrentContext()->Resources.AccountEmailMap, Entry->Value.SingleValue.String, id);

	GHDFCompound_Deconstruct(&ProfileData);
	return ErrorCode_Success;
}

static ErrorCode GenerateMetaInfoForAccounts()
{
	unsigned long long MaxID = LTTServerC_GetCurrentContext()->Resources.AvailablePostID;
	for (unsigned long long ID = 0; ID < MaxID; ID++)
	{
		if (GenerateMetaInfoForSingleAccount(ID) != ErrorCode_Success)
		{
			return Error_GetLastErrorCode();
		}
	}

	return ErrorCode_Success;
}

static ErrorCode ReadAccountFromCompound(UserAccount* account, GHDFCompound* compound)
{

}

static ErrorCode WriteAccountToCompound(UserAccount* account, GHDFCompound* compound)
{

}

static ErrorCode CreateFreshAccount(UserAccount* account, const char* name, const char* surname, const char* email, const char* password)
{
	if ((VerifyName(name) != ErrorCode_Success) || (VerifyName(surname) != ErrorCode_Success)
		|| (VerifyPassword(password) != ErrorCode_Success) || (VerifyEmail(email) != ErrorCode_Success))
	{
		return Error_GetLastErrorCode();
	}

	account->Name = name;
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
}

ErrorCode ResourceManager_GenerateIDHashMaps()
{
	if (GenerateMetaInfoForAccounts() != ErrorCode_Success)
	{
		return Error_GetLastErrorCode();
	}
}