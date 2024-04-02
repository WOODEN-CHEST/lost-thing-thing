#include "LTTAccountManager.h"
#include "GHDF.h"
#include "LTTServerC.h"
#include "Memory.h"
#include "LTTServerResourceManager.h"
#include "math.h"


// Macros.
#define DIR_NAME_ACCOUNTS "accounts"

#define NO_ACCOUNT_IMAGE_ID 0


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

#define ACCOUNT_LIST_CAPACITY 8
#define ACCOUNT_LIST_GROWTH 2


/* Account struct. */
#define ENTRY_ID_ACCOUNT_NAME 1 // String
#define ENTRY_ID_ACCOUNT_SURNAME 2 // String
#define ENTRY_ID_ACCOUNT_EMAIL 3 // String
#define ENTRY_ID_ACCOUNT_PASSWORD 4 // ulong array with length PASSWORD_HASH_LENGTH
#define ENTRY_ID_ACCOUNT_PROFILE_IMAGE_ID 5 // ulong
#define ENTRY_ID_ACCOUNT_POSTS 6 // ulong array with length >= 1, MAY NOT BE PRESENT IF ACCOUNT HAS NO POSTS
#define ENTRY_ID_ACCOUNT_CREATION_TIME 7 // long
#define ENTRY_ID_ACCOUNT_ID 8 // ulong


// Types.
typedef struct AccountListStruct
{
	UserAccount* Accounts;
	size_t Count;
	size_t _capacity;
} AccountList;


// Static functions.
static unsigned long long GetAndUseAccountID()
{
	unsigned long long ID = LTTServerC_GetCurrentContext()->Resources.AccountContext.AvailableAccountID;
	LTTServerC_GetCurrentContext()->Resources.AccountContext.AvailableAccountID += 1;
	return ID;
}


/* Account. */
void AccountDeconstruct(UserAccount* account)
{
	Memory_Free(account->Name);
	Memory_Free(account->Surname);
	Memory_Free(account->Email);
	if (account->Posts)
	{
		Memory_Free(account->Posts);
	}
}



/* Account list. */
void AccountListConstruct(AccountList* self)
{
	self->Count = 0;
	self->Accounts = (UserAccount*)Memory_SafeMalloc(sizeof(UserAccount) * ACCOUNT_LIST_CAPACITY);
	self->_capacity = ACCOUNT_LIST_CAPACITY;
}

void AccountListDeconstruct(AccountList* self)
{
	for (size_t i = 0; i < self->Count; i++)
	{
		AccountDeconstruct(self->Accounts + i);
	}

	Memory_Free(self->Accounts);
}

void AccountListEnsureCapacity(AccountList* self, size_t capacity)
{
	if (self->_capacity > capacity)
	{
		return;
	}

	while (self->_capacity < capacity)
	{
		self->_capacity *= ACCOUNT_LIST_GROWTH;
	}
	self->Accounts = (UserAccount*)Memory_SafeRealloc(self->Accounts, sizeof(UserAccount) * self->_capacity);
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
			long long HashNumber = password[j] * (i + 57 + i + password[j]);
		    float FloatNumber = sinf(HashNumber);
			HashNumber &= *(long long*)(&FloatNumber);
			FloatNumber = cbrtf(i + password[j] + 7);
			HashNumber += *(long long*)(&FloatNumber);
			HashNumber |= PasswordLength * PasswordLength * PasswordLength * PasswordLength / (i + 1);
			HashNumber ^= password[j] * i;
		}
	}

	for (int i = 0; i < PASSWORD_HASH_LENGTH; i++)
	{
		longArray[i] = longArray[i * 7 % PASSWORD_HASH_LENGTH] ^ PasswordLength;
		longArray[i] *= longArray[(i + longArray[i]) / longArray[i] % PASSWORD_HASH_LENGTH];
	}

	for (int i = 0; i < PASSWORD_HASH_LENGTH; i++)
	{
		int Index1 = (i + longArray[i]) % PASSWORD_HASH_LENGTH;
		long long Temp = longArray[Index1];
		int Index2 = (Index1 + Temp) % PASSWORD_HASH_LENGTH;
		longArray[Index1] = longArray[Index2];
		longArray[Index2] = Temp;
	}
}

