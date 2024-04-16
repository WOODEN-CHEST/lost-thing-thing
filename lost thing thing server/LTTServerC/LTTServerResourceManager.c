#include "LTTServerResourceManager.h"
#include "Directory.h"
#include "File.h"
#include "LTTServerC.h"
#include "Memory.h"
#include "LTTHTML.h"
#include <stdlib.h>
#include "GHDF.h"
#include "IDCodepointHashMap.h"
#include "LTTChar.h"
#include "LTTAccountManager.h"
#include "LTTPostManager.h"
#include "Logger.h"
#include "LTTBase64.h"


// Macros.
#define DIR_NAME_DATABASE "data"
#define DIR_NAME_SOURCE "source"
#define DIR_NAME_POSTS "posts"

#define ARGUMENT_ASSIGNMENT_OPERATOR '='
#define ARGUMENT_VALUE_QUOTE '\"'
#define ARGUMENT_ESCAPE_CHAR '\\'

#define MAX_ARGUMENT_COUNT 32

#define TARGET_PATH_SEPARATOR '/'
#define TARGET_PATH_SEPARATOR_STR "/"
#define SESSION_DELIMETER " "

/* Cookie names. */
#define COOKIE_NAME_SESSION "session"




// Lots of hard-coded strings, let's go!
/* JSON. */
#define JSON_QUOTE '"'
#define JSON_VALUE_ASSIGNMENT ':'
#define JSON_OBJECT_OPEN '{'
#define JSON_OBJECT_CLOSE '}'
#define JSON_ARRAY_OPEN '['
#define JSON_ARRAY_CLOSE ']'
#define JSON_DELIMETER ','


// Types.
typedef struct ArgumentStruct
{
	const char* Key;
	const char* Value;
} Argument;

typedef struct ParseArgumentsStruct
{
	Argument Args[MAX_ARGUMENT_COUNT];
	size_t ArgumentCount;
} ParsedArguments;


// Static functions.
/* Arguments. */
static const char* SkipWhitespace(const char* text)
{
	while (Char_IsWhitespace(text) && (*text != '\0'))
	{
		text++;
	}
	return text;
}

static const char* ParseSingleArgument(Argument* argument, const char* bodyAtPosition)
{
	StringBuilder TextBuilder;
	StringBuilder_Construct(&TextBuilder, DEFAULT_STRING_BUILDER_CAPACITY);
	size_t Index;

	// Key.
	for (Index = 0; (bodyAtPosition[Index] != '\0') && (bodyAtPosition[Index] != ARGUMENT_ASSIGNMENT_OPERATOR); Index++)
	{
		StringBuilder_AppendChar(&TextBuilder, bodyAtPosition[Index]);
	}
	argument->Key = (char*)Memory_SafeMalloc(TextBuilder.Length + 1);
	String_CopyTo(TextBuilder.Data, argument->Key);

	// Assignment.
	if (bodyAtPosition[Index] != ARGUMENT_ASSIGNMENT_OPERATOR)
	{
		Memory_Free((char*)argument->Key);
		StringBuilder_Deconstruct(&TextBuilder);
		return NULL;
	}

	// Value.
	Index++;
	if (bodyAtPosition[Index] != ARGUMENT_VALUE_QUOTE)
	{
		Memory_Free((char*)argument->Key);
		StringBuilder_Deconstruct(&TextBuilder);
		return NULL;
	}

	Index++;
	StringBuilder_Clear(&TextBuilder);
	for (bool EscapeQuote = false; (bodyAtPosition[Index] != '\0')
		&& ((bodyAtPosition[Index] != ARGUMENT_VALUE_QUOTE) || EscapeQuote); Index++)
	{
		EscapeQuote = (bodyAtPosition[Index] == ARGUMENT_ESCAPE_CHAR) && !EscapeQuote;
		StringBuilder_AppendChar(&TextBuilder, bodyAtPosition[Index]);
	}
	argument->Value = (char*)Memory_SafeMalloc(TextBuilder.Length + 1);
	String_CopyTo(TextBuilder.Data, argument->Value);

	if (bodyAtPosition[Index] != ARGUMENT_VALUE_QUOTE)
	{
		Memory_Free((char*)argument->Key);
		Memory_Free((char*)argument->Value);
		StringBuilder_Deconstruct(&TextBuilder);
		return NULL;
	}

	StringBuilder_Deconstruct(&TextBuilder);
	return bodyAtPosition + Index + 1;
}

