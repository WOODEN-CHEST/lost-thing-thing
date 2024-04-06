#include "LTTAccountManager.h"
#include "GHDF.h"
#include "LTTServerC.h"
#include "Memory.h"
#include "LTTServerResourceManager.h"
#include "math.h"
#include "File.h"
#include "LTTChar.h"
#include "LTTMath.h"
#include "Directory.h"


// Macros.
#define DIR_NAME_ACCOUNTS "accounts"

#define GENERIC_LIST_CAPACITY 8
#define GENERIC_LIST_GROWTH 2

#define MAX_ACCOUNT_VERIFICATION_TIME 60 * 5
#define MAX_VERIFICATION_ATTEMPTS 3


/* Account data. */
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

#define NO_ACCOUNT_IMAGE_ID 0


/* Account struct. */
#define ENTRY_ID_ACCOUNT_NAME 1 // String
#define ENTRY_ID_ACCOUNT_SURNAME 2 // String
#define ENTRY_ID_ACCOUNT_EMAIL 3 // String
#define ENTRY_ID_ACCOUNT_PASSWORD 4 // ulong array with length PASSWORD_HASH_LENGTH
#define ENTRY_ID_ACCOUNT_PROFILE_IMAGE_ID 5 // ulong
#define ENTRY_ID_ACCOUNT_POSTS 6 // ulong array with length >= 1, MAY NOT BE PRESENT IF ACCOUNT HAS NO POSTS
#define ENTRY_ID_ACCOUNT_CREATION_TIME 7 // long
#define ENTRY_ID_ACCOUNT_ID 8 // ulong
#define ENTRY_ID_ACCOUNT_IS_ADMIN 9 // bool


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

static unsigned long long GetAccountID() 
{
	return LTTServerC_GetCurrentContext()->Resources.AccountContext.AvailableAccountID;
}

/* Data verification and generation. */
static bool VerifyName(const char* name)
{
	if (!String_IsValidUTF8String(name))
	{
		Logger_LogWarning("Found invalid UTF-8 string while verifying account name.");
		return false;
	}

	size_t Length = String_LengthCodepointsUTF8(name);
	if (Length > MAX_NAME_LENGTH_CODEPOINTS)
	{
		return false;
	}
	else if (Length < MIN_NAME_LENGTH_CODEPOINTS)
	{
		return false;
	}

	for (size_t i = 0; name[i] != '\0'; i += Char_GetByteCount(name + i))
	{
		if (!(Char_IsLetter(name + i) || (name[i] == ' ') || Char_IsDigit(name + i)))
		{
			return false;
		}
	}

	return true;
}

static bool VerifyPassword(const char* password)
{
	if (!String_IsValidUTF8String(password))
	{
		Logger_LogWarning("Found invalid UTF-8 string while verifying account password.");
		return false;
	}

	size_t Length = String_LengthCodepointsUTF8(password);
	if (Length > MAX_PASSWORD_LENGTH_CODEPOINTS)
	{
		return false;
	}
	else if (Length < MIN_PASSWORD_LENGTH_CODEPOINTS)
	{
		return false;
	}

	for (size_t i = 0; password[i] != '\0'; i += Char_GetByteCount(password + i))
	{
		if (Char_IsWhitespace(password + i))
		{
			return false;
		}
	}

	return true;
}

static inline bool IsSpecialEmailChar(const char* character)
{
	return (*character == EMAIL_SPECIAL_CHAR_DASH) || (*character == EMAIL_SPECIAL_CHAR_DOT) || (*character == EMAIL_SPECIAL_CHAR_UNDERSCORE);
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

	for (size_t i = 0; i < LTTServerC_GetCurrentContext()->Configuration.AcceptedDomainCount; i++)
	{
		if (String_Equals(LTTServerC_GetCurrentContext()->Configuration.AcceptedDomains[i], email))
		{
			return ShiftedString;
		}
	}

	return NULL;
}

