#include "LTTAccountManager.h"
#include "GHDF.h"
#include "LTTServerC.h"
#include "Memory.h"
#include "math.h"
#include "File.h"
#include "LTTChar.h"
#include "LTTMath.h"
#include "Directory.h"
#include <limits.h>
#include "Logger.h"
#include "ConfigFile.h"
#include "LTTServerResourceManager.h"


// Macros.
/* Directories. */
#define DIR_NAME_ACCOUNTS "accounts"

#define ACCOUNT_METAINFO_FILE_NAME "account_meta" GHDF_FILE_EXTENSION

#define ENTRY_FOLDER_NAME_DIVIDER 1000
#define ACCOUNT_ENTRIES_DIR_NAME "entries"
#define IMAGE_ENTRIES_DIR_NAME "images"

/* List. */
#define GENERIC_LIST_CAPACITY 8
#define GENERIC_LIST_GROWTH 2

/* Unverified accounts. */
#define MAX_ACCOUNT_VERIFICATION_TIME 60 * 5
#define MAX_VERIFICATION_ATTEMPTS 3

/* Account cache/ */
#define ACCOUNT_CACHE_CAPACITY 256
#define ACCOUNT_CACHE_UNLOADED_TIME -1


/* Account meta-info file. */
#define DEFAULT_AVAILABLE_ACCOUNT_ID 1
#define DEFAILT_SESSION_COUNT 0

#define ENTRY_ID_METAINFO_AVAILABLE_ID 1 // ulong
#define ENTRY_ID_METAINFO_SESSION_ARRAY 2 // compound array with variable size, MAY NOT EXIST.
#define ENTRY_ID_METAINFO_SESSION_ACCOUNT_ID 1 // ulong
#define ENTRY_ID_METAINFO_SESSION_START_TIME 2 // long
#define ENTRY_ID_METAINFO_SESSION_IDVALUES 3 // uint array of length SESSION_ID_LENGTH


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

#define PROFILE_IMAGE_MAX_SIZE 512
#define PROFILE_IMAGE_COLOR_CHANNEL_COUNT 3


/* Account struct. */
#define ENTRY_ID_ACCOUNT_NAME 1 // String
#define ENTRY_ID_ACCOUNT_SURNAME 2 // String
#define ENTRY_ID_ACCOUNT_EMAIL 3 // String
#define ENTRY_ID_ACCOUNT_PASSWORD 4 // ulong array with length PASSWORD_HASH_LENGTH
#define ENTRY_ID_ACCOUNT_POSTS 5 // ulong array with length >= 1, MAY NOT BE PRESENT IF ACCOUNT HAS NO POSTS
#define ENTRY_ID_ACCOUNT_CREATION_TIME 6 // long
#define ENTRY_ID_ACCOUNT_ID 7 // ulong
#define ENTRY_ID_ACCOUNT_IS_ADMIN 8 // bool


// Types.
typedef struct CachedAccountStruct
{
	time_t LastAccessTime;
	UserAccount Account;
} CachedAccount;

typedef struct UnverifiedUserAccountStruct
{
	int VerificationCode;
	int VerificationAttempts;
	time_t VerificationStartTime;
	const char* Name;
	const char* Surname;
	const char* Email;
	const char* Password;
} UnverifiedUserAccount;


// Static functions.
/* IDs. */
static unsigned long long GetAndUseAccountID(DBAccountContext* context)
{
	unsigned long long ID = context->AvailableAccountID;
	context->AvailableAccountID += 1;
	return ID;
}

static unsigned long long GetAccountID(DBAccountContext* context)
{
	 return context->AvailableAccountID;
}

static const char* GetPathToIDFile(DBAccountContext* context, unsigned long long id, const char* dirName, const char* extension)
{
	StringBuilder Builder;
	StringBuilder_Construct(&Builder, DEFAULT_STRING_BUILDER_CAPACITY);

	StringBuilder_Append(&Builder, context->AccountRootPath);

	StringBuilder_AppendChar(&Builder, PATH_SEPARATOR);
	StringBuilder_Append(&Builder, dirName);

	StringBuilder_AppendChar(&Builder, PATH_SEPARATOR);
	char NumberString[32];
	sprintf(NumberString, "%llu", id / ENTRY_FOLDER_NAME_DIVIDER);
	StringBuilder_Append(&Builder, NumberString);
	StringBuilder_AppendChar(&Builder, 'k');

	StringBuilder_AppendChar(&Builder, PATH_SEPARATOR);
	sprintf(NumberString, "%llu", id);
	StringBuilder_Append(&Builder, NumberString);
	StringBuilder_Append(&Builder, extension);

	return Builder.Data;
}


/* Data verification and generation. */
static bool VerifyName(const char* name)
{
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

static const char* VerifyEmailDomain(const char* email, const char** domains, size_t domainCount)
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

	for (size_t i = 0; i < domainCount; i++)
	{
		if (String_Equals(domains[i], email))
		{
			return ShiftedString;
		}
	}

	return NULL;
}

static bool VerifyEmail(const char* email, const char** domains, size_t domainCount)
{
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

	ParsedEmailPosition = VerifyEmailDomain(ParsedEmailPosition, domains, domainCount);
	if (!ParsedEmailPosition)
	{
		return false;
	}

	return true;
}