static bool ParseArgumentsFromBody(ParsedArguments* arguments, const char* body)
{
	arguments->ArgumentCount = 0;
	const char* MovedBodyPtr = body;

	for (int i = 0; (i < MAX_ARGUMENT_COUNT) && (*MovedBodyPtr != '\0'); i++, arguments->ArgumentCount++)
	{
		MovedBodyPtr = ParseSingleArgument(arguments->Args + i, MovedBodyPtr);
		if (!MovedBodyPtr)
		{
			return false;
		}

		MovedBodyPtr = SkipWhitespace(MovedBodyPtr);
	}

	return true;
}

static void ArgumentsDeconstruct(ParsedArguments* arguments)
{
	for (size_t i = 0; i < arguments->ArgumentCount; i++)
	{
		Memory_Free((char*)arguments->Args[i].Key);
		Memory_Free((char*)arguments->Args[i].Value);
	}
}

static const char* GetArgumentValueByName(ParsedArguments* arguments, const char* name)
{
	for (size_t i = 0; i < arguments->ArgumentCount; i++)
	{
		if (String_Equals(arguments->Args[i].Key, name))
		{
			return arguments->Args[i].Value;
		}
	}

	return NULL;
}


/* Cookies. */
static const char* GetCookieValueByName(HttpCookie* cookies, size_t cookieCount, const char* name)
{
	for (size_t i = 0; i < cookieCount; i++)
	{
		if (String_Equals(cookies[i].Name, name))
		{
			return cookies[i].Value;
		}
	}

	return NULL;
}

/* Paths. */
static char** GetPathAsParts(char* unformattedPath, size_t* pathCount)
{
	size_t PathLength = String_LengthBytes(unformattedPath);
	char* TrimmedPathCopy = PathLength <= 1 ? String_CreateCopy("") : String_SubString(unformattedPath, 1, PathLength);
	return String_Split(TrimmedPathCopy, TARGET_PATH_SEPARATOR_STR, pathCount);
}

/* JSON. */
static void JSONAppendQuoted(StringBuilder* builder, const char* string)
{
	StringBuilder_AppendChar(builder, JSON_QUOTE);
	StringBuilder_Append(builder, string);
	StringBuilder_AppendChar(builder, JSON_QUOTE);
}

static void JSONAppendString(StringBuilder* builder, const char* key, const char* value)
{
	JSONAppendQuoted(builder, key);
	StringBuilder_AppendChar(builder, JSON_VALUE_ASSIGNMENT);
	JSONAppendQuoted(builder, value);
}

static void JSONAppendInt(StringBuilder* builder, const char* key, long long integer)
{
	char Number[32];
	snprintf(Number, sizeof(Number), "ll", integer);
	JSONAppendQuoted(builder, key);
	StringBuilder_AppendChar(builder, JSON_VALUE_ASSIGNMENT);
	JSONAppendQuoted(builder, Number);
}

/* Accounts. */
static bool DoSessionsMatch(unsigned int* session1, unsigned int* session2)
{
	for (int i = 0; i < SESSION_ID_LENGTH; i++)
	{
		if (session1[i] != session2[i])
		{
			return false;
		}
	}

	return true;
}

static unsigned int* ParseSession(const char* sessionString)
{
	unsigned int* Session = (unsigned int*)Memory_SafeMalloc(sizeof(unsigned int) * SESSION_ID_LENGTH);

	size_t SessionIDCount;
	char* SessionStringCopy = String_CreateCopy(sessionString);
	char** SessionIDStrings = String_Split(SessionStringCopy, SESSION_DELIMETER, &SessionIDCount);

	if (SessionIDCount != SESSION_ID_LENGTH)
	{
		Memory_Free(Session);
		Memory_Free(SessionStringCopy);
		Memory_Free(SessionIDStrings);
		return NULL;
	}

	for (int i = 0; i < SESSION_ID_LENGTH; i++)
	{
		if (!String_IsNumeric(SessionIDStrings[i]))
		{
			Memory_Free(Session);
			Memory_Free(SessionStringCopy);
			Memory_Free(SessionIDStrings);
			return NULL;
		}

		Session[i] = strtoul(SessionIDStrings[i], NULL, 10);
	}

	Memory_Free(SessionStringCopy);
	Memory_Free(SessionIDStrings);
	return Session;
}