static bool VerifyEmail(const char* email)
{
	if (!String_IsValidUTF8String(email))
	{
		Logger_LogWarning("Found invalid UTF-8 string while verifying account email.");
		return false;
	}

	if (String_LengthCodepointsUTF8(email) > MAX_EMAIL_LENGTH_CODEPOINTS)
	{
		return false;
	}

	const char* ParsedEmailPosition = VerifyEmailPrefix(email);
	if (!ParsedEmailPosition)
	{
		return false;
	}

	if (*ParsedEmailPosition != EMAIL_SPECIAL_CHAR_AT)
	{
		return false;
	}
	ParsedEmailPosition++;

	ParsedEmailPosition = VerifyEmailDomain(ParsedEmailPosition);
	if (!ParsedEmailPosition)
	{
		return false;
	}

	return true;
}

static void GeneratePasswordHash(unsigned long long* longArray, const char* password) // World's most secure hash, literally impossible to crack.
{
	int PasswordLength = (int)String_LengthBytes(password);

	for (int i = 0; i < PASSWORD_HASH_LENGTH; i++)
	{
		unsigned long long HashNumber = 9;
		for (int j = 0; j < PasswordLength; j++)
		{
			HashNumber += password[j] * (i + 57 + i + password[j]);
			double DoubleNumber = (double)HashNumber;
			HashNumber &= *(long long*)(&DoubleNumber);
			DoubleNumber = (i + password[j] + 7) * 0.3984;
			HashNumber += *(long long*)(&DoubleNumber);
			HashNumber |= PasswordLength * PasswordLength * PasswordLength * PasswordLength / (i + 1);
			HashNumber ^= password[j] * i;
		}
		longArray[i] = HashNumber;
	}

	for (int i = 0; i < PASSWORD_HASH_LENGTH; i++)
	{
		longArray[i] = longArray[i * 7 % PASSWORD_HASH_LENGTH] ^ PasswordLength;
		longArray[i] *= longArray[(i + longArray[i]) / longArray[i] % PASSWORD_HASH_LENGTH];
	}

	for (int i = 0; i < PASSWORD_HASH_LENGTH; i++)
	{
		int Index1 = (i + longArray[i]) % PASSWORD_HASH_LENGTH;
		unsigned long long Temp = longArray[Index1];
		int Index2 = (Index1 + Temp) % PASSWORD_HASH_LENGTH;
		longArray[Index1] = longArray[Index2];
		longArray[Index2] = Temp;
	}
}


/* Account. */
static void AccountSetDefaultValues(UserAccount* account)
{
	Memory_Set(account, sizeof(UserAccount), 0);
}

static void AccountDeconstruct(UserAccount* account)
{
	if (account->Name)
	{
		Memory_Free(account->Name);
	}
	if (account->Surname)
	{
		Memory_Free(account->Surname);
	}
	if (account->Email)
	{
		Memory_Free(account->Email);
	}
	if (account->Posts)
	{
		Memory_Free(account->Posts);
	}
}

static ErrorCode AccountCreateNew(UserAccount* account,
	const char* name,
	const char* surname,
	const char* email,
	const char* password)
{
	time_t CurrentTime = time(NULL);
	if (CurrentTime == -1)
	{
		return Error_SetError(ErrorCode_DatabaseError, "AccountManager_TryCreateUser: Failed to generate account creation time.");
	}
	account->ID = GetAndUseAccountID();
	account->Name = String_CreateCopy(name);
	account->Surname = String_CreateCopy(surname);
	account->Email = String_CreateCopy(email);
	GeneratePasswordHash(&account->PasswordHash, password);
	account->CreationTime = CurrentTime;
	account->PostCount = 0;
	account->Posts = NULL;
	account->ProfileImageID = NO_ACCOUNT_IMAGE_ID;
	account->IsAdmin = false;

	return ErrorCode_Success;
}


/* Account list. */
static void AccountListConstruct(AccountList* self)
{
	self->Count = 0;
	self->Accounts = (UserAccount*)Memory_SafeMalloc(sizeof(UserAccount) * GENERIC_LIST_CAPACITY);
	self->_capacity = GENERIC_LIST_CAPACITY;
}

static void AccountListDeconstruct(AccountList* self)
{
	for (size_t i = 0; i < self->Count; i++)
	{
		AccountDeconstruct(self->Accounts + i);
	}

	Memory_Free(self->Accounts);
}

