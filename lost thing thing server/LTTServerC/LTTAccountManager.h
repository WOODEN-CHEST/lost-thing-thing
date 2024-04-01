#pragma once
#include "LttErrors.h"
#include <stdbool.h>
#include <time.h>


// Macros.
#define SESSION_ID_LENGTH 128

// Types.
typedef struct UserAccountStruct
{
	unsigned long long ID;
	const char* Name;
	const char* Surname;
	const char* Email;
	long long PasswordHash[16];
	long long CreationTime;
	unsigned long long ProfileImageID;
	unsigned long long* Posts;
} UserAccount;

typedef struct SessionIDStruct
{
	time_t SessionStartTime;
	unsigned char IDValues[SESSION_ID_LENGTH];
} SessionID;

typedef struct DBAccountContextStruct
{
	unsigned long long AvailableID;
	IDCodepointHashMap NameMap;
	IDCodepointHashMap EmailMap;
	SessionID* ActiveSessions;
	size_t SessionCount;
	size_t _sessionListCapacity;
} DBAccountContext;



// Functions.
ErrorCode AccountManager_ConstructContext(DBAccountContext* context, unsigned long long availableID);

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