static void GeneratePasswordHash(unsigned long long* longArray, const char* password) // World's most secure hash, literally impossible to crack.
{
	unsigned long long PasswordLength = (unsigned long long)String_LengthBytes(password);

	for (unsigned long long i = 0; i < PASSWORD_HASH_LENGTH; i++)
	{
		unsigned long long HashNumber = 9;
		for (unsigned long long j = 0; j < PasswordLength; j++)
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
	Memory_Set((char*)account, sizeof(UserAccount), 0);
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
	if (account->ProfileImageData)
	{
		Memory_Free((char*)account->ProfileImageData);
	}
}

static Error AccountCreateNew(DBAccountContext* context,
	UserAccount* account,
	const char* name,
	const char* surname,
	const char* email,
	const char* password)
{
	time_t CurrentTime = time(NULL);
	if (CurrentTime == -1)
	{
		return Error_CreateError(ErrorCode_DatabaseError, "AccountManager_TryCreateUser: Failed to generate account creation time.");
	}
	account->ID = GetAndUseAccountID(context);
	account->Name = String_CreateCopy(name);
	account->Surname = String_CreateCopy(surname);
	account->Email = String_CreateCopy(email);
	String_ToLowerUTF8((char*)account->Email);
	GeneratePasswordHash(account->PasswordHash, password);
	account->CreationTime = CurrentTime;
	account->PostCount = 0;
	account->Posts = NULL;
	account->IsAdmin = false;
	account->ProfileImageData = NULL;

	return Error_CreateSuccess();
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

static void UnverifiedAccountListEnsureCapacity(DBAccountContext* context, size_t capacity)
{
	if (context->_unverifiedAccountCapacity >= capacity)
	{
		return;
	}

	while (context->_unverifiedAccountCapacity < capacity)
	{
		context->_unverifiedAccountCapacity *= GENERIC_LIST_GROWTH;
	}
	context->UnverifiedAccounts = (UnverifiedUserAccount*)
		Memory_SafeRealloc(context->UnverifiedAccounts, sizeof(UnverifiedUserAccount)
			* context->_unverifiedAccountCapacity);
}

static void AddUnverifiedAccount(DBAccountContext* context, const char* name, const char* surname, const char* email, const char* password)
{
	UnverifiedAccountListEnsureCapacity(context, context->UnverifiedAccountCount + 1);

	UnverifiedUserAccount* Account = context->UnverifiedAccounts + context->UnverifiedAccountCount;

	Account->VerificationAttempts = 0;
	Account->VerificationCode = 0; //Math_RandomInt(); // <--- REPLACE IN FINAL PRODUCT WITH MATH_RANDOMINT INSTEAD OF CONSTANT 0
	Account->VerificationStartTime = (long)time(NULL);;
	Account->Name = String_CreateCopy(name);
	Account->Surname = String_CreateCopy(surname);
	Account->Email = String_CreateCopy(email);
	String_ToLowerUTF8((char*)Account->Email);
	Account->Password = String_CreateCopy(password);

	context->UnverifiedAccountCount += 1;
}

static UnverifiedUserAccount* GetUnverifiedAccountByEmail(DBAccountContext* context, const char* email)
{
	for (size_t i = 0; i < context->UnverifiedAccountCount; i++)
	{
		if (String_EqualsCaseInsensitive(email, context->UnverifiedAccounts[i].Email))
		{
			return context->UnverifiedAccounts + i;
		}
	}

	return NULL;
}

static void RemoveUnverifiedAccountByIndex(DBAccountContext* context, size_t index)
{
	if (context->UnverifiedAccountCount == 0)
	{
		return;
	}

	UnverifiedAccountDeconstruct(context->UnverifiedAccounts + index);
	for (size_t i = index + 1; i < context->UnverifiedAccountCount; i++)
	{
		context->UnverifiedAccounts[i - 1] = context->UnverifiedAccounts[i];
	}
	context->UnverifiedAccountCount -= 1;
}

static void RemoveUnverifiedAccountByEmail(DBAccountContext* context, const char* email)
{
	for (size_t i = 0; i < context->UnverifiedAccountCount; i++)
	{
		if (String_EqualsCaseInsensitive(email, context->UnverifiedAccounts[i].Email))
		{
			RemoveUnverifiedAccountByIndex(context, i);
			return;
		}
	}
}

static void RefreshUnverifiedAccounts(DBAccountContext* context)
{
	for (long long i = 0; i < (long long)context->UnverifiedAccountCount; i++)
	{
		
		if (UnverifiedAccountIsExpired(context->UnverifiedAccounts + i))
		{
			// Dangerous thing being done while iterating list, but should be fine because logic works out.
			RemoveUnverifiedAccountByIndex(context, i);
			i--;
		}
	}
}


/* Session list. */
static void SessionIDListEnsureCapacity(DBAccountContext* context, size_t capacity)
{
	if (context->_sessionListCapacity >= capacity)
	{
		return;
	}

	while (context->_sessionListCapacity < capacity)
	{
		context->_sessionListCapacity *= GENERIC_LIST_GROWTH;
	}
	context->ActiveSessions = (SessionID*)Memory_SafeRealloc(context->ActiveSessions, sizeof(SessionID) * context->_sessionListCapacity);
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

static SessionID* GetSessionByID(DBAccountContext* context, unsigned long long id)
{
	for (size_t i = 0; i < context->SessionCount; i++)
	{
		if (context->ActiveSessions[i].AccountID == id)
		{
			return context->ActiveSessions + i;
		}
	}

	return NULL;;
}

static SessionID* CreateSession(DBAccountContext* context, UserAccount* account)
{
	SessionID* Session = GetSessionByID(context, account->ID);
	if (Session)
	{
		Session->SessionStartTime = time(NULL);
		return Session;
	}

	SessionIDListEnsureCapacity(context, context->SessionCount + 1);

	Session = &context->ActiveSessions[context->SessionCount];
	Session->AccountID = account->ID;
	GenerateSessionID(Session->IDValues, account->Email);
	Session->SessionStartTime = time(NULL);
	context->SessionCount += 1;
	return Session;
}

static void RemoveSessionByIndex(DBAccountContext* context, size_t index)
{
	if (context->SessionCount == 0)
	{
		return;
	}

	for (size_t i = index + 1; i < context->SessionCount; i++)
	{
		context->ActiveSessions[i - 1] = context->ActiveSessions[i];
	}
	context->SessionCount -= 1;
}

static void RemoveSessionByID(DBAccountContext* context, unsigned long long id)
{
	for (size_t i = 0; i < context->SessionCount; i++)
	{
		if (context->ActiveSessions[i].AccountID == id)
		{
			RemoveSessionByIndex(context, i);
			return;
		}
	}
}

static void RefreshSessions(DBAccountContext* context)
{
	time_t CurrentTime = time(NULL);

	for (size_t i = 0; i < context->SessionCount; i++)
	{
		if (CurrentTime - context->ActiveSessions[i].SessionStartTime > MAX_SESSION_TIME)
		{
			// Again, removing elements while iterating but should work out.
			RemoveSessionByIndex(context, i);
			i--;
		}
	}
}


/* Account loading and saving. */
static Error ReadAccountFromCompoundFailCleanup(UserAccount* account, Error errorToReturn)
{
	AccountDeconstruct(account);
	return errorToReturn;
}

static Error ReadAccountFromCompound(UserAccount* account, unsigned long long accountID, GHDFCompound* compound)
{
	GHDFEntry* Entry;
	AccountSetDefaultValues(account);

	// ID.
	Error ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_ACCOUNT_ID, &Entry, GHDFType_ULong, "Account ID");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReadAccountFromCompoundFailCleanup(account, ReturnedError);
	}
	if (Entry->Value.SingleValue.ULong != accountID)
	{
		char Message[128];
		snprintf(Message, sizeof(Message), "Stored and provided ID mismatch (Stored: %llu, provided: %llu)",
			Entry->Value.SingleValue.ULong, accountID);
		return ReadAccountFromCompoundFailCleanup(account, Error_CreateError(ErrorCode_DatabaseError, Message));
	}
	account->ID = accountID;

	// Name.
	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_ACCOUNT_NAME, &Entry, GHDFType_String, "Account Name");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReadAccountFromCompoundFailCleanup(account, ReturnedError);
	}
	account->Name = String_CreateCopy(Entry->Value.SingleValue.String);

	// Surname
	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_ACCOUNT_SURNAME, &Entry, GHDFType_String, "Account Surname");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReadAccountFromCompoundFailCleanup(account, ReturnedError);
	}
	account->Surname = String_CreateCopy(Entry->Value.SingleValue.String);

	// Email.
	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_ACCOUNT_EMAIL, &Entry, GHDFType_String, "Account Email");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReadAccountFromCompoundFailCleanup(account, ReturnedError);
	}
	account->Email = String_CreateCopy(Entry->Value.SingleValue.String);

	// Password hash
	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_ACCOUNT_PASSWORD, &Entry,
		GHDFType_ULong | GHDF_TYPE_ARRAY_BIT, "Account Password Hash");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReadAccountFromCompoundFailCleanup(account, ReturnedError);
	}
	if (Entry->Value.ValueArray.Size != PASSWORD_HASH_LENGTH)
	{
		return ReadAccountFromCompoundFailCleanup(account, ReturnedError);
	}
	for (int i = 0; i < PASSWORD_HASH_LENGTH; i++)
	{
		account->PasswordHash[i] = Entry->Value.ValueArray.Array[i].Long;
	}


	// Posts.
	ReturnedError = GHDFCompound_GetVerifiedOptionalEntry(compound, ENTRY_ID_ACCOUNT_POSTS, &Entry,
		GHDFType_ULong | GHDF_TYPE_ARRAY_BIT, "Account Post Array");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReadAccountFromCompoundFailCleanup(account, ReturnedError);
	}
	if (Entry)
	{
		unsigned long long* PostIDs = (unsigned long long*)Memory_SafeMalloc(sizeof(unsigned long long) * Entry->Value.ValueArray.Size);
		for (unsigned int i = 0; i < Entry->Value.ValueArray.Size; i++)
		{
			PostIDs[i] = Entry->Value.ValueArray.Array[i].ULong;
		}
		account->Posts = PostIDs;
		account->PostCount = Entry->Value.ValueArray.Size;
	}

	// Creation time.
	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_ACCOUNT_CREATION_TIME, &Entry, GHDFType_Long, "Account Creation Time");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReadAccountFromCompoundFailCleanup(account, ReturnedError);
	}
	account->CreationTime = Entry->Value.SingleValue.Long;

	// IsAdmin field.
	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_ACCOUNT_IS_ADMIN, &Entry, GHDFType_Bool, "Account IsAdmin");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReadAccountFromCompoundFailCleanup(account, ReturnedError);
	}
	account->IsAdmin = Entry->Value.SingleValue.Bool;

	return Error_CreateSuccess();
}