static void AccountListEnsureCapacity(AccountList* self, size_t capacity)
{
	if (self->_capacity > capacity)
	{
		return;
	}

	while (self->_capacity < capacity)
	{
		self->_capacity *= GENERIC_LIST_GROWTH;
	}
	self->Accounts = (UserAccount*)Memory_SafeRealloc(self->Accounts, sizeof(UserAccount) * self->_capacity);
}


/* Unverified account. */
static void UnverifiedAccountDeconstruct(UnverifiedUserAccount* account)
{
	Memory_Free(account->Name);
	Memory_Free(account->Surname);
	Memory_Free(account->Email);
	Memory_Free(account->Password);
}

static bool UnverifiedAccountIsExpired(UnverifiedUserAccount* account)
{
	time_t CurrentTime = time(NULL);
	return CurrentTime - account->VerificationStartTime > MAX_ACCOUNT_VERIFICATION_TIME;
}

static bool UnverifiedAccountTryVerify(UnverifiedUserAccount* account, int code)
{
	account->VerificationAttempts += 1;

	if (UnverifiedAccountIsExpired(account) || (account->VerificationCode != code)
		|| (account->VerificationAttempts > MAX_VERIFICATION_ATTEMPTS))
	{
		return false;
	}

	return true;
}

static void UnverifiedAccountListEnsureCapacity(size_t capacity)
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;
	if (Context->_unverifiedAccountCapacity >= capacity)
	{
		return;
	}

	while (Context->_unverifiedAccountCapacity < capacity)
	{
		Context->_unverifiedAccountCapacity *= GENERIC_LIST_GROWTH;
	}
	Context->UnverifiedAccounts = (UnverifiedUserAccount*)
		Memory_SafeRealloc(Context->UnverifiedAccounts, sizeof(UnverifiedUserAccount) * Context->_unverifiedAccountCapacity);
}

static ErrorCode AddUnverifiedAccount(const char* name, const char* surname, const char* email, const char* password)
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;
	UnverifiedAccountListEnsureCapacity(Context->UnverifiedAccountCount + 1);

	UnverifiedUserAccount* Account = Context->UnverifiedAccounts + Context->UnverifiedAccountCount;

	time_t Time = time(NULL);
	if (Time == -1)
	{
		return Error_SetError(ErrorCode_DatabaseError, "Failed to generate time for unverified accounnt.");
	}

	Account->VerificationAttempts = 0;
	Account->VerificationCode = Math_RandomInt();
	Account->VerificationStartTime = Time;
	Account->Name = String_CreateCopy(name);
	Account->Surname = String_CreateCopy(surname);
	Account->Email = String_CreateCopy(email);
	Account->Password = String_CreateCopy(password);

	Context->UnverifiedAccountCount += 1;
	
	return ErrorCode_Success;
}

static UnverifiedUserAccount* GetUnverifiedAccountByEmail(const char* email)
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;

	for (size_t i = 0; i < Context->UnverifiedAccountCount; i++)
	{
		if (String_Equals(email, Context->UnverifiedAccounts[i].Email))
		{
			return Context->UnverifiedAccounts + i;
		}
	}

	return NULL;
}

static void RemoeUnverifiedAccountByIndex(size_t index)
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;

	for (size_t i = index + 1; i < Context->UnverifiedAccountCount; i++)
	{
		Context->UnverifiedAccounts[i - 1] = Context->UnverifiedAccounts[i];
	}
	Context->UnverifiedAccountCount -= 1;
}

static void RemoveUnverifiedAccountByEmail(const char* email)
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;

	for (size_t i = 0; i < Context->UnverifiedAccountCount; i++)
	{
		if (String_Equals(email, Context->UnverifiedAccounts[i].Email))
		{
			RemoeUnverifiedAccountByIndex(i);
			return;
		}
	}
}

static void RefreshUnverifiedAccounts()
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;

	for (long long i = 0; i < (long long)Context->UnverifiedAccountCount; i++)
	{
		
		if (UnverifiedAccountIsExpired(Context->UnverifiedAccounts + i)) 
		{
			// Dangerous thing being done while iterating list, but should be fine because logic works out.
			RemoeUnverifiedAccountByIndex(i);
			i--;
		}
	}
}

