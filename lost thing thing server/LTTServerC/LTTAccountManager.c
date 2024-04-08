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
#include <limits.h>


// Macros.
#define DIR_NAME_ACCOUNTS "accounts"

#define GENERIC_LIST_CAPACITY 8
#define GENERIC_LIST_GROWTH 2

#define MAX_ACCOUNT_VERIFICATION_TIME 60 * 5
#define MAX_VERIFICATION_ATTEMPTS 3

#define ACCOUNT_CACHE_CAPACITY 256
#define ACCOUNT_CACHE_UNLOADED_TIME -1


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
	unsigned long long PasswordLength = (unsigned long long)String_LengthBytes(password);

	for (int i = 0; i < PASSWORD_HASH_LENGTH; i++)
	{
		unsigned long long HashNumber = 9;
		for (int j = 0; j < PasswordLength; j++)
		{
			HashNumber += (unsigned long long)password[j] * (i + 57 + i + password[j]);
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
	Memory_Set((unsigned char*)account, sizeof(UserAccount), 0);
}

static void AccountDeconstruct(UserAccount* account)
{
	if (account->Name)
	{
		Memory_Free((char*)account->Name);
	}
	if (account->Surname)
	{
		Memory_Free((char*)account->Surname);
	}
	if (account->Email)
	{
		Memory_Free((char*)account->Email);
	}
	if (account->Posts)
	{
		Memory_Free((char*)account->Posts);
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
	String_ToLowerUTF8((char*)account->Email);
	GeneratePasswordHash(account->PasswordHash, password);
	account->CreationTime = CurrentTime;
	account->PostCount = 0;
	account->Posts = NULL;
	account->ProfileImageID = NO_ACCOUNT_IMAGE_ID;
	account->IsAdmin = false;

	return ErrorCode_Success;
}

static bool PasswordHashEquals(unsigned long long* hash1, unsigned long long* hash2)
{
	for (int i = 0; i < PASSWORD_HASH_LENGTH; i++)
	{
		if (hash1[i] != hash2[i])
		{
			return false;
		}
	}
	return true;
}


/* Unverified account. */
static void UnverifiedAccountDeconstruct(UnverifiedUserAccount* account)
{
	Memory_Free((char*)account->Name);
	Memory_Free((char*)account->Surname);
	Memory_Free((char*)account->Email);
	Memory_Free((char*)account->Password);
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
	String_ToLowerUTF8((char*)Account->Email);
	Account->Password = String_CreateCopy(password);

	Context->UnverifiedAccountCount += 1;
	
	return ErrorCode_Success;
}

static UnverifiedUserAccount* GetUnverifiedAccountByEmail(const char* email)
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;

	for (size_t i = 0; i < Context->UnverifiedAccountCount; i++)
	{
		if (String_EqualsCaseInsensitive(email, Context->UnverifiedAccounts[i].Email))
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
		if (String_EqualsCaseInsensitive(email, Context->UnverifiedAccounts[i].Email))
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

static bool IsSessionIDValuesEqual(unsigned int* idValues1, unsigned int* idValues2)
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

static void GenerateSessionID(unsigned int* sessionCode, const char* email)
{
	time_t Time = time(NULL);
	unsigned int EmailLength = (int)String_LengthBytes(email);

	for (unsigned int CodeIndex = 0; CodeIndex < SESSION_ID_LENGTH; CodeIndex++)
	{
		sessionCode[CodeIndex] *= 0xac3f201;
		for (unsigned int EmailIndex = 0; EmailIndex < EmailLength; EmailIndex++)
		{
			sessionCode[CodeIndex] += (unsigned int)Time * (EmailIndex + 2189) * CodeIndex;
			sessionCode[CodeIndex] *= (unsigned int)email[EmailIndex] ^ (unsigned int)Time + CodeIndex;
			float FloatValue = (float)Time * 4.25f - (float)EmailIndex;
			sessionCode[CodeIndex] -= *((unsigned int*)&FloatValue);
			sessionCode[CodeIndex] += sessionCode[CodeIndex] * (int)(((unsigned long long)sessionCode & (unsigned long long)email));
			sessionCode[CodeIndex] *= (int)(((unsigned long long)&EmailLength) & ((unsigned long long)&Time));
		}
	}

	for (unsigned int i = 0; i < SESSION_ID_LENGTH; i++)
	{
		unsigned int Index1 = sessionCode[i] % SESSION_ID_LENGTH;
		unsigned int Value1 = sessionCode[Index1];
		unsigned int Index2 = sessionCode[Value1 % SESSION_ID_LENGTH] % SESSION_ID_LENGTH;
		unsigned int Value2 = sessionCode[Index2];
		
		sessionCode[Index1] = Value2;
		sessionCode[Index2] = Value1;
	}
}

static SessionID* GetSessionByID(unsigned long long id)
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;

	for (size_t i = 0; i < Context->SessionCount; i++)
	{
		if (Context->ActiveSessions[i].AccountID == id)
		{
			return Context->ActiveSessions + i;
		}
	}

	return NULL;;
}

static SessionID* CreateSession(UserAccount* account)
{
	SessionID* Session = GetSessionByID(account->ID);
	if (Session)
	{
		Session->SessionStartTime = time(NULL);
		return Session;
	}

	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;
	SessionIDListEnsureCapacity(Context->SessionCount + 1);

	Session = &Context->ActiveSessions[Context->SessionCount];
	Session->AccountID = account->ID;
	GenerateSessionID(Session->IDValues, account->Email);
	Session->SessionStartTime = time(NULL);
	return Session;
}

static void RemoveSessionByIndex(size_t index)
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;

	for (size_t i = index + 1; i < Context->SessionCount; i++)
	{
		Context->ActiveSessions[i - 1] = Context->ActiveSessions[i];
	}
	Context->SessionCount -= 1;
}

static void RemoveSessionByID(unsigned long long id)
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;

	for (size_t i = 0; i < Context->SessionCount; i++)
	{
		if (Context->ActiveSessions[i].AccountID == id)
		{
			RemoveSessionByIndex(i);
			return;
		}
	}
}