static void BuildSessionJSONString(SessionID* session, StringBuilder* builder)
{
	StringBuilder_Construct(builder, DEFAULT_STRING_BUILDER_CAPACITY);

	
	StringBuilder_AppendChar(builder, JSON_OBJECT_OPEN);
	JSONAppendQuoted(builder, "session");
	StringBuilder_AppendChar(builder, JSON_VALUE_ASSIGNMENT);
	StringBuilder_AppendChar(builder, JSON_ARRAY_OPEN);

	for (int i = 0; i < SESSION_ID_LENGTH; i++)
	{
		if (i != 0)
		{
			StringBuilder_AppendChar(builder, JSON_DELIMETER);
		}
		char Number[16];
		snprintf(Number, sizeof(Number), "%u", session->IDValues[i]);
		StringBuilder_Append(builder, Number);
	}

	StringBuilder_AppendChar(builder, JSON_ARRAY_CLOSE);
	StringBuilder_AppendChar(builder, JSON_OBJECT_CLOSE);
}

static void BuildFoundAccountJSONString(UserAccount** accounts, size_t accountCount, StringBuilder* builder)
{
	StringBuilder_AppendChar(builder, JSON_OBJECT_OPEN);
	JSONAppendQuoted(builder, "accounts");
	StringBuilder_AppendChar(builder, JSON_VALUE_ASSIGNMENT);
	StringBuilder_AppendChar(builder, JSON_ARRAY_OPEN);
	
	for (size_t i = 0; i < accountCount; i++)
	{
		if (i != 0)
		{
			StringBuilder_AppendChar(builder, JSON_DELIMETER);
		}
		StringBuilder_AppendChar(builder, JSON_OBJECT_OPEN);

		JSONAppendInt(builder, "id", (long long)accounts[i]->ID);
		StringBuilder_AppendChar(builder, JSON_DELIMETER);

		JSONAppendString(builder, "name", accounts[i]->Name);
		StringBuilder_AppendChar(builder, JSON_DELIMETER);

		JSONAppendString(builder, "surname", accounts[i]->Surname);
		StringBuilder_AppendChar(builder, JSON_DELIMETER);

		JSONAppendString(builder, "email", accounts[i]->Email);
		StringBuilder_AppendChar(builder, JSON_DELIMETER);
		
		JSONAppendInt(builder, "creation_time", accounts[i]->CreationTime);
		StringBuilder_AppendChar(builder, JSON_DELIMETER);

		JSONAppendQuoted(builder, "posts");
		StringBuilder_AppendChar(builder, JSON_VALUE_ASSIGNMENT);
		StringBuilder_AppendChar(builder, JSON_ARRAY_OPEN);
		for (size_t PostIndex = 0; PostIndex < accounts[i]->PostCount; i++)
		{
			char Number[32];
			if (PostIndex != 0)
			{
				StringBuilder_AppendChar(builder, JSON_DELIMETER);
			}
			snprintf(Number, sizeof(Number), "%llu", accounts[i]->Posts[PostIndex]);
			StringBuilder_Append(builder, Number);
		}
		StringBuilder_AppendChar(builder, JSON_ARRAY_CLOSE);

		StringBuilder_AppendChar(builder, JSON_OBJECT_CLOSE);
	}

	StringBuilder_AppendChar(builder, JSON_ARRAY_CLOSE);
	StringBuilder_AppendChar(builder, JSON_OBJECT_OPEN);
}

static ResourceResult CreateAccount(ServerContext* context, ParsedArguments* arguments, Error* error)
{
	*error = Error_CreateSuccess();
	const char* Name = GetArgumentValueByName(arguments, "name");
	if (!Name)
	{
		return ResourceResult_Invalid;
	}
	const char* Surname = GetArgumentValueByName(arguments, "surname");
	if (!Surname)
	{
		return ResourceResult_Invalid;
	}
	const char* Email = GetArgumentValueByName(arguments, "email");
	if (!Email)
	{
		return ResourceResult_Invalid;
	}
	const char* Password = GetArgumentValueByName(arguments, "password");
	if (!Password)
	{
		return ResourceResult_Invalid;
	}

	return AccountManager_TryCreateUnverifiedAccount(context, Name, Surname, Email, Password, error)
		? ResourceResult_Successful : ResourceResult_Invalid;
}