static Error ReadAccountImageFromDatabase(DBAccountContext* context, UserAccount* account)
{
	const char* FilePath = GetPathToIDFile(context, account->ID, IMAGE_ENTRIES_DIR_NAME, FILE_EXTENSION_PNG);
	if (!File_Exists(FilePath))
	{
		Memory_Free((char*)FilePath);
		FilePath = GetPathToIDFile(context, 0, IMAGE_ENTRIES_DIR_NAME, FILE_EXTENSION_PNG);
	}

	Error ReturnedError;
	FILE* File = File_Open(FilePath, FileOpenMode_ReadBinary, &ReturnedError);
	Memory_Free((char*)FilePath);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	size_t DataLength;
	account->ProfileImageData = File_ReadAllData(File, &DataLength, &ReturnedError);
	File_Close(File);
	return ReturnedError;
}

static bool ReadAccountFromDatabase(DBAccountContext* context, UserAccount* account, unsigned long long id, Error* error)
{
	*error = Error_CreateSuccess();
	const char* FilePath = GetPathToIDFile(context, id, ACCOUNT_ENTRIES_DIR_NAME, GHDF_FILE_EXTENSION);
	if (!File_Exists(FilePath))
	{
		Memory_Free((char*)FilePath);
		return false;
	}

	GHDFCompound Compound;
	Error ReturnedError = GHDFCompound_ReadFromFile(FilePath, &Compound);
	Memory_Free((char*)FilePath);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		*error = ReturnedError;
		return false;
	}

	ReturnedError = ReadAccountFromCompound(account, id, &Compound);
	GHDFCompound_Deconstruct(&Compound);
	if (ReturnedError.Code != ErrorCode_Success) 
	{
		*error = ReturnedError;
		return false;
	}
	
	ReturnedError = ReadAccountImageFromDatabase(context, account);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		*error = ReturnedError;
		return false;
	}

	*error = Error_CreateSuccess();
	return true;
}

