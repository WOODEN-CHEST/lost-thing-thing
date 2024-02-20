#include "HttpListener.h"
#include <stdio.h>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include "LttCommon.h"
#include "LttErrors.h"
#include <stdbool.h>
#include <string.h>


// Macros.
#define REQUEST_MESSAGE_BUFFER_LENGTH 2000000
#define TARGET_WSA_VERSION_MAJOR 2
#define TARGET_WSA_VERSION_MINOR 2

#define MAX_METHOD_LENGTH 8

#define METHOD_GET "GET"
#define METHOD_POST "POST"

#define HTTP_VERSION_PREFIX "HTTP/"
#define HTTP_VERSION_SEPARATOR '.'
#define HTTP_VERSION_PREFIX_LENGTH 5
#define HTTP_INVALID_VERSION -1
#define HTTP_SEPARATOR ' '

#define IsWhitespace(character) ((0 <= character) && (character <= 32))
#define IsEndOfMessageLine(character) ((character == '\0') || (character == '\n')) // Note: They usually end in \r\n
#define IsNewline(character) ((character == '\n') || (character == '\r'))
#define IsDigit(character) (('0' <= character) && (character <= '9'))
#define TrySkipOverCharacter(message) if (*message != '\0') message++

#define MAX_HEADER_LENGTH 32
#define HEADER_VALUE_DEFINER ':'
#define HEADER_COOKIES "Cookie"

#define MAX_COOKIE_COUNT 64
#define MAX_COOKIE_NAME_LENGTH 64
#define MAX_COOKIE_VALUE_LENGTH 512
#define COOKIE_VALUE_ASSIGNMENT_OPERATOR '='
#define COOKIE_VALUE_END_OPERATOR ';'

#define HTTP_RESPONSE_TARGET_VERSION "HTTP/1.1"
#define HTTP_RESPONSE_CONTENT_LENGTH_NAME "Content-Length"
#define HTTP_STANDART_NEWLINE "\r\n"


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
	bool KeepAlive;
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


// Static functions.
/* Responses. */
static void AppendResponseCode(StringBuilder* builder, HttpResponseCode code)
{
	char Code[16];
	sprintf(Code, "%d", code);
	StringBuilder_Append(builder, Code);
	StringBuilder_AppendChar(builder, ' ');

	switch (code)
	{
		case HttpResponseCode_OK:
			StringBuilder_Append(builder, "OK");
			break;

		case HttpResponseCode_Created:
			StringBuilder_Append(builder, "Created");
			break;

		case HttpResponseCode_BadRequest:
			StringBuilder_Append(builder, "Bad Request");
			break;

		case HttpResponseCode_Unauthorized:
			StringBuilder_Append(builder, "Unauthorized");
			break;

		case HttpResponseCode_Forbidden:
			StringBuilder_Append(builder, "Forbidden");
			break;

		case HttpResponseCode_NotFound:
			StringBuilder_Append(builder, "Not Found");
			break;

		case HttpResponseCode_ImATeapot:
			StringBuilder_Append(builder, "I'm a Teapot");
			break;

		case HttpResponseCode_InternalServerError:
			StringBuilder_Append(builder, "Internal Server Error");
			break;
	}
}

static void CreateHttpResponse(StringBuilder* responseBuilder, HttpResponse* response)
{
	StringBuilder_Clear(responseBuilder);
	StringBuilder_Append(responseBuilder, HTTP_RESPONSE_TARGET_VERSION);
	StringBuilder_AppendChar(responseBuilder, ' ');
	AppendResponseCode(responseBuilder, response);
	StringBuilder_Append(responseBuilder, HTTP_STANDART_NEWLINE);


	char NumberBuffer[16];
	sprintf(NumberBuffer, "%d", String_LengthBytes(response->Body));
	StringBuilder_Append(responseBuilder, HTTP_RESPONSE_CONTENT_LENGTH_NAME);
	StringBuilder_AppendChar(responseBuilder, HEADER_VALUE_DEFINER);
}


/* Parsing functions. */
static char* SkipLineUntilNonWhitespace(char* message)
{
	while (!IsEndOfMessageLine(*message) && IsWhitespace(*message))
	{
		message++;
	}

	if (*message == '\n')
	{
		message++;
	}

	return message;
}

static char* SkipUntilNextLine(char* message)
{
	while (!IsEndOfMessageLine(*message))
	{
		message++;
	}

	TrySkipOverCharacter(message);

	return message;
}

static int ParseLineUntil(char* buffer, int bufferSize, char* message, char targetCharacter)
{
	int Index;
	for (Index = 0; (Index < bufferSize - 1) && !IsEndOfMessageLine(message[Index])
		&& (message[Index] != targetCharacter) && (message[Index] != '\r'); Index++)
	{
		buffer[Index] = message[Index];
	}

	buffer[Index] = '\0';

	if (message[Index] == '\r')
	{
		message++;
		return Index + 1;
	}
	
	return Index;
}

