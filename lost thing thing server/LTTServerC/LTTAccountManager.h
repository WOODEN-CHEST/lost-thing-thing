#pragma once
#include "LttErrors.h"
#include <stdbool.h>
#include "LTTServerC.h"
#include <time.h>
#include "IDCodepointHashMap.h"
#include "Image.h"


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

	const char* ProfileImageData;

	unsigned long long* Posts;
	unsigned int PostCount;

	bool IsAdmin;
} UserAccount;

typedef struct SessionIDStruct
{
	time_t SessionStartTime;
	unsigned long long AccountID;
	unsigned int IDValues[SESSION_ID_LENGTH];
} SessionID;


typedef struct DBAccountContextStruct
{
	const char* AccountRootPath;

	unsigned long long AvailableAccountID;
	IDCodepointHashMap NameMap;
	IDCodepointHashMap EmailMap;

	SessionID* ActiveSessions;
	size_t SessionCount;
	size_t _sessionListCapacity;

	struct UnverifiedUserAccountStruct* UnverifiedAccounts;
	size_t UnverifiedAccountCount;
	size_t _unverifiedAccountCapacity;

	struct CachedAccountStruct* AccountCache;
} DBAccountContext;



// Functions.
Error AccountManager_Construct(ServerContext* serverContext);

Error AccountManager_Deconstruct(DBAccountContext* context);


/* Account/ */
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

Error AccountManager_VerifyAccount(ServerContext* serverContext, const char* email);

bool AccountManager_TryVerifyAccount(ServerContext* serverContext, const char* email, int code, Error* error);

UserAccount* AccountManager_GetAccountByID(DBAccountContext* context, unsigned long long id, Error* error);

UserAccount** AccountManager_GetAccountsByName(DBAccountContext* context, const char* name, size_t* accountCount, Error* error);

UserAccount* AccountManager_GetAccountByEmail(DBAccountContext* context, const char* email, Error* error);

UserAccount* AccountManager_GetAccountBySession(DBAccountContext* context, unsigned int* sessionValues, Error* error);

Error AccountManager_DeleteAccount(ServerContext* serverContext, UserAccount* account);

Error AccountManager_DeleteAllAccounts(ServerContext* serverContext);

bool AccountManager_IsPasswordCorrect(UserAccount* account, const char* password);

bool AccountManager_SetName(UserAccount* account, const char* name);

bool AccountManager_SurnameName(UserAccount* account, const char* surname);


/* Sessions. */
bool AccountManager_IsSessionAdmin(DBAccountContext* context, unsigned int* sessionIdValues, Error* error);

SessionID* AccountManager_TryCreateSession(DBAccountContext* context, UserAccount* account, const char* password);

SessionID* AccountManager_CreateSession(DBAccountContext* context, UserAccount* account);


/* Profile image. */
Error AccountManager_GetProfileImage(DBAccountContext* context, unsigned long long id, Image* image);

Error AccountManager_SetProfileImage(DBAccountContext* context, UserAccount* account, unsigned char* uploadedImageData, size_t dataLength);

void AccountManager_DeleteAllProfileImages(ServerContext* context);