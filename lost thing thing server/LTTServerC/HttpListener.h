#pragma once
#include "LttErrors.h"
#include <stdbool.h>


// Macros.
#define MAX_COOKIE_COUNT 64
#define MAX_COOKIE_NAME_LENGTH 64
#define MAX_COOKIE_VALUE_LENGTH 512


// Types.
typedef enum HttpMethodEnum
{
	HttpMethod_GET,
	HttpMethod_POST,
	HttpMethod_UNKNOWN
} HttpMethod;

typedef struct HttpCookieStruct
{
	char Name[MAX_COOKIE_NAME_LENGTH];
	char Value[MAX_COOKIE_VALUE_LENGTH];
} HttpCookie;

typedef struct HttpRequestStruct
{
	HttpMethod Method;
	char RequestTarget[512];

	int HttpVersionMajor;
	int HttpVersionMinor;

	HttpCookie* CookieArray;
	size_t CookieCount;

	char* Body;
} HttpRequest;

typedef enum HttpResponseCodeEnum
{
	HttpResponseCode_OK = 200,
	HttpResponseCode_Created = 201,
	HttpResponseCode_BadRequest = 400,
	HttpResponseCode_Unauthorized = 401,
	HttpResponseCode_Forbidden = 403,
	HttpResponseCode_NotFound = 404,
	HttpResponseCode_ImATeapot = 418,
	HttpResponseCode_InternalServerError = 500
} HttpResponseCode;

typedef struct HttpResponseStruct
{
	HttpResponseCode Code;
	char* Body;
} HttpResponse;


// Functions.
ErrorCode HttpListener_Listen(const char* address);