static Error WriteImageToDatabase(DBAccountContext* context, Image* image, unsigned long long id)
{
	const char* FileName = GetPathToIDFile(context, id, IMAGE_ENTRIES_DIR_NAME, FILE_EXTENSION_PNG);
	int Result = stbi_write_png(FileName, image->SizeX, image->SizeY, STBI_rgb, image->Data, 0);
	Memory_Free((char*)FileName);

	if (!Result)
	{
		return Error_CreateError(ErrorCode_IO, "Failed to write uploaded image to database.");
	}
	return Error_CreateSuccess();
}

static Error WriteAccountToDatabase(DBAccountContext* context, UserAccount* account)
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

	const char* AccountPath = GetPathToIDFile(context, account->ID, ACCOUNT_ENTRIES_DIR_NAME, GHDF_FILE_EXTENSION);
	Error ReturnedError = GHDFCompound_WriteToFile(AccountPath, &Compound);
	Memory_Free((char*)AccountPath);
	GHDFCompound_Deconstruct(&Compound);
	return ReturnedError;
}

static void DeleteAccountFromDatabase(DBAccountContext* context, unsigned long long id)
{
	const char* Path = GetPathToIDFile(context, id, ACCOUNT_ENTRIES_DIR_NAME, GHDF_FILE_EXTENSION);
	File_Delete(Path);
	Memory_Free((char*)Path);
}


/* Hashmap. */
static void GenerateMetaInfoForSingleAccount(DBAccountContext* context, UserAccount* account)
{
	IDCodepointHashMap_AddID(&context->NameMap, account->Name, account->ID);
	IDCodepointHashMap_AddID(&context->NameMap, account->Surname, account->ID);
	IDCodepointHashMap_AddID(&context->EmailMap, account->Email, account->ID);
}

static Error GenerateMetaInfoForAccounts(DBAccountContext* context, size_t* readAccountCount)
{
	unsigned long long MaxIDExclusive = context->AvailableAccountID;
	size_t ReadAccounts = 0;
	*readAccountCount = 0;
	for (unsigned long long ID = DEFAULT_AVAILABLE_ACCOUNT_ID; ID < MaxIDExclusive; ID++)
	{
		UserAccount Account;
		Error ReturnedError;
		if (!ReadAccountFromDatabase(context, &Account, ID, &ReturnedError))
		{
			if (ReturnedError.Code != ErrorCode_Success)
			{
				return ReturnedError;
			}
			continue;
		}

		ReadAccounts++;
		GenerateMetaInfoForSingleAccount(context, &Account);

		AccountDeconstruct(&Account);
	}

	*readAccountCount = ReadAccounts;
	return Error_CreateSuccess();
}

static void ClearMetaInfoForAccount(DBAccountContext* context, UserAccount* account)
{
	IDCodepointHashMap_RemoveID(&context->NameMap, account->Name, account->ID);
	IDCodepointHashMap_RemoveID(&context->NameMap, account->Surname, account->ID);
	IDCodepointHashMap_RemoveID(&context->EmailMap, account->Email, account->ID);
}


/* Account cache. */
static void InitializeAccountCache(DBAccountContext* context)
{
	context->AccountCache = (CachedAccount*)Memory_SafeMalloc(sizeof(CachedAccount) * ACCOUNT_CACHE_CAPACITY);
	for (int i = 0; i < ACCOUNT_CACHE_CAPACITY; i++)
	{
		context->AccountCache[i].LastAccessTime = ACCOUNT_CACHE_UNLOADED_TIME;
	}
}

static UserAccount* TryGetAccountFromCache(DBAccountContext* context, unsigned long long id)
{
	for (int i = 0; i < ACCOUNT_CACHE_CAPACITY; i++)
	{
		CachedAccount* TargetCachedAccount = (context->AccountCache + i);
		if ((TargetCachedAccount->LastAccessTime != ACCOUNT_CACHE_UNLOADED_TIME) && (TargetCachedAccount->Account.ID == id))
		{
			TargetCachedAccount->LastAccessTime = time(NULL);
			return &TargetCachedAccount->Account;
		}
	}

	return NULL;
}

static Error ClearCacheSpotByIndex(DBAccountContext* context, size_t index, bool saveAccount)
{
	CachedAccount* AccountCached = context->AccountCache + index;
	if (AccountCached->LastAccessTime != ACCOUNT_CACHE_UNLOADED_TIME)
	{
		if (saveAccount)
		{
			Error ReturnedError = WriteAccountToDatabase(context, &AccountCached->Account);
			if (ReturnedError.Code != ErrorCode_Success)
			{
				return ReturnedError;
			}
		}
		AccountDeconstruct(&AccountCached->Account);
		AccountCached->LastAccessTime = ACCOUNT_CACHE_UNLOADED_TIME;
	}
	
	return Error_CreateSuccess();
}

static CachedAccount* GetCacheSpotForAccount(DBAccountContext* context, Error* error)
{
	time_t LowestLoadTime = LLONG_MAX;
	int LowestLoadTimeIndex = 0;

	for (int i = 0; i < ACCOUNT_CACHE_CAPACITY; i++)
	{
		if (context->AccountCache[i].LastAccessTime < LowestLoadTime)
		{
			LowestLoadTimeIndex = i;
			LowestLoadTime = context->AccountCache[i].LastAccessTime;
		}
	}

	*error = ClearCacheSpotByIndex(context, LowestLoadTimeIndex, true);
	return context->AccountCache + LowestLoadTimeIndex;
}