static int ReadFromLine(char* buffer, int count, char* message)
{
	int Index;
	for (Index = 0; (Index < count) && (!IsEndOfMessageLine(message[Index])); Index++)
	{
		buffer[Index] = message[Index];
	}
	buffer[Index] = '\0';

	return Index;
}

static int ReadIntFromLine(char* message, int* readInt, bool* success)
{
	char ReadBuffer[16];

	int Index;
	for (Index = 0; (Index < sizeof(ReadBuffer)) && IsDigit(message[Index]); Index++)
	{
		ReadBuffer[Index] = message[Index];
	}
	ReadBuffer[Index] = '\0';

	if (Index == 0)
	{
		*readInt = 0;
		*success = false;
		return Index;
	}

	*readInt = atoi(ReadBuffer);
	*success = true;
	return Index;
}

/* Http Request Line. */
static const char* ReadMethod(const char* message, HttpRequest* finalRequest)
{
	message = SkipLineUntilNonWhitespace(message);

	char MethodNameString[MAX_METHOD_LENGTH + 1];
	message = message + ParseLineUntil(MethodNameString, sizeof(MethodNameString), message, HTTP_SEPARATOR);

	if (String_Equals(MethodNameString, METHOD_GET))
	{
		finalRequest->Method = HttpMethod_GET;
	}
	else if (String_Equals(MethodNameString, METHOD_POST))
	{
		finalRequest->Method = HttpMethod_POST;
	}
	else
	{
		finalRequest->Method = HttpMethod_UNKNOWN;
	}

	return message;
}

static const char* ReadHttpRequestTarget(const char* message, HttpRequest* finalRequest)
{
	message = SkipLineUntilNonWhitespace(message);
	message = message + ParseLineUntil(finalRequest->RequestTarget, sizeof(finalRequest->RequestTarget), message, HTTP_SEPARATOR);
	return message;
}

static void SetInvalidHttpVersion(HttpRequest* request)
{
	request->HttpVersionMajor = HTTP_INVALID_VERSION;
	request->HttpVersionMinor = HTTP_INVALID_VERSION;
}

static const char* ReadHttpVersion(const char* message, HttpRequest* finalRequest)
{
	message = SkipLineUntilNonWhitespace(message);

	char ReadPrefix[HTTP_VERSION_PREFIX_LENGTH + 1];
	message = message + ReadFromLine(ReadPrefix, HTTP_VERSION_PREFIX_LENGTH, message);

	if (!String_Equals(ReadPrefix, HTTP_VERSION_PREFIX))
	{
		SetInvalidHttpVersion(finalRequest);
		return message;
	}

	bool Success;
	message = message + ReadIntFromLine(message, &finalRequest->HttpVersionMajor, &Success);
	if (!Success)
	{
		SetInvalidHttpVersion(finalRequest);
		return message;
	}

	if (*message == HTTP_VERSION_SEPARATOR)
	{
		message++;
		message = message + ReadIntFromLine(message, &finalRequest->HttpVersionMinor, &Success);

		if (!Success)
		{
			SetInvalidHttpVersion(finalRequest);
			return message;
		}
	}

	return message;
}

static char* ReadHttpRequestLine(const char* message, HttpRequest* finalRequest)
{
	message = ReadMethod(message, finalRequest);
	message = ReadHttpRequestTarget(message, finalRequest);
	message = ReadHttpVersion(message, finalRequest);
	return message;
}


/* Http Cookies. */
static char* ReadSingleHttpCookie(const char* message, HttpRequest* finalRequest)
{
	message = SkipLineUntilNonWhitespace(message);

	char Name[MAX_COOKIE_NAME_LENGTH];
	message = message + ParseLineUntil(Name, sizeof(Name), message, COOKIE_VALUE_ASSIGNMENT_OPERATOR);

	if (Name[0] == '\0')
	{
		return message;
	}

	TrySkipOverCharacter(message);

	char Value[MAX_COOKIE_VALUE_LENGTH];
	message = message + ParseLineUntil(Value, sizeof(Value), message, COOKIE_VALUE_END_OPERATOR);

	if (Value[0] == '\0')
	{
		return message;
	}
	else if (*message == COOKIE_VALUE_END_OPERATOR)
	{
		TrySkipOverCharacter(message);
	}


	String_CopyTo(Name, finalRequest->CookieArray[finalRequest->CookieCount].Name);
	String_CopyTo(Value, finalRequest->CookieArray[finalRequest->CookieCount].Value);
	finalRequest->CookieCount++;

	return message;
}

static char* ReadHttpCookies(const char* message, HttpRequest* finalRequest)
{
	for (int i = 0; (i < MAX_COOKIE_COUNT) && !IsEndOfMessageLine(*message); i++)
	{
		message = ReadSingleHttpCookie(message, finalRequest);
	}

	return message;
}


