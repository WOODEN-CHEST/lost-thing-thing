#include "LTTAccountManager.h"
#include "GHDF.h"
#include "LTTServerC.h"
#include "Memory.h"
#include "LTTServerResourceManager.h"


// Macros.
#define DIR_NAME_ACCOUNTS "accounts"


#define MIN_NAME_LENGTH_CODEPOINTS 1
#define MAX_NAME_LENGTH_CODEPOINTS 128

#define MIN_PASSWORD_LENGTH_CODEPOINTS 4
#define MAX_PASSWORD_LENGTH_CODEPOINTS 256
#define PASSWORD_HASH_LENGTH 16

#define MAX_EMAIL_LENGTH_CODEPOINTS 256
#define EMAIL_SPECIAL_CHAR_DOT '.'
#define EMAIL_SPECIAL_CHAR_UNDERSCORE '_'
#define EMAIL_SPECIAL_CHAR_DASH '-'
#define EMAIL_SPECIAL_CHAR_AT '@'


#define SESSION_ID_BYTE_COUNT 128


#define ENTRY_ID_ACCOUNT_NAME 1 // String
#define ENTRY_ID_ACCOUNT_SURNAME 2 // String
#define ENTRY_ID_ACCOUNT_EMAIL 3 // String
#define ENTRY_ID_ACCOUNT_PASSWORD 4 // ulong array with length PASSWORD_HASH_LENGTH
#define ENTRY_ID_ACCOUNT_PROFILE_IMAGE_ID 5 // ulong
#define ENTRY_ID_ACCOUNT_POSTS 6 // ulong array with length >= 1, MAY NOT BE PRESENT IF ACCOUNT HAS NO POSTS
#define ENTRY_ID_ACCOUNT_CREATION_TIME 7 // long
#define ENTRY_ID_ACCOUNT_ID 7 // ulong


// Static functions.
static unsigned long long GetAndUseAccountID()
{
	unsigned long long ID = LTTServerC_GetCurrentContext()->Resources.AccountContext.AvailableID;
	LTTServerC_GetCurrentContext()->Resources.AccountContext.AvailableID += 1;
	return ID;
}

/* Data verification and generation. */
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
	size_t DomainLength = 0, SuffixLength = 0;
	const size_t MIN_SUFFIX_LENGTH = 2;
	bool HadSpecialChar = false, HadSeparator = false;
	const char* ShiftedString;

	for (ShiftedString = email; (*ShiftedString != '\0'); ShiftedString += Char_GetByteCount(ShiftedString), DomainLength++)
	{
		HadSpecialChar = IsSpecialEmailChar(ShiftedString);
		if (!IsValidEmailCharacter(ShiftedString) || ((DomainLength == 0) && HadSpecialChar))
		{
			return NULL;
		}

		if (HadSeparator)
		{
			SuffixLength++;
		}

		if (*ShiftedString == EMAIL_SPECIAL_CHAR_DOT)
		{
			HadSeparator = true;
		}
	}

	if ((DomainLength == 0) || (SuffixLength < MIN_SUFFIX_LENGTH) || HadSpecialChar || !HadSeparator)
	{
		return NULL;
	}

	return ShiftedString;
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

static void GeneratePasswordHash(long long* longArray, const char* password) // World's most secure hash, literally impossible to crack.
{
	int PasswordLength = (int)String_LengthBytes(password);

	for (int i = 0; i < PASSWORD_HASH_LENGTH; i++)
	{
		for (int j = 0; j < PasswordLength; j++)
		{
			longArray[i] = (long long)((sinf(password[i]) * password[i]) + (cbrtf(password[i] * i)));
		}
	}

	//for (int i = 0; i < PASSWORD_HASH_LENGTH; i++)
	//{

	//}

	//for (int i = 0; i < PASSWORD_HASH_LENGTH; i++)
	//{

	//}
}

/* Account loading and saving. */
static ErrorCode ReadAccountFromCompound(UserAccount* account, GHDFCompound* compound)
{

}