static CachedAccount* LoadAccountIntoCache(DBAccountContext* context, unsigned long long id, Error* error)
{
	CachedAccount* AccountSpot = GetCacheSpotForAccount(context, error);
	if (error->Code != ErrorCode_Success)
	{
		return NULL;
	}

	if (!ReadAccountFromDatabase(context , &AccountSpot->Account, id, error))
	{
		AccountSpot->LastAccessTime = ACCOUNT_CACHE_UNLOADED_TIME;
		return NULL;
	}

	AccountSpot->LastAccessTime = time(NULL);
	*error = Error_CreateSuccess();
	return AccountSpot;
}

static Error SaveAllCachedAccountsToDatabase(DBAccountContext* context)
{
	for (int i = 0; i < ACCOUNT_CACHE_CAPACITY; i++)
	{
		if (context->AccountCache[i].LastAccessTime == ACCOUNT_CACHE_UNLOADED_TIME)
		{
			continue;
		}

		Error ReturnedError = WriteAccountToDatabase(context, &context->AccountCache[i].Account);
		if (ReturnedError.Code != ErrorCode_Success)
		{
			return ReturnedError;
		}
	}

	return Error_CreateSuccess();
}

static Error RemoveAccountFromCacheByID(DBAccountContext* context, unsigned long long id, bool saveAccount)
{
	for (int i = 0; i < ACCOUNT_CACHE_CAPACITY; i++)
	{
		if ((context->AccountCache[i].Account.ID == id))
		{
			return ClearCacheSpotByIndex(context, i, saveAccount);
		}
	}

	return Error_CreateSuccess();
}


/* Searching database. */
static bool IsEmailInDatabase(DBAccountContext* context, const char* email, Error* error)
{
	*error = Error_CreateSuccess();
	size_t IDArraySize;
	unsigned long long* IDs = IDCodepointHashMap_FindByString(&context->EmailMap, email, true, &IDArraySize);
	if (IDArraySize == 0)
	{
		return false;
	}

	bool FoundEmail = false;
	for (size_t i = 0; i < IDArraySize; i++)
	{
		UserAccount* Account = AccountManager_GetAccountByID(context, IDs[i], error);

		if (error->Code != ErrorCode_Success)
		{
			break;
		}

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

static UserAccount** SearchAccountFromIDsByName(DBAccountContext* context,
	unsigned long long* ids,
	size_t idCount,
	const char* name,
	size_t* accountCount,
	Error* error)
{
	UserAccount** AccountArray = (UserAccount**)Memory_SafeMalloc(sizeof(UserAccount*) * idCount);
	size_t MatchedAccountCount = 0;
	for (size_t i = 0; i < idCount; i++)
	{
		UserAccount* Account = AccountManager_GetAccountByID(context, ids[i], error);

		if (error->Code != ErrorCode_Success)
		{
			Memory_Free(AccountArray);
			return NULL;
		}

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

static size_t FindNextAvailableAccountID(DBAccountContext* context)
{
	size_t SkippedAccounts = 0;
	bool FileExists;
	do
	{
		const char* FilePath = GetPathToIDFile(context, GetAccountID(context),
			ACCOUNT_ENTRIES_DIR_NAME, GHDF_FILE_EXTENSION);
		FileExists = File_Exists(FilePath);
		Memory_Free((char*)FilePath);

		if (FileExists)
		{
			context->AvailableAccountID++;
			SkippedAccounts++;
		}

	} while (FileExists);
	return SkippedAccounts;
}


/* Constructor helper functions. */
static void LoadDefaultMetaInfo(DBAccountContext* context)
{
	context->AvailableAccountID = DEFAULT_AVAILABLE_ACCOUNT_ID;
	context->SessionCount = 0;
}

static Error AddSessionFromCompound(DBAccountContext* context, GHDFCompound* compound)
{
	SessionIDListEnsureCapacity(context, context->SessionCount + 1);
	SessionID* Session = &context->ActiveSessions[context->SessionCount];
	GHDFEntry* Entry;

	// Time.
	Error ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_METAINFO_SESSION_START_TIME, &Entry,
		GHDFType_Long, "Session start time");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	Session->SessionStartTime = (time_t)Entry->Value.SingleValue.Long;


	// Account ID.
	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_METAINFO_SESSION_ACCOUNT_ID, &Entry,
		GHDFType_ULong, "Session account ID");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	Session->AccountID = Entry->Value.SingleValue.ULong;

	// Session values.
	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_METAINFO_SESSION_IDVALUES, &Entry,
		GHDFType_UInt | GHDF_TYPE_ARRAY_BIT, "Session ID values.");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	if (Entry->Value.ValueArray.Size != SESSION_ID_LENGTH)
	{
		return Error_CreateError(ErrorCode_DatabaseError, "Session ID values length for compound is invalid.");
	}
	for (int i = 0; i < SESSION_ID_LENGTH; i++)
	{
		Session->IDValues[i] = Entry->Value.ValueArray.Array[i].UInt;
	}

	context->SessionCount += 1;
	return Error_CreateSuccess();
}

static Error ReadMetaInfoFromCompound(DBAccountContext* context, GHDFCompound* compound)
{
	GHDFEntry* Entry;

	// Account ID.
	Error ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_METAINFO_AVAILABLE_ID, &Entry, GHDFType_ULong,
		"Meta-info account id.");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	context->AvailableAccountID = Entry->Value.SingleValue.ULong;


	// Sessions.
	ReturnedError = GHDFCompound_GetVerifiedOptionalEntry(compound,
		ENTRY_ID_METAINFO_SESSION_ARRAY, &Entry, GHDFType_Compound | GHDF_TYPE_ARRAY_BIT,
		"Meta-info account sessions.");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	if (!Entry)
	{
		return Error_CreateSuccess();
	}

	for (unsigned int i = 0; i < Entry->Value.ValueArray.Size; i++)
	{
		ReturnedError = AddSessionFromCompound(context, Entry->Value.ValueArray.Array[i].Compound);
		if (ReturnedError.Code != ErrorCode_Success)
		{
			return ReturnedError;
		}
	}

	return Error_CreateSuccess();
}