static void RefreshSessions()
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;
	time_t CurrentTime = time(NULL);

	for (size_t i = 0; i < Context->SessionCount; i++)
	{
		if (CurrentTime - Context->ActiveSessions[i].SessionStartTime > MAX_SESSION_TIME)
		{
			// Again, removing elements while iterating but should work out.
			RemoveSessionByIndex(i);
			i--;
		}
	}
}

static void CreateSessionContext(DBAccountContext* context, SessionID* sessions, size_t sessionCount)
{
	context->_sessionListCapacity = Math_Max(GENERIC_LIST_CAPACITY, sessionCount);
	context->ActiveSessions = (SessionID*)Memory_SafeMalloc(sizeof(SessionID) * context->_sessionListCapacity);
	context->SessionCount = sessionCount;

	time_t CurrentTime = time(NULL);
	for (size_t SourceIndex = 0, TargetIndex = 0; SourceIndex < sessionCount; SourceIndex++)
	{
		if (CurrentTime - sessions[SourceIndex].SessionStartTime > MAX_SESSION_TIME)
		{
			continue;
		}

		*(context->ActiveSessions + TargetIndex) = sessions[SourceIndex];
		TargetIndex++;
	}
}


/* Account loading and saving. */
static bool ReadAccountFromDatabase(UserAccount* account, unsigned long long id)
{
	Error_ClearError();
	const char* FilePath = ResourceManager_GetPathToIDFile(id, DIR_NAME_ACCOUNTS);
	if (!File_Exists(FilePath))
	{
		Memory_Free((char*)FilePath);
		return false;
	}

	GHDFCompound Compound;
	GHDFEntry* Entry;
	if (GHDFCompound_ReadFromFile(FilePath, &Compound) != ErrorCode_Success)
	{
		Memory_Free((char*)FilePath);
		return false;
	}
	Memory_Free((char*)FilePath);

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
	Memory_Free((char*)AccountPath);
	GHDFCompound_Deconstruct(&Compound);
	return ResultCode;
}

static bool DeleteAccountFromDatabase(unsigned long long id)
{
	const char* Path = ResourceManager_GetPathToIDFile(id, DIR_NAME_ACCOUNTS);
	ErrorCode Result = File_Delete(Path);
	Memory_Free((char*)Path);
	return Result == ErrorCode_Success;
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
		UserAccount* Account = AccountManager_GetAccountByID(IDs[i]);

		if (!Account)
		{
			continue;
		}

		if (String_EqualsCaseInsensitive(email, Account->Email))
		{
			FoundEmail = true;
			break;
		}
	}

	Memory_Free(IDs);
	return FoundEmail;
}