static ResourceResult VerifyAccount(ServerContext* context, ParsedArguments* arguments, Error* error)
{
	*error = Error_CreateSuccess();
	const char* Email = GetArgumentValueByName(arguments, "email");
	if (!Email)
	{
		return ResourceResult_Invalid;
	}
	const char* CodeString = GetArgumentValueByName(arguments, "verification_code");
	if (!CodeString)
	{
		return ResourceResult_Invalid;
	}
	if (!String_IsNumeric(CodeString))
	{
		return ResourceResult_Invalid;
	}
	unsigned int Code = strtoul(CodeString, NULL, 10);

	return AccountManager_TryVerifyAccount(context, Email, Code, error)
		? ResourceResult_Successful : ResourceResult_Invalid;
}

static ResourceResult LoginAccount(ServerContext* context, ParsedArguments* arguments, ServerResourceRequest* request, Error* error)
{
	*error = Error_CreateSuccess();

	const char* Email = GetArgumentValueByName(arguments, "email");
	if (!Email)
	{
		return ResourceResult_Invalid;
	}
	const char* Password = GetArgumentValueByName(arguments, "password");
	if (!Password)
	{
		return ResourceResult_Invalid;
	}

	UserAccount* TargetAccount = AccountManager_GetAccountByEmail(context->AccountContext, Email, error);
	if ((error->Code != ErrorCode_Success) || !TargetAccount)
	{
		return ResourceResult_Invalid;
	}

	SessionID* Session = AccountManager_TryCreateSession(context->AccountContext, TargetAccount, Password);
	if (!Session)
	{
		return ResourceResult_Invalid;
	}

	BuildSessionJSONString(Session, request->ResultStringBuilder);

	return ResourceResult_Successful;
}

static UserAccount* GetAccountFromRequest(ServerContext* context, ServerResourceRequest* request, Error* error)
{
	*error = Error_CreateSuccess();
	const char* SessionValueString = GetCookieValueByName(request->CookieArray, request->CookieCount, COOKIE_NAME_SESSION);
	if (!SessionValueString)
	{
		return NULL;
	}
	
	unsigned int* SessionInRequest = ParseSession(SessionValueString);
	if (!SessionInRequest)
	{
		return ResourceResult_Invalid;
	}

	UserAccount* TargetAccount = AccountManager_GetAccountBySession(context->AccountContext, SessionInRequest, error);
	Memory_Free(SessionInRequest);
	return TargetAccount;
}

static ResourceResult DeleteAccount(ServerContext* context, ServerResourceRequest* request, ParsedArguments* arguments, Error* error)
{
	UserAccount* TargetAccount = GetAccountFromRequest(context, request, error);
	if ((error->Code != ErrorCode_Success) || !TargetAccount)
	{
		return ResourceResult_Invalid;
	}
	const char* Password = GetArgumentValueByName(arguments, "");
	if (!Password)
	{
		return ResourceResult_Invalid;
	}
	if (!TargetAccount->IsAdmin && !AccountManager_IsPasswordCorrect(TargetAccount, Password))
	{
		return ResourceResult_Invalid;
	}

	*error = AccountManager_DeleteAccount(context, TargetAccount);
	error->Code == ErrorCode_Success ? ResourceResult_Successful : ResourceResult_Invalid;
}

static ResourceResult GetAccounts(ServerContext* context, ServerResourceRequest* request, ParsedArguments* arguments, Error* error)
{
	*error = Error_CreateSuccess();
	char* IDString = GetArgumentValueByName(arguments, "id");
	if(IDString)
	{
		if (!String_IsNumeric(IDString))
		{
			return ResourceResult_Invalid;
		}
		unsigned long long ID = strtoull(IDString, NULL, 10);
		UserAccount* FoundAccount = AccountManager_GetAccountByID(context->AccountContext, ID, error);
		if (error->Code != ErrorCode_Success)
		{
			return ResourceResult_Invalid;
		}
		BuildFoundAccountJSONString(&FoundAccount, FoundAccount ? 1 : 0, request->ResultStringBuilder);
		return ResourceResult_Successful;
	}

	char* Name = GetArgumentValueByName(arguments, "name");
	if (!Name)
	{
		return ResourceResult_Invalid;
	}
	size_t AccountCount;
	UserAccount** FoundAccounts = AccountManager_GetAccountsByName(context->AccountContext, Name, &AccountCount, error);
	if (error->Code != ErrorCode_Success)
	{
		return ResourceResult_Invalid;
	}
	BuildFoundAccountJSONString(FoundAccounts, AccountCount, request->ResultStringBuilder);
	Memory_Free(FoundAccounts);
	return ResourceResult_Successful;
}