static Error LoadMetaInfo(DBAccountContext* context)
{
	LoadDefaultMetaInfo(context);

	GHDFCompound Compound;
	const char* FilePath = Directory_CombinePaths(context->AccountRootPath, ACCOUNT_METAINFO_FILE_NAME);
	if (!File_Exists(FilePath))
	{
		Memory_Free((char*)FilePath);
		return Error_CreateSuccess();
	}

	Error ReturnedError = GHDFCompound_ReadFromFile(FilePath, &Compound);
	Memory_Free((char*)FilePath);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	ReturnedError = ReadMetaInfoFromCompound(context, &Compound);
	GHDFCompound_Deconstruct(&Compound);
	return ReturnedError;
}

static void SaveSessionsToCompound(DBAccountContext* context, GHDFCompound* compound)
{
	GHDFPrimitive* PrimitiveArray = (GHDFPrimitive*)Memory_SafeMalloc(sizeof(GHDFPrimitive) * context->SessionCount);

	for (size_t CompIndex = 0; CompIndex < context->SessionCount; CompIndex++)
	{
		GHDFCompound* TargetCompound = (GHDFCompound*)Memory_SafeMalloc(sizeof(GHDFCompound));
		PrimitiveArray[CompIndex].Compound = TargetCompound;
		GHDFCompound_Construct(TargetCompound, COMPOUND_DEFAULT_CAPACITY);
		GHDFPrimitive SingleValue;

		SingleValue.Long = (long long)context->ActiveSessions[CompIndex].SessionStartTime;
		GHDFCompound_AddSingleValueEntry(TargetCompound, GHDFType_Long, ENTRY_ID_METAINFO_SESSION_START_TIME, SingleValue);
		SingleValue.ULong = (unsigned long long)context->ActiveSessions[CompIndex].AccountID;
		GHDFCompound_AddSingleValueEntry(TargetCompound, GHDFType_ULong, ENTRY_ID_METAINFO_SESSION_ACCOUNT_ID, SingleValue);

		GHDFPrimitive* SessionIDValues = (GHDFPrimitive*)Memory_SafeMalloc(sizeof(GHDFPrimitive) * SESSION_ID_LENGTH);
		for (int IDIndex = 0; IDIndex < SESSION_ID_LENGTH; IDIndex++)
		{
			SessionIDValues[IDIndex].UInt = context->ActiveSessions[CompIndex].IDValues[IDIndex];
		}
		GHDFCompound_AddArrayEntry(TargetCompound, GHDFType_UInt, ENTRY_ID_METAINFO_SESSION_IDVALUES, SessionIDValues, SESSION_ID_LENGTH);
	}

	GHDFCompound_AddArrayEntry(compound, GHDFType_Compound, ENTRY_ID_METAINFO_SESSION_ARRAY,
		PrimitiveArray, (unsigned int)context->SessionCount);
}

static Error SaveMetaInfo(DBAccountContext* context)
{
	GHDFCompound Compound;
	GHDFCompound_Construct(&Compound, COMPOUND_DEFAULT_CAPACITY);
	GHDFPrimitive SingleValue;

	SingleValue.ULong = context->AvailableAccountID;
	GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_ULong, ENTRY_ID_METAINFO_AVAILABLE_ID, SingleValue);
	SingleValue.ULong = context->AvailableAccountID;
	GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_ULong, ENTRY_ID_METAINFO_SESSION_ACCOUNT_ID, SingleValue);

	RefreshSessions(context);
	if (context->SessionCount > 0)
	{
		SaveSessionsToCompound(context, &Compound);
	}

	const char* FilePath = Directory_CombinePaths(context->AccountRootPath, ACCOUNT_METAINFO_FILE_NAME);
	Error ReturnedError = GHDFCompound_WriteToFile(FilePath, &Compound);
	Memory_Free((char*)FilePath);
	GHDFCompound_Deconstruct(&Compound);

	return ReturnedError;
}


// Functions.
Error AccountManager_Construct(ServerContext* serverContext)
{
	serverContext->AccountContext->AccountRootPath = Directory_CombinePaths(serverContext->Resources->DatabaseRootPath, DIR_NAME_ACCOUNTS);

	serverContext->AccountContext->_sessionListCapacity = GENERIC_LIST_CAPACITY;
	serverContext->AccountContext->ActiveSessions = 
		(SessionID*)Memory_SafeMalloc(sizeof(SessionID) * serverContext->AccountContext->_sessionListCapacity);
	serverContext->AccountContext->SessionCount = 0;

	Error ReturnedError = LoadMetaInfo(serverContext->AccountContext);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	IDCodepointHashMap_Construct(&serverContext->AccountContext->NameMap);
	IDCodepointHashMap_Construct(&serverContext->AccountContext->EmailMap);

	serverContext->AccountContext->UnverifiedAccounts = 
		(UnverifiedUserAccount*)Memory_SafeMalloc(sizeof(UnverifiedUserAccount) * GENERIC_LIST_CAPACITY);
	serverContext->AccountContext->UnverifiedAccountCount = 0;
	serverContext->AccountContext->_unverifiedAccountCapacity = GENERIC_LIST_CAPACITY;

	size_t ReadAccountCount;
	ReturnedError = GenerateMetaInfoForAccounts(serverContext->AccountContext, &ReadAccountCount);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	char Message[128];
	snprintf(Message, sizeof(Message), "Read %llu accounts while creating ID hashes.", ReadAccountCount);
	Logger_LogInfo(serverContext->Logger, Message);

	InitializeAccountCache(serverContext->AccountContext);

	return ReturnedError;
}

Error AccountManager_Deconstruct(DBAccountContext* context)
{
	Error ReturnedError = SaveMetaInfo(context);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	ReturnedError = SaveAllCachedAccountsToDatabase(context);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	IDCodepointHashMap_Deconstruct(&context->NameMap);
	IDCodepointHashMap_Deconstruct(&context->EmailMap);
	Memory_Free(context->ActiveSessions);
	Memory_Free(context->UnverifiedAccounts);
	Memory_Free(context->AccountCache);

	return Error_CreateSuccess();
}