/* Session list. */
static void SessionIDListEnsureCapacity(size_t capacity)
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;
	if (Context->_sessionListCapacity >= capacity)
	{
		return;
	}

	while (Context->_sessionListCapacity < capacity)
	{
		Context->_sessionListCapacity *= GENERIC_LIST_GROWTH;
	}
	Context->ActiveSessions = (SessionID*)Memory_SafeRealloc(Context->ActiveSessions, sizeof(SessionID) * Context->_sessionListCapacity);
}

static bool IsSessionIDValuesEqual(unsigned char* idValues1, unsigned char* idValues2)
{
	for (int i = 0; i < SESSION_ID_LENGTH; i++)
	{
		if (idValues1[i] != idValues2[i])
		{
			return false;
		}
	}

	return true;
}

static void SessionIDListAdd(SessionID* sessionID)
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;
	SessionIDListEnsureCapacity(Context->SessionCount + 1);

	Context->ActiveSessions[Context->SessionCount] = *sessionID;
	Context->SessionCount++;
}

static void SessionIDListRemove(unsigned char* idValues)
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;

	size_t RemovalIndex = Context->SessionCount;
	for (size_t i = 0; i < Context->SessionCount; i++)
	{
		if (IsSessionIDValuesEqual(&Context->ActiveSessions[i].IDValues, idValues))
		{
			Context->SessionCount -= 1;
			RemovalIndex = i;
		}
	}

	for (size_t i = RemovalIndex + 1; i < Context->SessionCount + 1; i++)
	{
		Context->ActiveSessions[i - 1] = Context->ActiveSessions[i];
	}
}


