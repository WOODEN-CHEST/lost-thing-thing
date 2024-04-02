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
	const char* Name;
	const char* Surname;
	const char* Email;
	const char* Password;
} UnverifiedUserAccount;

typedef struct SessionIDStruct
{
	time_t SessionStartTime;
	unsigned char SessionFlags;
	unsigned char IDValues[SESSION_ID_LENGTH];
} SessionID;

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
} DBAccountContext;



// Functions.
ErrorCode AccountManager_ConstructContext(DBAccountContext* context, unsigned long long availableID, unsigned long long availableImageID);

void AccountManager_CloseContext(DBAccountContext* context);

ErrorCode AccountManager_TryCreateUser(UserAccount* account,
	const char* name,
	const char* surname,
	const char* email,
	const char* password);

ErrorCode AccountManager_TryCreateUnverifiedUser(UserAccount* account,
	const char* name,
	const char* surname,
	const char* email,
	const char* password);

bool AccountManager_IsUserAdmin(SessionID* sessionID);

bool AccountManager_GetAccount(UserAccount* account, unsigned long long id);