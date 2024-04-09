#pragma once
#include "LttErrors.h"
#include <stdbool.h>
#include "LTTServerC.h"
#include <time.h>
#include "IDCodepointHashMap.h"


// Macros.
#define SESSION_ID_LENGTH 32
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
	unsigned int IDValues[SESSION_ID_LENGTH];
} SessionID;

typedef struct CachedAccountStruct
{
	time_t LastAccessTime;
	UserAccount Account;
} CachedAccount;

typedef struct DBAccountContextStruct
{
	const char* AccountRootPath;

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
Error AccountManager_Construct(DBAccountContext* context, Logger* logger, const char* databaseRootPath);

void AccountManager_Deconstruct(DBAccountContext* context);

UserAccount* AccountManager_TryCreateAccount(ServerContext* context,
	const char* name,
	const char* surname,
	const char* email,
	const char* passwor,
	Error* error);

bool AccountManager_TryCreateUnverifiedAccount(ServerContext* context,
	const char* name,
	const char* surname,
	const char* email,
	const char* password,
	Error* error);

Error AccountManager_VerifyAccount(ServerContext* context, const char* email);

bool AccountManager_TryVerifyAccount(ServerContext* context, const char* email, int code, Error* error);

UserAccount* AccountManager_GetAccountByID(DBAccountContext* context, unsigned long long id, Error* error);

UserAccount** AccountManager_GetAccountsByName(DBAccountContext* context, const char* name, size_t* accountCount, Error* error);

UserAccount* AccountManager_GetAccountByEmail(DBAccountContext* context, const char* email, Error* error);

void AccountManager_DeleteAccount(ServerContext* context, UserAccount* account);

Error AccountManager_DeleteAllAccounts(ServerContext* context);

bool AccountManager_IsSessionAdmin(DBAccountContext* context, unsigned int* sessionIdValues, Error* error);

SessionID* AccountManager_TryCreateSession(DBAccountContext* context, UserAccount* account, const char* password);

SessionID* AccountManager_CreateSession(DBAccountContext* context, UserAccount* account);