static UserAccount** SearchAccountFromIDsByName(unsigned long long* ids, size_t idCount, const char* name, size_t* accountCount)
{
	UserAccount** AccountArray = (UserAccount**)Memory_SafeMalloc(sizeof(UserAccount*) * idCount);
	size_t MatchedAccountCount = 0;
	for (size_t i = 0; i < idCount; i++)
	{
		UserAccount* Account = AccountManager_GetAccountByID(ids[i]);

		char* NameSurname = String_Concatenate(Account->Name, Account->Surname);
		char* SurnameName = String_Concatenate(Account->Surname, Account->Name);

		bool IsMatch = String_IsFuzzyMatched(NameSurname, name, true)
			|| String_IsFuzzyMatched(SurnameName, name, true);

		Memory_Free(NameSurname);
		Memory_Free(SurnameName);

		if (IsMatch)
		{
			AccountArray[MatchedAccountCount] = Account;
			MatchedAccountCount++;
		}
	}

	*accountCount = MatchedAccountCount;
	if (MatchedAccountCount == 0)
	{
		Memory_Free(AccountArray);
		return NULL;
	}
	return AccountArray;
}


/* Hashmap. */
static ErrorCode GenerateMetaInfoForSingleAccount(DBAccountContext* context, UserAccount* account)
{
	IDCodepointHashMap_AddID(&context->NameMap, account->Name, account->ID);
	IDCodepointHashMap_AddID(&context->NameMap, account->Surname, account->ID);
	IDCodepointHashMap_AddID(&context->EmailMap, account->Email, account->ID);

	return ErrorCode_Success;
}

static ErrorCode GenerateMetaInfoForAccounts(DBAccountContext* context, size_t* readAccountCount)
{
	unsigned long long MaxIDExclusive = context->AvailableAccountID;
	size_t ReadAccounts = 0;
	*readAccountCount = 0;
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

		ReadAccounts++;
		if (GenerateMetaInfoForSingleAccount(context, &Account) != ErrorCode_Success)
		{
			return Error_GetLastErrorCode();
		}

		AccountDeconstruct(&Account);
	}

	*readAccountCount = ReadAccounts;
	return ErrorCode_Success;
}

static void ClearMetaInfoForAccount(UserAccount* account)
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;

	IDCodepointHashMap_RemoveID(&Context->NameMap, account->Name, account->ID);
	IDCodepointHashMap_RemoveID(&Context->NameMap, account->Surname, account->ID);
	IDCodepointHashMap_RemoveID(&Context->EmailMap, account->Email, account->ID);
}


/* Account cache. */
static UserAccount* TryGetAccountFromCache(unsigned long long id)
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;

	for (int i = 0; i < ACCOUNT_CACHE_CAPACITY; i++)
	{
		CachedAccount* TargetCachedAccount = (Context->AccountCache + i);
		if ((TargetCachedAccount->LastAccessTime != ACCOUNT_CACHE_UNLOADED_TIME) && (TargetCachedAccount->Account.ID == id))
		{
			TargetCachedAccount->LastAccessTime = time(NULL);
			return &TargetCachedAccount->Account;
		}
	}

	return NULL;
}

static CachedAccount* GetCacheSpotForAccount()
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;

	time_t LowestLoadTime = LLONG_MAX;
	int LowestLoadTimeIndex = 0;

	for (int i = 0; i < ACCOUNT_CACHE_CAPACITY; i++)
	{
		if (Context->AccountCache[i].LastAccessTime < LowestLoadTime)
		{
			LowestLoadTimeIndex = i;
			LowestLoadTime = Context->AccountCache[i].LastAccessTime;
		}
	}

	if (Context->AccountCache[LowestLoadTimeIndex].LastAccessTime != ACCOUNT_CACHE_UNLOADED_TIME)
	{
		WriteAccountToDatabase(&Context->AccountCache[LowestLoadTimeIndex].Account);
		AccountDeconstruct(&Context->AccountCache[LowestLoadTimeIndex].Account);
	}

	return Context->AccountCache + LowestLoadTimeIndex;
}

static CachedAccount* LoadAccountIntoCache(unsigned long long id)
{
	CachedAccount* AccountSpot = GetCacheSpotForAccount();
	if (!ReadAccountFromDatabase(&AccountSpot->Account, id))
	{
		AccountSpot->LastAccessTime = ACCOUNT_CACHE_UNLOADED_TIME;
		return NULL;
	}

	AccountSpot->LastAccessTime = time(NULL);
	return AccountSpot;
}

static void SaveAllCachedAccountsToDatabase()
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;

	for (int i = 0; i < ACCOUNT_CACHE_CAPACITY; i++)
	{
		if (Context->AccountCache[i].LastAccessTime != ACCOUNT_CACHE_UNLOADED_TIME)
		{
			WriteAccountToDatabase(&Context->AccountCache[i].Account);
		}
	}
}