/* Account loading and saving. */
static bool ReadAccountFromDatabase(UserAccount* account, unsigned long long id)
{
	Error_ClearError();
	const char* FilePath = ResourceManager_GetPathToIDFile(id, DIR_NAME_ACCOUNTS);
	if (!File_Exists(FilePath))
	{
		Memory_Free(FilePath);
		return false;
	}

	GHDFCompound Compound;
	GHDFEntry* Entry;
	if (GHDFCompound_ReadFromFile(FilePath, &Compound) != ErrorCode_Success)
	{
		Memory_Free(FilePath);
		return false;
	}
	Memory_Free(FilePath);

	AccountSetDefaultValues(account);
	if (GHDFCompound_GetVerifiedEntry(&Compound, ENTRY_ID_ACCOUNT_ID, &Entry, GHDFType_ULong, "Account ID") != ErrorCode_Success)
	{
		goto ReadFailCase;
	}
	if (Entry->Value.SingleValue.ULong != id)
	{
		char Message[128];
		snprintf(Message, sizeof(Message), "Stored and provided ID mismatch (Stored: %llu, provided: %llu)",
			Entry->Value.SingleValue.ULong, id);
		Error_SetError(ErrorCode_DatabaseError, Message);
		goto ReadFailCase;
	}
	account->ID = id;

	if (GHDFCompound_GetVerifiedEntry(&Compound, ENTRY_ID_ACCOUNT_NAME, &Entry, GHDFType_String, "Account Name") != ErrorCode_Success)
	{
		goto ReadFailCase;
	}
	account->Name = String_CreateCopy(Entry->Value.SingleValue.String);
	if (GHDFCompound_GetVerifiedEntry(&Compound, ENTRY_ID_ACCOUNT_SURNAME, &Entry, GHDFType_String, "Account Surname") != ErrorCode_Success)
	{
		goto ReadFailCase;
	}
	account->Surname = String_CreateCopy(Entry->Value.SingleValue.String);
	if (GHDFCompound_GetVerifiedEntry(&Compound, ENTRY_ID_ACCOUNT_EMAIL, &Entry, GHDFType_String, "Account Email") != ErrorCode_Success)
	{
		goto ReadFailCase;
	}
	account->Email = String_CreateCopy(Entry->Value.SingleValue.String);

	if (GHDFCompound_GetVerifiedEntry(&Compound, ENTRY_ID_ACCOUNT_PASSWORD, &Entry, GHDFType_ULong | GHDF_TYPE_ARRAY_BIT,
		"Account Password Hash") != ErrorCode_Success)
	{
		goto ReadFailCase;
	}
	if (Entry->Value.ValueArray.Size != PASSWORD_HASH_LENGTH)
	{
		goto ReadFailCase;
	}
	for (int i = 0; i < PASSWORD_HASH_LENGTH; i++)
	{
		account->PasswordHash[i] = Entry->Value.ValueArray.Array[i].Long;
	}

	if (GHDFCompound_GetVerifiedEntry(&Compound, ENTRY_ID_ACCOUNT_PROFILE_IMAGE_ID, &Entry, GHDFType_ULong, "Account Image ID")
		!= ErrorCode_Success)
	{
		goto ReadFailCase;
	}
	account->ProfileImageID = Entry->Value.SingleValue.ULong;

	Entry = GHDFCompound_GetEntry(&Compound, ENTRY_ID_ACCOUNT_POSTS);
	if (Entry)
	{
		if (Entry->ValueType != (GHDFType_ULong | GHDF_TYPE_ARRAY_BIT))
		{
			Error_SetError(ErrorCode_DatabaseError, "Account Posts array not of type unsigned long array.");
			goto ReadFailCase;
		}
		unsigned long long* PostIDs = (unsigned long long*)Memory_SafeMalloc(sizeof(unsigned long long) * Entry->Value.ValueArray.Size);
		for (unsigned int i = 0; i < Entry->Value.ValueArray.Size; i++)
		{
			PostIDs[i] = Entry->Value.ValueArray.Array[i].ULong;
		}
		account->Posts = PostIDs;
		account->PostCount = Entry->Value.ValueArray.Size;
	}
	else
	{
		account->Posts = NULL;
		account->PostCount = 0;
	}

	if (GHDFCompound_GetVerifiedEntry(&Compound, ENTRY_ID_ACCOUNT_CREATION_TIME, &Entry, GHDFType_Long, "Account Creation Time")
		!= ErrorCode_Success)
	{
		goto ReadFailCase;
	}
	account->CreationTime = Entry->Value.SingleValue.Long;
	if (GHDFCompound_GetVerifiedEntry(&Compound, ENTRY_ID_ACCOUNT_IS_ADMIN, &Entry, GHDFType_Bool, "Account IsAdmin") != ErrorCode_Success)
	{
		goto ReadFailCase;
	}
	account->IsAdmin = Entry->Value.SingleValue.Bool;


	GHDFCompound_Deconstruct(&Compound);
	return true;

	ReadFailCase:
	GHDFCompound_Deconstruct(&Compound);
	AccountDeconstruct(account);
	return false;
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
		PrimitiveArray[i].ULong = account->PasswordHash[i];
	}
	GHDFCompound_AddArrayEntry(&Compound, GHDFType_ULong, ENTRY_ID_ACCOUNT_PASSWORD, PrimitiveArray, PASSWORD_HASH_LENGTH);

	SingleValue.ULong = account->ProfileImageID;
	GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_ULong, ENTRY_ID_ACCOUNT_PROFILE_IMAGE_ID, SingleValue);

	if (account->PostCount > 0)
	{
		PrimitiveArray = (GHDFPrimitive*)Memory_SafeMalloc(sizeof(GHDFPrimitive) * account->PostCount);
		for (unsigned int i = 0; i < account->PostCount; i++)
		{
			PrimitiveArray[i].ULong = account->Posts[i];
		}
		GHDFCompound_AddArrayEntry(&Compound, GHDFType_ULong, ENTRY_ID_ACCOUNT_POSTS, PrimitiveArray, account->PostCount);
	}
	
	SingleValue.Long = account->CreationTime;
	GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_Long, ENTRY_ID_ACCOUNT_CREATION_TIME, SingleValue);
	SingleValue.ULong = account->ID;
	GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_ULong, ENTRY_ID_ACCOUNT_ID, SingleValue);
	SingleValue.Bool = account->IsAdmin;
	GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_Bool, ENTRY_ID_ACCOUNT_IS_ADMIN, SingleValue);

	const char* AccountPath = ResourceManager_GetPathToIDFile(account->ID, DIR_NAME_ACCOUNTS);
	ErrorCode ResultCode = GHDFCompound_WriteToFile(AccountPath, &Compound);
	Memory_Free(AccountPath);
	GHDFCompound_Deconstruct(&Compound);
	return ResultCode;
}