static ResourceResult EditAccount(ServerContext* context, ServerResourceRequest* request, ParsedArguments* arguments, Error* error)
{
	UserAccount* TargetAccount = GetAccountFromRequest(context, request, error);
	if ((error->Code != ErrorCode_Success) || !TargetAccount)
	{
		return ResourceResult_Invalid;
	}
	const char* Name = GetArgumentValueByName(arguments, "name");
	if (!Name)
	{
		return ResourceResult_Invalid;
	}
	const char* Surname = GetArgumentValueByName(arguments, "surname");
	if (!Surname)
	{
		return ResourceResult_Invalid;
	}
	
	return AccountManager_SetName(TargetAccount, Name) && AccountManager_SetSurname(TargetAccount, Surname) ?
		ResourceResult_Successful : ResourceResult_Invalid;
}


/* Posts. */
static void BuildPostsJSONString(Post** posts, size_t postCount, StringBuilder* builder)
{
	StringBuilder_AppendChar(builder, JSON_OBJECT_OPEN);

	JSONAppendQuoted(builder, "posts");
	StringBuilder_AppendChar(builder, JSON_VALUE_ASSIGNMENT);
	StringBuilder_AppendChar(builder, JSON_ARRAY_OPEN);

	for (size_t i = 0; i < postCount; i++)
	{
		if (i != 0)
		{
			StringBuilder_AppendChar(builder, JSON_DELIMETER);
		}
		
		StringBuilder_AppendChar(builder, JSON_OBJECT_OPEN);

		JSONAppendInt(builder, "id", posts[i]->ID);
		StringBuilder_AppendChar(builder, JSON_DELIMETER);

		JSONAppendInt(builder, "author_id", posts[i]->AuthorID);
		StringBuilder_AppendChar(builder, JSON_DELIMETER);

		JSONAppendString(builder, "title", posts[i]->Title);
		StringBuilder_AppendChar(builder, JSON_DELIMETER);

		JSONAppendString(builder, "description", posts[i]->Description);
		StringBuilder_AppendChar(builder, JSON_DELIMETER);

		const char* EncodedThumbnail = Base64_Encode(posts[i]->ThumbnailData, posts[i]->ThumbnailDataLength);
		JSONAppendString(builder, "thumbnail", EncodedThumbnail);
		StringBuilder_AppendChar(builder, JSON_DELIMETER);
		Memory_Free(EncodedThumbnail);

		JSONAppendInt(builder, "creation_time", posts[i]->CreationTime);
		StringBuilder_AppendChar(builder, JSON_DELIMETER);

		JSONAppendInt(builder, "image_count", posts[i]->ImageCount);
		StringBuilder_AppendChar(builder, JSON_DELIMETER);

		JSONAppendInt(builder, "claimer", posts[i]->ClaimerID);
		StringBuilder_AppendChar(builder, JSON_DELIMETER);

		JSONAppendInt(builder, "tags", posts[i]->Tags);
		StringBuilder_AppendChar(builder, JSON_DELIMETER);

		JSONAppendQuoted(builder, "requesters");
		StringBuilder_AppendChar(builder, JSON_VALUE_ASSIGNMENT);
		StringBuilder_AppendChar(builder, JSON_ARRAY_OPEN);
		for (size_t reqIndex = 0; reqIndex < posts[i]->RequesterCount; i++)
		{
			if (reqIndex != 0)
			{
				StringBuilder_AppendChar(builder, JSON_DELIMETER);
			}

			char Number[32];
			snprintf(Number, sizeof(Number), "%llu", posts[i]->RequesterIDs[i]);
			StringBuilder_Append(builder, Number);
		}
		StringBuilder_AppendChar(builder, JSON_ARRAY_CLOSE);

		StringBuilder_AppendChar(builder, JSON_OBJECT_CLOSE);
	}

	StringBuilder_AppendChar(builder, JSON_ARRAY_CLOSE);
	StringBuilder_AppendChar(builder, JSON_OBJECT_CLOSE);
}

