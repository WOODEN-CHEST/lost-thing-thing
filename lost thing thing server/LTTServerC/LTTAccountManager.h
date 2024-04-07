#pragma once
#include "LttErrors.h"
#include <stdbool.h>
#include <time.h>
#include "IDCodepointHashMap.h"


// Macros.
#define SESSION_ID_LENGTH 128
#define MAX_SESSION_TIME 60 * 60 * 24 * 7


// Types.
typedef struct UserAccountStruct
{
	unsigned long long ID;
	const char* Name;
	const char* Surname;
	const char* Email;
	unsigned long long PasswordHash[16];
	long long CreationTime;
	unsigned long long ProfileImageID;
	unsigned long long* Posts;
	unsigned int PostCount;
	_Bool IsAdmin;
} UserAccount;

typedef struct UnverifiedUserAccount
{
	int VerificationCode;
	int VerificationAttempts;
	time_t VerificationStartTime;
	const char* Name;
	const char* Surname;
	const char* Email;
	const char* Password;
} UnverifiedUserAccount;

typedef struct SessionIDStruct
{
	time_t SessionStartTime;
	unsigned long long AccountID;
	unsigned char IDValues[SESSION_ID_LENGTH];
} SessionID;

typedef struct CachedAccountStruct
{
	time_t LastAccessTime;
	UserAccount Account;
} CachedAccount;

typedef struct DBAccountContextStruct
{
	unsigned long long AvailableAccountID;
	unsigned long long AvailableImageID;
	IDCodepointHashMap NameMap;
	IDCodepointHashMap EmailMap;

	SessionID* ActiveSessions;
	size_t SessionCount;
	size_t _sessionListCapacity;

	UnverifiedUserAccount* UnverifiedAccounts;
	size_t UnverifiedAccountCount;
	size_t _unverifiedAccountCapacity;

	CachedAccount* AccountCache;
} DBAccountContext;



// Functions.
ErrorCode AccountManager_ConstructContext(DBAccountContext* context, unsigned long long availableID, unsigned long long availableImageID);

void AccountManager_CloseContext(DBAccountContext* context);

UserAccount* AccountManager_TryCreateAccount(const char* name,
	const char* surname,
	const char* email,
	const char* password);

bool AccountManager_TryCreateUnverifiedAccount(const char* name,
	const char* surname,
	const char* email,
	const char* password);

UserAccount* AccountManager_GetAccountByID(unsigned long long id);

UserAccount** AccountManager_GetAccountByName(const char* name, size_t* accountCount);

ErrorCode AccountManager_VerifyAccount(const char* email);

bool AccountManager_TryVerifyAccount(const char* email, int code);

bool AccountManager_IsSessionAdmin(unsigned char* sessionIdValues);

bool AccountManager_CreateSession(const char* email, const char* password);