/* Searching database. */
static bool IsEmailInDatabase(const char* email)
{
	size_t IDArraySize;
	unsigned long long* IDs = IDCodepointHashMap_FindByString(
		&LTTServerC_GetCurrentContext()->Resources.AccountContext.EmailMap, email, true, &IDArraySize);
	if (IDArraySize == 0)
	{
		return false;
	}

	bool FoundEmail = false;
	for (size_t i = 0; i < IDArraySize; i++)
	{
		UserAccount Account;
		if (!ReadAccountFromDatabase(&Account, IDs[i]))
		{
			AccountDeconstruct(&Account);
			continue;
		}

		if (String_Equals(email, Account.Email))
		{
			FoundEmail = true;
			AccountDeconstruct(&Account);
			break;
		}
		AccountDeconstruct(&Account);
	}

	Memory_Free(IDs);
	return FoundEmail;
}


/* Hashmap. */
static ErrorCode GenerateMetaInfoForSingleAccount(DBAccountContext* context, UserAccount* account)
{
	char* LowerName = String_CreateCopy(account->Name);
	String_ToLowerUTF8(LowerName);
	char* LowerSurname = String_CreateCopy(account->Surname);
	String_ToLowerUTF8(LowerName);


	IDCodepointHashMap_AddID(&context->NameMap, LowerName, account->ID);
	IDCodepointHashMap_AddID(&context->NameMap, LowerSurname, account->ID);
	IDCodepointHashMap_AddID(&context->EmailMap, account->Email, account->ID);

	Memory_Free(LowerName);
	Memory_Free(LowerSurname);
}

static ErrorCode GenerateMetaInfoForAccounts(DBAccountContext* context)
{
	unsigned long long MaxIDExclusive = context->AvailableAccountID;
	for (unsigned long long ID = 0; ID < MaxIDExclusive; ID++)
	{
		UserAccount Account;
		if (!ReadAccountFromDatabase(&Account, ID))
		{
			if (Error_GetLastErrorCode() != ErrorCode_Success)
			{
				return Error_GetLastErrorCode();
			}
			continue;
		}

		if (GenerateMetaInfoForSingleAccount(context, &Account) != ErrorCode_Success)
		{
			return Error_GetLastErrorCode();
		}
		AccountDeconstruct(&Account);
	}

	return ErrorCode_Success;
}


// Functions.
ErrorCode AccountManager_ConstructContext(DBAccountContext* context, unsigned long long availableID, unsigned long long availableImageID)
{
	context->AvailableAccountID = availableID;
	context->AvailableImageID = availableImageID;
	IDCodepointHashMap_Construct(&context->NameMap);
	IDCodepointHashMap_Construct(&context->EmailMap);

	context->ActiveSessions = (SessionID*)Memory_SafeMalloc(sizeof(SessionID) * GENERIC_LIST_CAPACITY);
	context->SessionCount = 0;
	context->_sessionListCapacity = 0;

	context->UnverifiedAccounts = (UnverifiedUserAccount*)Memory_SafeMalloc(sizeof(UnverifiedUserAccount) * GENERIC_LIST_CAPACITY);
	context->UnverifiedAccountCount = 0;
	context->_unverifiedAccountCapacity = GENERIC_LIST_CAPACITY;

	if (GenerateMetaInfoForAccounts(context) != ErrorCode_Success)
	{
		return Error_GetLastErrorCode();
	}

	return ErrorCode_Success;
}

void AccountManager_CloseContext(DBAccountContext* context)
{
	IDCodepointHashMap_Deconstruct(&context->NameMap);
	IDCodepointHashMap_Deconstruct(&context->EmailMap);
	Memory_Free(context->ActiveSessions);
	Memory_Free(context->UnverifiedAccounts);
}