static void BuildCommentsJSONString(PostComment** comments, size_t commentCount, StringBuilder* builder)
{
	StringBuilder_AppendChar(builder, JSON_OBJECT_OPEN);
	JSONAppendQuoted(builder, "comments");
	StringBuilder_AppendChar(builder, JSON_VALUE_ASSIGNMENT);
	StringBuilder_AppendChar(builder, JSON_ARRAY_OPEN);

	for (size_t i = 0; i < commentCount; i++)
	{
		if (i != 0)
		{
			StringBuilder_AppendChar(builder, JSON_DELIMETER);
		}

		StringBuilder_AppendChar(builder, JSON_OBJECT_OPEN);

		JSONAppendInt(builder, "id", comments[i]->ID);
		StringBuilder_AppendChar(builder, JSON_DELIMETER);

		JSONAppendInt(builder, "author_id", comments[i]->AuthorID);
		StringBuilder_AppendChar(builder, JSON_DELIMETER);


		JSONAppendInt(builder, "parent_comment_id", comments[i]->ParentCommentID);
		StringBuilder_AppendChar(builder, JSON_DELIMETER);

		JSONAppendInt(builder, "creation_time", comments[i]->CreationTime);
		StringBuilder_AppendChar(builder, JSON_DELIMETER);

		JSONAppendInt(builder, "edit_time", comments[i]->LastEditTime);
		StringBuilder_AppendChar(builder, JSON_DELIMETER);

		JSONAppendString(builder, "contents", comments[i]->Contents);

		StringBuilder_AppendChar(builder, JSON_OBJECT_CLOSE);
	}

	StringBuilder_AppendChar(builder, JSON_ARRAY_CLOSE);
	StringBuilder_AppendChar(builder, JSON_OBJECT_CLOSE);
}

static ResourceResult CreatePost(ServerContext* context,
	ServerResourceRequest* request,
	ParsedArguments* arguments,
	Error* error)
{
	UserAccount* TargetAccount = GetAccountFromRequest(context, request, error);
	if ((error->Code != ErrorCode_Success) || !TargetAccount)
	{
		return ResourceResult_Invalid;
	}

	const char* Title = GetArgumentValueByName(arguments, "title");
	if (!Title)
	{
		return ResourceResult_Invalid;
	}
	const char* Description = GetArgumentValueByName(arguments, "description");
	if (!Description)
	{
		return ResourceResult_Invalid;
	}
	const char* TagsString = GetArgumentValueByName(arguments, "tags");
	if (!TagsString)
	{
		return ResourceResult_Invalid;
	}
	if (!String_IsNumeric(TagsString))
	{
		return ResourceResult_Invalid;
	}
	int Tags = strtoul(TagsString, NULL, 10);

	return PostManager_BeginPostCreation(context->PostContext, TargetAccount, Title, Description, Tags) ? 
		ResourceResult_Successful : ResourceResult_Invalid;
}

static ResourceResult UploadPostImage(ServerContext* context,
	ServerResourceRequest* request,
	ParsedArguments* arguments,
	Error* error)
{
	UserAccount* TargetAccount = GetAccountFromRequest(context, request, error);
	if ((error->Code != ErrorCode_Success) || !TargetAccount)
	{
		return ResourceResult_Invalid;
	}
	const char* EncodedImageData = GetArgumentValueByName(arguments, "image");
	if (!EncodedImageData)
	{
		return ResourceResult_Invalid;
	}

	size_t DecodedDataLength;
	const char* DecodedData = Base64_Decode(EncodedImageData, &DecodedDataLength);
	if (!DecodedData)
	{
		return ResourceResult_Invalid;
	}

	*error = PostManager_UploadPostImage(context->PostContext, TargetAccount->ID, DecodedData, DecodedDataLength);
	Memory_Free((char*)DecodedData);
	*error = Error_CreateSuccess();
	return error->Code == ErrorCode_Success ? ResourceResult_Successful : ResourceResult_Invalid;
}

static ResourceResult FinishPostCreation(ServerContext* context,
	ServerResourceRequest* request,
	Error* error)
{
	UserAccount* TargetAccount = GetAccountFromRequest(context, request, error);
	if ((error->Code != ErrorCode_Success) || !TargetAccount)
	{
		return ResourceResult_Invalid;
	}

	PostManager_FinishPostCreation(context->PostContext, TargetAccount->ID, error);
	return error->Code == ErrorCode_Success ? ResourceResult_Successful : ResourceResult_Invalid;
}

static ResourceResult DeletePost(ServerContext* context,
	ServerResourceRequest* request,
	ParsedArguments* arguments,
	Error* error)
{
	UserAccount* TargetAccount = GetAccountFromRequest(context, request, error);
	if ((error->Code != ErrorCode_Success) || !TargetAccount)
	{
		return ResourceResult_Invalid;
	}
	const char* PostIDString = GetArgumentValueByName(arguments, "id");
	if (!PostIDString || !String_IsNumeric(PostIDString))
	{
		return ResourceResult_Invalid;
	}
	unsigned long long ID = strtoull(PostIDString, NULL, 10);
	Post* TargetPost = PostManager_GetPostByID(context->PostContext, ID, error);
	if (!TargetPost || (TargetPost->AuthorID != TargetAccount->ID))
	{
		return ResourceResult_Invalid;
	}

	*error = PostManager_DeletePost(context, TargetPost);
	return error->Code == ErrorCode_Success ? ResourceResult_Successful : ResourceResult_Invalid;
}