/* Searching database. */
static bool IsEmailInDatabase(const char* email)
{
	size_t IDArraySize;
	IDCodepointHashMap_FindByString(&LTTServerC_GetCurrentContext()->Resources.AccountContext.EmailMap, email, true, &IDArraySize);
}

static UserAccount* GetAccountsByName(const char* name, size_t* accountArraySize)
{

}


/* Account loading and saving. */
static bool ReadAccountFromDatabase(UserAccount* account, unsigned long long id)
{

}

static ErrorCode WriteAccountToDatabase(UserAccount* account)
{
	GHDFCompound Compound;
	GHDFCompound_Construct(&Compound, COMPOUND_DEFAULT_CAPACITY);
	GHDFPrimitive SingleValue;

	SingleValue.String = String_CreateCopy(account->Name);
	GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_String, ENTRY_ID_ACCOUNT_NAME, SingleValue);
	SingleValue.String = String_CreateCopy(account->Surname);
	GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_String, ENTRY_ID_ACCOUNT_SURNAME, SingleValue);
	SingleValue.String = String_CreateCopy(account->Email);
	GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_String, ENTRY_ID_ACCOUNT_EMAIL, SingleValue);

	GHDFPrimitive* PrimitiveArray = (GHDFPrimitive*)Memory_SafeMalloc(sizeof(GHDFPrimitive) * PASSWORD_HASH_LENGTH);
	for (int i = 0; i < PASSWORD_HASH_LENGTH; i++)
	{
		PrimitiveArray[i].Long = account->PasswordHash[i];
	}
	GHDFCompound_AddArrayEntry(&Compound, GHDFType_Long, ENTRY_ID_ACCOUNT_PASSWORD, PrimitiveArray, PASSWORD_HASH_LENGTH);

	SingleValue.ULong = account->ProfileImageID;
	GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_ULong, ENTRY_ID_ACCOUNT_PROFILE_IMAGE_ID, SingleValue);

	if (account->PostCount > 0)
	{
		PrimitiveArray = (GHDFPrimitive*)Memory_SafeMalloc(sizeof(GHDFPrimitive) * account->PostCount);
		for (int i = 0; i < account->PostCount; i++)
		{
			PrimitiveArray[i].ULong = account->Posts[i];
		}
		GHDFCompound_AddArrayEntry(&Compound, GHDFType_ULong, ENTRY_ID_ACCOUNT_POSTS, PrimitiveArray, account->PostCount);
	}
	
	SingleValue.Long = account->CreationTime;
	GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_Long, ENTRY_ID_ACCOUNT_CREATION_TIME, SingleValue);
	SingleValue.ULong = account->ID;
	GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_ULong, ENTRY_ID_ACCOUNT_ID, SingleValue);

	const char* AccountPath = ResourceManager_GetPathToIDFile(account->ID, DIR_NAME_ACCOUNTS);
	ErrorCode ResultCode = GHDFCompound_WriteToFile(AccountPath, &Compound);
	Memory_Free(AccountPath);
	GHDFCompound_Deconstruct(&Compound);
	return ResultCode;
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
ErrorCode AccountManager_ConstructContext(DBAccountContext* context, unsigned long long availableID, unsigned long long availableImageID)
{
	context->AvailableAccountID = availableID;
	IDCodepointHashMap_Construct(&context->NameMap);
	IDCodepointHashMap_Construct(&context->EmailMap);

	unsigned long long MaxIDExclusive = context->AvailableAccountID;
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
	
	if (IsEmailInDatabase(email))
	{
		return Error_SetError(ErrorCode_InvalidArgument, "AccountManager_TryCreateUser: Account with the same email already exists.");
	}

	time_t CurrentTime = time(NULL);
	if (CurrentTime == -1)
	{
		return Error_SetError(ErrorCode_DatabaseError, "AccountManager_TryCreateUser: Failed to generate account creation time.");
	}
	
	account->ID = GetAndUseAccountID();
	account->Name = String_CreateCopy(name);
	account->Surname = String_CreateCopy(surname);
	account->Email = String_CreateCopy(email);
	GeneratePasswordHash(account->PasswordHash, password);
	account->CreationTime = CurrentTime;
	account->PostCount = 0;
	account->Posts = NULL;
	account->ProfileImageID = NO_ACCOUNT_IMAGE_ID;

	if (WriteAccountToDatabase(account) != ErrorCode_Success)
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