static void RemoveAccountFromCacheByID(unsigned long long id)
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;

	for (int i = 0; i < ACCOUNT_CACHE_CAPACITY; i++)
	{
		if (Context->AccountCache[i].Account.ID == id)
		{
			Context->AccountCache[i].LastAccessTime = ACCOUNT_CACHE_UNLOADED_TIME;
			return;
		}
	}
}


// Functions.
ErrorCode AccountManager_ConstructContext(DBAccountContext* context,
	unsigned long long availableID,
	unsigned long long availableImageID,
	SessionID* sessions,
	size_t sessionCount)
{
	context->AvailableAccountID = availableID;
	context->AvailableImageID = availableImageID;
	IDCodepointHashMap_Construct(&context->NameMap);
	IDCodepointHashMap_Construct(&context->EmailMap);

	context->UnverifiedAccounts = (UnverifiedUserAccount*)Memory_SafeMalloc(sizeof(UnverifiedUserAccount) * GENERIC_LIST_CAPACITY);
	context->UnverifiedAccountCount = 0;
	context->_unverifiedAccountCapacity = GENERIC_LIST_CAPACITY;

	CreateSessionContext(context, sessions, sessionCount);

	size_t ReadAccountCount;
	if (GenerateMetaInfoForAccounts(context, &ReadAccountCount) != ErrorCode_Success)
	{
		return Error_GetLastErrorCode();
	}
	char Message[128];
	snprintf(Message, sizeof(Message), "Read %llu accounts while creating ID hashes.", ReadAccountCount);
	Logger_LogInfo(Message);

	context->AccountCache = (CachedAccount*)Memory_SafeMalloc(sizeof(CachedAccount) * ACCOUNT_CACHE_CAPACITY);
	for (int i = 0; i < ACCOUNT_CACHE_CAPACITY; i++)
	{
		context->AccountCache[i].LastAccessTime = ACCOUNT_CACHE_UNLOADED_TIME;
		AccountSetDefaultValues(&context->AccountCache[i].Account);
	}

	return ErrorCode_Success;
}

void AccountManager_CloseContext(DBAccountContext* context)
{
	SaveAllCachedAccountsToDatabase();
	IDCodepointHashMap_Deconstruct(&context->NameMap);
	IDCodepointHashMap_Deconstruct(&context->EmailMap);
	Memory_Free(context->ActiveSessions);
	Memory_Free(context->UnverifiedAccounts);
	Memory_Free(context->AccountCache);
}

UserAccount* AccountManager_TryCreateAccount(const char* name,
	const char* surname,
	const char* email,
	const char* password)
{
	Error_ClearError();
	if (!VerifyName(name) || !VerifyName(surname) || !VerifyPassword(password) || !VerifyEmail(email) || IsEmailInDatabase(email))
	{
		return NULL;
	}

	const char* AccountFilePath = ResourceManager_GetPathToIDFile(GetAccountID(), DIR_NAME_ACCOUNTS);
	if (File_Exists(AccountFilePath))
	{
		Memory_Free((char*)AccountFilePath);
		char Message[256];
		snprintf(Message, sizeof(Message), "Account with ID %llu already exists in database (file match), but no email entry was found and "
			"an attempt to creating this account was made. Corrupted database?", GetAccountID());
		Error_SetError(ErrorCode_DatabaseError, Message);
		LTTServerC_GetCurrentContext()->Resources.AccountContext.AvailableAccountID += 1; // Increment so possibly working ID could be found.
		return NULL;
	}
	Memory_Free((char*)AccountFilePath);

	UserAccount Account;
	AccountSetDefaultValues(&Account);
	if (AccountCreateNew(&Account, name, surname, email, password) != ErrorCode_Success)
	{
		return NULL;
	}

	if (WriteAccountToDatabase(&Account) != ErrorCode_Success)
	{
		AccountDeconstruct(&Account);
		return NULL;
	}

	GenerateMetaInfoForSingleAccount(&LTTServerC_GetCurrentContext()->Resources.AccountContext, &Account);
	unsigned long long ID = Account.ID;
	AccountDeconstruct(&Account);
	return &LoadAccountIntoCache(ID)->Account;
}