static ResourceResult CreateComment(ServerContext* context,
	ServerResourceRequest* request,
	ParsedArguments* arguments,
	Error* error)
{
	UserAccount* TargetAccount = GetAccountFromRequest(context, request, error);
	if ((error->Code != ErrorCode_Success) || !TargetAccount)
	{
		return ResourceResult_Invalid;
	}

	const char* Contents = GetArgumentValueByName(arguments, "contents");
	if (!Contents)
	{
		return ResourceResult_Invalid;
	}
	const char* TargetPostString = GetArgumentValueByName(arguments, "post_id");
	if (!TargetPostString || !String_IsNumeric(TargetPostString))
	{
		return ResourceResult_Invalid;
	}
	Post* TargetPost = PostManager_GetPostByID(context->PostContext, strtoul(TargetPostString, NULL, 10), error);
	if ((error->Code != ErrorCode_Success) || !TargetPost)
	{
		return ResourceResult_Invalid;
	}
	const char* ParentCommentIDString = GetArgumentValueByName(arguments, "parent_comment");
	if (!ParentCommentIDString || !String_IsNumeric(ParentCommentIDString))
	{
		return ResourceResult_Invalid;
	}
	unsigned long long ParentCommentID = strtoul(ParentCommentIDString, NULL, 10);

	return PostManager_AddComment(TargetPost, TargetAccount->ID, ParentCommentID, Contents) ? 
		ResourceResult_Successful : ResourceResult_Invalid;
}

static ResourceResult EditComment(ServerContext* context,
	ServerResourceRequest* request,
	ParsedArguments* arguments,
	Error* error)
{
	//UserAccount* TargetAccount = GetAccountFromRequest(context, request, error);
	//if ((error->Code != ErrorCode_Success) || !TargetAccount)
	//{
	//	return ResourceResult_Invalid;
	//}
	//const char CommentIDString = GetArgumentValueByName(arguments, "id");
	//if (!CommentIDString || !String_IsNumeric(CommentIDString))
	//{
	//	return ResourceResult_Invalid;
	//}
	//unsigned long long CommentID = strtoull(CommentIDString, NULL, 10);

	//PostComment* TargetComment = PostManager_GetComment()
}

static ResourceResult DeleteComment(ServerContext* context,
	ServerResourceRequest* request,
	ParsedArguments* arguments,
	Error* error)
{

}

static ResourceResult GetComments(ServerContext* context,
	ServerResourceRequest* request,
	ParsedArguments* arguments,
	Error* error)
{

}


/* General action finder functions. */
static ResourceResult ExecutePostAccountAction(ServerContext* context, 
	ServerResourceRequest* request,
	ParsedArguments* arguments, 
	char** paths,
	size_t pathCount,
	Error* error)
{
	*error = Error_CreateSuccess();
	if (pathCount != 1)
	{
		return ResourceResult_Invalid;
	}

	if (String_Equals(paths[0], "signup"))
	{
		return CreateAccount(context, arguments, error);
	}
	else if (String_Equals(paths[0], "verify"))
	{
		return VerifyAccount(context, arguments, error);
	}
	else if (String_Equals(paths[0], "login"))
	{
		return LoginAccount(context, arguments, request, error);
	}
	else if (String_Equals(paths[0], "edit"))
	{
		EditAccount(context, request, arguments, error);
	}
	else if (String_Equals(paths[0], "delete"))
	{
		DeleteAccount(context, request, arguments, error);
	}
	return ResourceResult_Invalid;
}

static ResourceResult ExecutePostPostCreateAction(ServerContext* context,
	ServerResourceRequest* request,
	ParsedArguments* arguments,
	char** paths,
	size_t pathCount,
	Error* error)
{
	*error = Error_CreateSuccess();
	if (pathCount != 1)
	{
		return ResourceResult_Invalid;
	}

	if (String_Equals(paths[0], "create"))
	{
		CreatePost(context, request, arguments, error);
	}
	else if (String_Equals(paths[0], "image"))
	{
		UploadPostImage(context, request, arguments, error);
	}
	else if (String_Equals(paths[0], "finish"))
	{
		FinishPostCreation(context, request, error);
	}

	return ResourceResult_Invalid;
}