static ErrorCode WriteAccountToCompound(UserAccount* account, GHDFCompound* compound)
{

}


/* Hashmap. */
static ErrorCode GenerateMetaInfoForSingleAccount(unsigned long long id)
{
	GHDFCompound ProfileData;
	const char* FilePath = ResourceManager_GetPathToIDFile(id, DIR_NAME_ACCOUNTS);
	if (GHDFCompound_ReadFromFile(FilePath, &ProfileData) != ErrorCode_Success)
	{
		Memory_Free(FilePath);
		return Error_GetLastErrorCode();
	}
	Memory_Free(FilePath);

	GHDFEntry* Entry;
	char AccountID[32];
	sprintf(AccountID, "Account ID is %llu", id);

	if (GHDFCompound_GetVerifiedEntry(&ProfileData, ENTRY_ID_ACCOUNT_NAME, &Entry, GHDFType_String, AccountID) != ErrorCode_Success)
	{
		GHDFCompound_Deconstruct(&ProfileData);
		return Error_GetLastErrorCode();
	}
	IDCodepointHashMap_AddID(&LTTServerC_GetCurrentContext()->Resources.AccountContext.NameMap, Entry->Value.SingleValue.String, id);

	if (GHDFCompound_GetVerifiedEntry(&ProfileData, ENTRY_ID_ACCOUNT_SURNAME, &Entry, GHDFType_String, AccountID) != ErrorCode_Success)
	{
		GHDFCompound_Deconstruct(&ProfileData);
		return Error_GetLastErrorCode();
	}
	IDCodepointHashMap_AddID(&LTTServerC_GetCurrentContext()->Resources.AccountContext.NameMap, Entry->Value.SingleValue.String, id);

	if (GHDFCompound_GetVerifiedEntry(&ProfileData, ENTRY_ID_ACCOUNT_EMAIL, &Entry, GHDFType_String, AccountID) != ErrorCode_Success)
	{
		GHDFCompound_Deconstruct(&ProfileData);
		return Error_GetLastErrorCode();
	}
	IDCodepointHashMap_AddID(&LTTServerC_GetCurrentContext()->Resources.AccountContext.EmailMap, Entry->Value.SingleValue.String, id);

	GHDFCompound_Deconstruct(&ProfileData);
	return ErrorCode_Success;
}


// Functions.
ErrorCode AccountManager_ConstructContext(DBAccountContext* context, unsigned long long availableID)
{
	context->AvailableID = availableID;
	IDCodepointHashMap_Construct(&context->NameMap);
	IDCodepointHashMap_Construct(&context->EmailMap);

	unsigned long long MaxIDExclusive = context->AvailableID;
	for (unsigned long long ID = 0; ID < MaxIDExclusive; ID++)
	{
		if (GenerateMetaInfoForSingleAccount(ID) != ErrorCode_Success)
		{
			return Error_GetLastErrorCode();
		}
	}

	return ErrorCode_Success;
}

void AccountManager_CloseContext(DBAccountContext* context)
{
	IDCodepointHashMap_Deconstruct(&context->NameMap);
	IDCodepointHashMap_Deconstruct(&context->EmailMap);
}

ErrorCode AccountManager_TryCreateUser(UserAccount* account,
	const char* name,
	const char* surname,
	const char* email,
	const char* password)
{
	if ((VerifyName(name) != ErrorCode_Success) || (VerifyName(surname) != ErrorCode_Success)
		|| (VerifyPassword(password) != ErrorCode_Success) || (VerifyEmail(email) != ErrorCode_Success))
	{
		return Error_GetLastErrorCode();
	}
}

ErrorCode AccountManager_TryCreateUnverifiedUser(UserAccount* account,
	const char* name,
	const char* surname,
	const char* email,
	const char* password)
{

}

bool AccountManager_IsUserAdmin(SessionID* sessionID)
{

}

bool AccountManager_GetAccount(UserAccount* account, unsigned long long id)
{

}

ErrorCode AccountManager_GenerateAccountHashMaps() 
{
	

	return ErrorCode_Success;
}