/* Accounts. */
UserAccount* AccountManager_TryCreateAccount(ServerContext* context,
	const char* name,
	const char* surname,
	const char* email,
	const char* password,
	Error* error)
{
	*error = Error_CreateSuccess();
	if (!VerifyName(name) || !VerifyName(surname) || !VerifyPassword(password)
		|| !VerifyEmail(email, context->Configuration->AcceptedDomains, context->Configuration->AcceptedDomainCount)
		|| IsEmailInDatabase(context->AccountContext, email, error))
	{
		return NULL;
	}

	const char* AccountFilePath = GetPathToIDFile(
		context->AccountContext, GetAccountID(context->AccountContext), ACCOUNT_ENTRIES_DIR_NAME, GHDF_FILE_EXTENSION);
	if (File_Exists(AccountFilePath))
	{
		Memory_Free((char*)AccountFilePath);
		char Message[256];
		snprintf(Message, sizeof(Message), "Account with ID %llu already exists in database (file match), but no email entry was found and "
			"an attempt to creating this account was made. Corrupted database?", GetAccountID(context->AccountContext));
		*error = Error_CreateError(ErrorCode_DatabaseError, Message);

		size_t SkippedAccountCount = FindNextAvailableAccountID(context->AccountContext);
		snprintf(Message, sizeof(Message), "Account ID mismatch with stored IDs, skipped %llu account IDs.", SkippedAccountCount);
		Logger_LogError(context->Logger, Message);
		return NULL;
	}
	Memory_Free((char*)AccountFilePath);

	UserAccount Account;
	AccountSetDefaultValues(&Account);
	*error = AccountCreateNew(context->AccountContext, &Account, name, surname, email, password);
	if (error->Code != ErrorCode_Success)
	{
		return NULL;
	}

	*error = WriteAccountToDatabase(context->AccountContext, &Account);
	if (error->Code != ErrorCode_Success)
	{
		AccountDeconstruct(&Account);
		return NULL;
	}

	GenerateMetaInfoForSingleAccount(context->AccountContext, &Account);
	unsigned long long ID = Account.ID;
	AccountDeconstruct(&Account);
	return &LoadAccountIntoCache(context->AccountContext, ID, error)->Account;
}

bool AccountManager_TryCreateUnverifiedAccount(ServerContext* context,
	const char* name,
	const char* surname,
	const char* email,
	const char* password,
	Error* error)
{
	*error = Error_CreateSuccess();
	if (!VerifyName(name) || !VerifyName(surname) || !VerifyPassword(password)
		|| !VerifyEmail(email, context->Configuration->AcceptedDomains, context->Configuration->AcceptedDomainCount)
		|| IsEmailInDatabase(context->AccountContext, email, error))
	{
		return false;
	}

	AddUnverifiedAccount(context->AccountContext, name, surname, email, password);
	return true;
}

bool AccountManager_TryVerifyAccount(ServerContext* serverContext, const char* email, int code, Error* error)
{
	RefreshUnverifiedAccounts(serverContext->AccountContext);
	*error = Error_CreateSuccess();

	UnverifiedUserAccount* UnverifiedAccount = GetUnverifiedAccountByEmail(serverContext->AccountContext, email);
	if (!UnverifiedAccount)
	{
		return false;
	}

	if (!UnverifiedAccountTryVerify(UnverifiedAccount, code))
	{
		return false;
	}

	if (!AccountManager_TryCreateAccount(serverContext, UnverifiedAccount->Name, UnverifiedAccount->Surname,
		UnverifiedAccount->Email, UnverifiedAccount->Password, error))
	{
		return false;
	}

	RemoveUnverifiedAccountByEmail(serverContext->AccountContext, email);
	return true;
}

Error AccountManager_VerifyAccount(ServerContext* serverContext, const char* email)
{
	RefreshUnverifiedAccounts(serverContext->AccountContext);

	UnverifiedUserAccount* UnverifiedAccount = GetUnverifiedAccountByEmail(serverContext->AccountContext, email);
	if (!UnverifiedAccount)
	{
		return Error_CreateSuccess();
	}

	Error ReturnedError;
	AccountManager_TryCreateAccount(serverContext, UnverifiedAccount->Name, UnverifiedAccount->Surname,
		UnverifiedAccount->Surname, UnverifiedAccount->Password, &ReturnedError);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	RemoveUnverifiedAccountByEmail(serverContext->AccountContext, email);
	return Error_CreateSuccess();
}

UserAccount* AccountManager_GetAccountByID(DBAccountContext* context, unsigned long long id, Error* error)
{
	*error = Error_CreateSuccess();
	UserAccount* Account = TryGetAccountFromCache(context, id);

	if (Account)
	{
		return Account;
	}

	CachedAccount* AccountCached = LoadAccountIntoCache(context, id, error);
	return AccountCached ? &AccountCached->Account : NULL;
}

UserAccount** AccountManager_GetAccountsByName(DBAccountContext* context, const char* name, size_t* accountCount, Error* error)
{
	size_t IDCount;
	unsigned long long* IDs = IDCodepointHashMap_FindByString(&context->NameMap, name, true, &IDCount);
	if (IDCount == 0)
	{
		*error = Error_CreateSuccess();
		return NULL;
	}

	UserAccount** FoundAccounts = SearchAccountFromIDsByName(context, IDs, IDCount, name, accountCount, error);
	Memory_Free(IDs);

	return FoundAccounts;
}

UserAccount* AccountManager_GetAccountByEmail(DBAccountContext* context, const char* email, Error* error)
{
	size_t IDCount;
	*error = Error_CreateSuccess();

	unsigned long long* IDs = IDCodepointHashMap_FindByString(&context->EmailMap, email, true, &IDCount);
	if (IDCount == 0)
	{
		return NULL;
	}

	UserAccount* FoundAccount = NULL;
	for (size_t i = 0; i < IDCount; i++)
	{
		UserAccount* Account = AccountManager_GetAccountByID(context, IDs[i], error);

		if (String_EqualsCaseInsensitive(Account->Email, email))
		{
			FoundAccount = Account;
			break;
		}
	}

	Memory_Free(IDs);
	return FoundAccount;
}