static ResourceResult ExecutePostPostGetAction(ServerContext* context,
	ServerResourceRequest* request,
	ParsedArguments* arguments,
	char** paths,
	size_t pathCount,
	Error* error)
{

}

static ResourceResult ExecutePostPostCommentAction(ServerContext* context,
	ServerResourceRequest* request,
	ParsedArguments* arguments,
	char** paths,
	size_t pathCount,
	Error* error)
{
	*error = Error_CreateSuccess();

	

	return ResourceResult_Invalid;
}

static ResourceResult ExecutePostPostAction(ServerContext* context,
	ServerResourceRequest* request,
	ParsedArguments* arguments,
	char** paths,
	size_t pathCount,
	Error* error)
{
	*error = Error_CreateSuccess();
	if (pathCount < 1)
	{
		return ResourceResult_Invalid;
	}

	if (String_Equals(paths[0], "create"))
	{
		ExecutePostPostCreateAction(context, request, arguments, paths + 1, pathCount - 1, error);
	}
	else if (String_Equals(paths[0], "delete"))
	{
		DeletePost(context, request, arguments, error);
	}
	else if (String_Equals(paths[0], "edit"))
	{

	}
}

static ResourceResult ExecuteSpecialAction(ServerContext* context,
	ServerResourceRequest* request,
	ParsedArguments* arguments,
	char* action,
	Error* error)
{
	*error = Error_CreateSuccess();
	UserAccount* TargetAccount = GetAccountFromRequest(context, request, error);
	if ((error->Code != ErrorCode_Success) || !TargetAccount || !TargetAccount->IsAdmin)
	{
		return ResourceResult_Invalid;
	}

	if (String_Equals(action, "stop"))
	{
		return ResourceResult_ShutDownServer;
	}
	return ResourceResult_Invalid;
}


// Functions.
void ResourceManager_Construct(ServerResourceContext* context, const char* dataRootPath)
{
	context->DatabaseRootPath = Directory_CombinePaths(dataRootPath, DIR_NAME_DATABASE);
	context->SourceRootPath = Directory_CombinePaths(dataRootPath, DIR_NAME_SOURCE);

	Directory_CreateAll(context->DatabaseRootPath);
	Directory_CreateAll(context->SourceRootPath);
}

void ResourceManager_Deconstruct(ServerResourceContext* context)
{
	Memory_Free((char*)context->DatabaseRootPath);
	Memory_Free((char*)context->SourceRootPath);
}

ResourceResult ResourceManager_Get(ServerContext* context, ServerResourceRequest* request)
{
	ParsedArguments Arguments;
	ParseArgumentsFromBody(&Arguments, request->Data);

	return ResourceResult_Successful;
}

ResourceResult ResourceManager_Post(ServerContext* context, ServerResourceRequest* request)
{
	ParsedArguments Arguments;
	ParseArgumentsFromBody(&Arguments, request->Data);

	size_t PathCount;
	char** Paths = GetPathAsParts(request->Target, &PathCount);
	if (PathCount == 0)
	{
		ArgumentsDeconstruct(&Arguments);
		Memory_Free(Paths);
		return ResourceResult_Invalid;
	}

	ResourceResult Result;
	Error ReturnedError = Error_CreateSuccess();
	if (String_Equals(Paths[0], "account"))
	{
		Result = ExecutePostAccountAction(context, request, &Arguments, Paths + 1, PathCount - 1, &ReturnedError);
	}
	else if (String_Equals(Paths[0], "post"))
	{
		Result = ExecutePostPostAction(context, request, &Arguments, Paths + 1, PathCount - 1, &ReturnedError);
	}
	else if ((PathCount == 2) && String_Equals(Paths[0], "special"))
	{
		Result = ExecuteSpecialAction(context, request, &Arguments, Paths[1], &ReturnedError);
	}
	else
	{
		Result = ResourceResult_Invalid;
	}

	if (ReturnedError.Code != ErrorCode_Success)
	{
		Logger_LogWarning(context->Logger, ReturnedError.Message);
		Error_Deconstruct(&ReturnedError);
	}

	ArgumentsDeconstruct(&Arguments);
	Memory_Free(Paths[0]);
	Memory_Free(Paths);
	return Result;
}