bool AccountManager_TryCreateAccount(UserAccount* account,
	const char* name,
	const char* surname,
	const char* email,
	const char* password)
{
	Error_ClearError();
	if (!VerifyName(name) || !VerifyName(surname) || !VerifyPassword(password) || !VerifyEmail(email) || IsEmailInDatabase(email))
	{
		return false;
	}

	const char* AccountFilePath = ResourceManager_GetPathToIDFile(GetAccountID(), DIR_NAME_ACCOUNTS);
	if (File_Exists(AccountFilePath))
	{
		Memory_Free(AccountFilePath);
		char Message[256];
		snprintf(Message, sizeof(Message), "Account with ID %llu already exists in database (file match), but no email entry was found and "
			"an attempt to creating this account was made. Corrupted database?", GetAccountID());
		Error_SetError(ErrorCode_DatabaseError, Message);
		LTTServerC_GetCurrentContext()->Resources.AccountContext.AvailableAccountID += 1; // Increment so possibly working ID could be found.
		return false;
	}
	Memory_Free(AccountFilePath);

	AccountSetDefaultValues(account);
	if (AccountCreateNew(account, name, surname, email, password) != ErrorCode_Success)
	{
		return false;
	}

	if (WriteAccountToDatabase(account) != ErrorCode_Success)
	{
		AccountDeconstruct(account);
		return false;
	}

	GenerateMetaInfoForSingleAccount(&LTTServerC_GetCurrentContext()->Resources.AccountContext, account);
	return true;
}

bool AccountManager_TryCreateUnverifiedAccount(const char* name,
	const char* surname,
	const char* email,
	const char* password)
{
	Error_ClearError();

	if (!VerifyName(name) || !VerifyName(surname) || !VerifyPassword(password) || !VerifyEmail(email))
	{
		return false;
	}

	if (IsEmailInDatabase(email))
	{
		return false;
	}

	if (AddUnverifiedAccount(name, surname, email, password) != ErrorCode_Success)
	{
		return false;
	}

	return true;
}

bool AccountManager_IsSessionAdmin(unsigned char* sessionIdValues)
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;

	for (size_t i = 0; i < Context->SessionCount; i++)
	{
		if (IsSessionIDValuesEqual(&Context->ActiveSessions[i].IDValues, sessionIdValues))
		{
			return Context->ActiveSessions[i].IsAdmin;
		}
	}
	return false;
}

bool AccountManager_GetAccount(UserAccount* account, unsigned long long id)
{
	Error_ClearError();
	return ReadAccountFromDatabase(account, id);
}



ErrorCode AccountManager_VerifyAccount(const char* email)
{
	RefreshUnverifiedAccounts();

	UnverifiedUserAccount* UnverifiedAccount = GetUnverifiedAccountByEmail(email);
	if (!UnverifiedAccount)
	{
		return ErrorCode_Success;
	}

	UserAccount Account;
	if (!AccountManager_TryCreateAccount(&Account, UnverifiedAccount->Name, UnverifiedAccount->Surname,
		UnverifiedAccount->Surname, UnverifiedAccount->Password))
	{
		return Error_GetLastErrorCode();
	}

	AccountDeconstruct(&Account);
	RemoveUnverifiedAccountByEmail(email);
	return ErrorCode_Success;
}

bool AccountManager_TryVerifyAccount(const char* email, int code)
{
	RefreshUnverifiedAccounts();

	UnverifiedUserAccount* UnverifiedAccount = GetUnverifiedAccountByEmail(email);
	if (!UnverifiedAccount)
	{
		return false;
	}

	if (!UnverifiedAccountTryVerify(UnverifiedAccount, code))
	{
		return false;
	}

	UserAccount Account;
	if (!AccountManager_TryCreateAccount(&Account, UnverifiedAccount->Name, UnverifiedAccount->Surname,
		UnverifiedAccount->Email, UnverifiedAccount->Password))
	{
		return false;
	}

	AccountDeconstruct(&Account);
	RemoveUnverifiedAccountByEmail(email);
	return true;
}