UserAccount* AccountManager_GetAccountBySession(DBAccountContext* context, unsigned int* sessionValues, Error* error)
{
	RefreshSessions(context);

	for (size_t i = 0; i < context->SessionCount; i++)
	{
		if (IsSessionIDValuesEqual(sessionValues, context->ActiveSessions[i].IDValues))
		{
			return AccountManager_GetAccountByID(context, context->ActiveSessions[i].AccountID, error);
		}
	}
	
	*error = Error_CreateSuccess();
	return NULL;
}

Error AccountManager_DeleteAccount(ServerContext* serverContext, UserAccount* account)
{
	char Message[256];
	snprintf(Message, sizeof(Message), "Deleting account with ID %llu and email %s", account->ID, account->Email);
	Logger_LogInfo(serverContext->Logger, Message);

	RemoveSessionByID(serverContext->AccountContext, account->ID);
	ClearMetaInfoForAccount(serverContext->AccountContext, account);
	Error ReturnedError = RemoveAccountFromCacheByID(serverContext->AccountContext, account->ID, false);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	DeleteAccountFromDatabase(serverContext->AccountContext, account->ID);
	AccountDeconstruct(account);
	return Error_CreateSuccess();
}

Error AccountManager_DeleteAllAccounts(ServerContext* serverContext)
{
	Logger_LogWarning(serverContext->Logger, "DELETING ALL ACCOUNTS");

	for (unsigned long long i = 0; i < GetAccountID(serverContext->AccountContext); i++)
	{
		Error ReturnedError;
		UserAccount* Account = AccountManager_GetAccountByID(serverContext->AccountContext, i, &ReturnedError);
		if (ReturnedError.Code != ErrorCode_Success)
		{
			return ReturnedError;
		}

		if (!Account)
		{
			continue;
		}

		AccountManager_DeleteAccount(serverContext, Account);
	}

	return Error_CreateSuccess();
}

bool AccountManager_IsPasswordCorrect(UserAccount* account, const char* password)
{
	unsigned long long Hash[16];
	GeneratePasswordHash(Hash, password);
	return PasswordHashEquals(account->PasswordHash, Hash);
}

bool AccountManager_SetName(UserAccount* account, const char* name)
{
	if (!VerifyName(name))
	{
		return false;
	}
	Memory_Free((char*)account->Name);
	account->Name = String_CreateCopy(name);

	return true;
}

bool AccountManager_SetSurname(UserAccount* account, const char* surname)
{
	if (!VerifyName(surname))
	{
		return false;
	}
	Memory_Free((char*)account->Surname);
	account->Surname = String_CreateCopy(surname);

	return true;
}


/* Sessions. */
bool AccountManager_IsSessionAdmin(DBAccountContext* context, unsigned int* sessionIdValues, Error* error)
{
	for (size_t i = 0; i < context->SessionCount; i++)
	{
		if (IsSessionIDValuesEqual(context->ActiveSessions[i].IDValues, sessionIdValues))
		{
			return AccountManager_GetAccountByID(context, context->ActiveSessions[i].AccountID, error)->IsAdmin;
		}
	}
	*error = Error_CreateSuccess();
	return false;
}

SessionID* AccountManager_TryCreateSession(DBAccountContext* context, UserAccount* account, const char* password)
{
	unsigned long long PasswordHash[16];
	GeneratePasswordHash(PasswordHash, password);
	if (!PasswordHashEquals(PasswordHash, account->PasswordHash))
	{
		return NULL;
	}

	return CreateSession(context, account);
}

SessionID* AccountManager_CreateSession(DBAccountContext* context, UserAccount* account)
{
	return CreateSession(context, account);
}


/* Profile image. */
Error AccountManager_GetProfileImage(DBAccountContext* context, unsigned long long id, Image* image)
{
	const char* FilePath = GetPathToIDFile(context, id, IMAGE_ENTRIES_DIR_NAME, FILE_EXTENSION_PNG);
	image->Data = stbi_load(FilePath, &image->SizeX, &image->SizeY, &image->ColorChannels, STBI_rgb);
	if (!image->Data)
	{
		return Error_CreateError(ErrorCode_IO, "Failed to load account image");
	}
	return Error_CreateSuccess();
}

Error AccountManager_SetProfileImage(DBAccountContext* context, UserAccount* account, unsigned char* uploadedImageData, size_t dataLength)
{
	Image UploadedImage;
	UploadedImage.Data = stbi_load_from_memory(uploadedImageData, (int)dataLength, &UploadedImage.SizeX,
		&UploadedImage.SizeY, &UploadedImage.ColorChannels, STBI_rgb);
	if (!UploadedImage.Data)
	{
		return Error_CreateError(ErrorCode_InvalidArgument, "Provided image's data to set for profile is invalid and couldn't be loaded.");
	}

	Image_ScaleImageToFit(&UploadedImage, PROFILE_IMAGE_MAX_SIZE);

	Error ReturnedError = WriteImageToDatabase(context, &UploadedImage, account->ID);
	Memory_Free((char*)UploadedImage.Data);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	Memory_Free((char*)account->ProfileImageData);
	return ReadAccountImageFromDatabase(context, account);
}

void AccountManager_DeleteAllProfileImages(ServerContext* serverContext)
{
	Logger_LogWarning(serverContext->Logger, "DELETING ALL PROFILE IMAGES.");

	for (unsigned long long id = 0; id < serverContext->AccountContext->AvailableAccountID; id++)
	{
		const char* FilePath = GetPathToIDFile(serverContext->AccountContext,
			id, IMAGE_ENTRIES_DIR_NAME, FILE_EXTENSION_PNG);
		File_Delete(FilePath);
		Memory_Free((char*)FilePath);
	}
}