/* Http Headers. */
static char* ReadHttpHeader(const char* message, HttpRequest* finalRequest)
{
	message = SkipLineUntilNonWhitespace(message);

	char HeaderName[MAX_HEADER_LENGTH];
	message = message + ParseLineUntil(HeaderName, sizeof(HeaderName), message, HEADER_VALUE_DEFINER);

	if (String_Equals(HeaderName, HEADER_COOKIES))
	{
		TrySkipOverCharacter(message);
		message = ReadHttpCookies(message, finalRequest);
	}

	return SkipUntilNextLine(message);
}


/* Requests. */
static bool ReadHttpRequest(const char* message, HttpRequest* finalRequest)
{
	message = ReadHttpRequestLine(message, finalRequest);

	int HeaderLength;
	do
	{
		message = ReadHttpHeader(message, finalRequest, &HeaderLength);
	} while ((*message != '\r') && !IsEndOfMessageLine(*message));

	message = SkipUntilNextLine(message);

	String_CopyTo(message, finalRequest->Body);
}

static char* HandleHttpRequest(HttpRequest* resuest)
{
	return "{\"text\":\"Hello World!\"}";
}

static ErrorCode SetSocketError(const char* message, int wsaCode)
{
	char ErrorMessage[128];
	sprintf(ErrorMessage, "%s (Code: %d)", message, wsaCode);
	return Error_SetError(ErrorCode_SocketError, ErrorMessage);
}

static void ClearHttpRequestStruct(HttpRequest* request)
{
	*request->Body = '\0';
	request->Method = HttpMethod_UNKNOWN;
	request->KeepAlive = false;
	request->RequestTarget[0] = '\0';
	for (int i = 0; i < MAX_COOKIE_COUNT; i++)
	{
		(request->CookieArray[i].Value[0]) = '\0';
		(request->CookieArray[i].Name[0]) = '\0';
	}
	request->CookieCount = 0;
}

static ErrorCode AcceptClients(SOCKET serverSocket)
{
	// Initialize request struct and memory.
	char* RequestMessage = Memory_SafeMalloc(REQUEST_MESSAGE_BUFFER_LENGTH);

	HttpRequest Request;
	Request.Body = (char*)Memory_SafeMalloc(REQUEST_MESSAGE_BUFFER_LENGTH);
	Request.CookieArray = (HttpCookie*)Memory_SafeMalloc(sizeof(HttpCookie) * MAX_COOKIE_COUNT);


	// Main listening loop.
	while (1)
	{
		// Accept client.
		SOCKET ClientSocket;
		ClientSocket = accept(serverSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET)
		{
			return SetSocketError("Failed to accept client.", WSAGetLastError());
		}

		int ReceivedLength = recv(ClientSocket, RequestMessage, REQUEST_MESSAGE_BUFFER_LENGTH - 1, 0);
		RequestMessage[ReceivedLength] = '\0';

		// Process.
		ClearHttpRequestStruct(&Request);

		if (ReadHttpRequest(RequestMessage, &Request))
		{
			const char* Response = HandleHttpRequest(&Request);
			send(ClientSocket, Response, (int)String_LengthBytes(Response), NULL);
		}

		// Close connection.
		if (closesocket(ClientSocket) == SOCKET_ERROR)
		{
			return SetSocketError("Failed to close the socket.", WSAGetLastError());
		}
	}

	return ErrorCode_Success;
}


// Functions.
ErrorCode HttpListener_Listen(const char* address)
{
	// Startup.
	WSADATA	WinSocketData;
	int Result = WSAStartup(MAKEWORD(TARGET_WSA_VERSION_MAJOR, TARGET_WSA_VERSION_MINOR), &WinSocketData);
	if (Result)
	{
		return SetSocketError("WSA startup failed.", Result);
	}

	// Create socket.
	SOCKET Socket = INVALID_SOCKET;
	Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (Socket == INVALID_SOCKET)
	{
		return SetSocketError("Failed to create socket.", WSAGetLastError());
	}

	// Setup address.
	struct sockaddr_in Address;
	ZeroMemory(&Address, sizeof(Address));
	Address.sin_family = AF_INET;
	Address.sin_port = htons(80);
	inet_pton(AF_INET, address, &Address.sin_addr.S_un.S_addr);

	// Bind socket.
	if (bind(Socket, &Address, sizeof(Address)) == SOCKET_ERROR)
	{
		return SetSocketError("Failed to bind socket.", WSAGetLastError());;
	}

	// Listen.
	if (listen(Socket, SOMAXCONN) == SOCKET_ERROR)
	{
		return SetSocketError("Failed to listen to client.", WSAGetLastError());
	}

	if (AcceptClients(Socket) != ErrorCode_Success)
	{
		return Error_GetLastErrorCode();
	}


	// End.
	if (WSACleanup() == SOCKET_ERROR)
	{
		return SetSocketError("Failed to accept client.", WSAGetLastError());
	}
}