bool AccountManager_TryCreateUnverifiedAccount(const char* name,
	const char* surname,
	const char* email,
	const char* password)
{
	Error_ClearError();

	if (!VerifyName(name) || !VerifyName(surname) || !VerifyPassword(password) || !VerifyEmail(email) || IsEmailInDatabase(email))
	{
		return false;
	}

	if (AddUnverifiedAccount(name, surname, email, password) != ErrorCode_Success)
	{
		return false;
	}

	return true;
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

	if (!AccountManager_TryCreateAccount(UnverifiedAccount->Name, UnverifiedAccount->Surname,
		UnverifiedAccount->Email, UnverifiedAccount->Password))
	{
		return false;
	}

	RemoveUnverifiedAccountByEmail(email);
	return true;
}

ErrorCode AccountManager_VerifyAccount(const char* email)
{
	RefreshUnverifiedAccounts();

	UnverifiedUserAccount* UnverifiedAccount = GetUnverifiedAccountByEmail(email);
	if (!UnverifiedAccount)
	{
		return ErrorCode_Success;
	}

	if (!AccountManager_TryCreateAccount(UnverifiedAccount->Name, UnverifiedAccount->Surname,
		UnverifiedAccount->Surname, UnverifiedAccount->Password))
	{
		return Error_GetLastErrorCode();
	}

	RemoveUnverifiedAccountByEmail(email);
	return ErrorCode_Success;
}

UserAccount* AccountManager_GetAccountByID(unsigned long long id)
{
	UserAccount* Account = TryGetAccountFromCache(id);

	if (Account)
	{
		return Account;
	}

	CachedAccount* AccountCached = LoadAccountIntoCache(id);
	return AccountCached ? &AccountCached->Account : NULL;
}

UserAccount** AccountManager_GetAccountsByName(const char* name, size_t* accountCount)
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;

	size_t IDCount;
	unsigned long long* IDs = IDCodepointHashMap_FindByString(&Context->NameMap, name, true, &IDCount);
	if (IDCount == 0)
	{
		return NULL;
	}

	UserAccount** FoundAccounts = SearchAccountFromIDsByName(IDs, IDCount, name, accountCount);

	Memory_Free(IDs);

	return FoundAccounts;
}

UserAccount* AccountManager_GetAccountByEmail(const char* email)
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;
	size_t IDCount;

	unsigned long long* IDs = IDCodepointHashMap_FindByString(&Context->EmailMap, email, true, &IDCount);
	if (IDCount == 0)
	{
		return NULL;
	}

	UserAccount* FoundAccount = NULL;
	for (size_t i = 0; i < IDCount; i++)
	{
		UserAccount* Account = AccountManager_GetAccountByID(IDs[i]);

		if (String_EqualsCaseInsensitive(Account->Email, email))
		{
			FoundAccount = Account;
			break;
		}
	}

	Memory_Free(IDs);
	return FoundAccount;
}

void AccountManager_DeleteAccount(UserAccount* account)
{
	char Message[256];
	snprintf(Message, sizeof(Message), "Deleting account with ID %llu and email %s", account->ID, account->Email);
	Logger_LogInfo(Message);

	RemoveSessionByID(account->ID);
	ClearMetaInfoForAccount(account);
	RemoveAccountFromCacheByID(account->ID);
	DeleteAccountFromDatabase(account->ID);
	AccountDeconstruct(account);
}

void AccountManager_DeleteAllAccounts()
{
	Logger_LogWarning("DELETING ALL ACCOUNTS");

	for (unsigned long long i = 0; i < GetAccountID(); i++)
	{
		UserAccount* Account = AccountManager_GetAccountByID(i);
		if (!Account)
		{
			continue;
		}

		AccountManager_DeleteAccount(Account);
	}
}

bool AccountManager_IsSessionAdmin(unsigned int* sessionIdValues)
{
	DBAccountContext* Context = &LTTServerC_GetCurrentContext()->Resources.AccountContext;

	for (size_t i = 0; i < Context->SessionCount; i++)
	{
		if (IsSessionIDValuesEqual(Context->ActiveSessions[i].IDValues, sessionIdValues))
		{
			return AccountManager_GetAccountByID(Context->ActiveSessions[i].AccountID)->IsAdmin;
		}
	}
	return false;
}

SessionID* AccountManager_TryCreateSession(UserAccount* account, const char* password)
{
	unsigned long long PasswordHash[16];
	GeneratePasswordHash(PasswordHash, password);
	if (!PasswordHashEquals(PasswordHash, account->PasswordHash))
	{
		return NULL;
	}

	return CreateSession(account);
}

SessionID* AccountManager_CreateSession(UserAccount* account)
{
	CreateSession(account);
}