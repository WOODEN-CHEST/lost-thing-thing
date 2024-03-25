#include "HttpListener.h"
#include <stdio.h>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include "LttCommon.h"
#include "LttErrors.h"
#include <stdbool.h>
#include <string.h>
#include "LTTServerResourceManager.h"


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

#define COOKIE_VALUE_ASSIGNMENT_OPERATOR '='
#define COOKIE_VALUE_END_OPERATOR ';'

#define HTTP_RESPONSE_TARGET_VERSION "HTTP/1.1"
#define HTTP_RESPONSE_CONTENT_LENGTH_NAME "Content-Length"
#define HTTP_STANDART_NEWLINE "\r\n"


// Types.
typedef enum SpecialActionEnum
{
	SpecialAction_None,
	SpecialAction_ShutdownServer
} SpecialAction;

typedef struct ServerRuntimeDataStruct
{
	bool IsStopRequested;
	size_t RequestCount;
} ServerRuntimeData;


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

static void AppendResponseBody(StringBuilder* builder, const char* body)
{
	char NumberBuffer[16];
	sprintf(NumberBuffer, "%llu", String_LengthBytes(body));
	StringBuilder_Append(builder, HTTP_RESPONSE_CONTENT_LENGTH_NAME);
	StringBuilder_AppendChar(builder, HEADER_VALUE_DEFINER);
	StringBuilder_AppendChar(builder, ' ');
	StringBuilder_Append(builder, NumberBuffer);
	StringBuilder_Append(builder, HTTP_STANDART_NEWLINE);

	StringBuilder_Append(builder, HTTP_STANDART_NEWLINE);

	StringBuilder_Append(builder, body);
}

static void BuildHttpResponse(StringBuilder* responseBuilder, HttpResponse* response)
{
	StringBuilder_Clear(responseBuilder);
	StringBuilder_Append(responseBuilder, HTTP_RESPONSE_TARGET_VERSION);
	StringBuilder_AppendChar(responseBuilder, ' ');
	AppendResponseCode(responseBuilder, response->Code);

	if (response->Body)
	{
		StringBuilder_Append(responseBuilder, HTTP_STANDART_NEWLINE);
		AppendResponseBody(responseBuilder, response->Body);
	}
}


/* Parsing functions. */
static const char* SkipLineUntilNonWhitespace(const char* message)
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

static const char* SkipUntilNextLine(const char* message)
{
	while (!IsEndOfMessageLine(*message))
	{
		message++;
	}

	TrySkipOverCharacter(message);

	return message;
}

static int ParseLineUntil(char* buffer, int bufferSize, const char* message, char targetCharacter)
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

static int ReadFromLine(char* buffer, int count, const char* message)
{
	int Index;
	for (Index = 0; (Index < count) && (!IsEndOfMessageLine(message[Index])); Index++)
	{
		buffer[Index] = message[Index];
	}
	buffer[Index] = '\0';

	return Index;
}

static int ReadIntFromLine(const char* message, int* readInt, bool* success)
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

/* HTTP Request Line. */
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

static const char* ReadHttpRequestLine(const char* message, HttpRequest* finalRequest)
{
	message = ReadMethod(message, finalRequest);
	message = ReadHttpRequestTarget(message, finalRequest);
	message = ReadHttpVersion(message, finalRequest);
	return message;
}


/* HTTP Cookies. */
static const char* ReadSingleHttpCookie(const char* message, HttpRequest* finalRequest)
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

static const char* ReadHttpCookies(const char* message, HttpRequest* finalRequest)
{
	for (int i = 0; (i < MAX_COOKIE_COUNT) && !IsEndOfMessageLine(*message); i++)
	{
		message = ReadSingleHttpCookie(message, finalRequest);
	}

	return message;
}


/* HTTP Headers. */
static const char* ReadHttpHeader(const char* message, HttpRequest* finalRequest)
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
static void ReadHttpRequest(const char* message, HttpRequest* finalRequest)
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

static HttpResponseCode ResourceResponseToHttpResponseCode(ResourceResult result)
{
	switch (result)
	{
		case ResourceResult_Successful:
			return HttpResponseCode_OK;

		case ResourceResult_NotFound:
			return HttpResponseCode_NotFound;

		case ResourceResult_Invalid:
			return HttpResponseCode_BadRequest;

		case ResourceResult_Unauthorized:
			return HttpResponseCode_Unauthorized;

		case ResourceResult_ShutDownServer:
			return HttpResponseCode_OK;
	
		default:
			return HttpResponseCode_ImATeapot;
	}
}

static SpecialAction ExecuteValidHttpRequest(HttpRequest* request, HttpResponse* response, StringBuilder* responseBuilder)
{
	ResourceResult Result = ResourceResult_Invalid;
	ServerResourceRequest RequestData =
	{
		request->RequestTarget,
		request->Body,
		request->CookieArray,
		request->CookieCount,
		&responseBuilder->Data
	};

	if (request->Method == HttpMethod_GET)
	{
		Result = ResourceManager_Get(&RequestData);
	}
	else if (request->Method == HttpMethod_POST)
	{
		Result = ResourceManager_Post(&RequestData);
	}

	response->Code = ResourceResponseToHttpResponseCode(Result);
	return Result == ResourceResult_ShutDownServer ? SpecialAction_ShutdownServer : SpecialAction_None;
}

static SpecialAction HandleHttpRequest(SOCKET clientSocket, HttpRequest* request, StringBuilder* responseBuilder)
{
	HttpResponse Response;
	Response.Body = NULL;
	Response.Code = HttpResponseCode_InternalServerError;
	SpecialAction RequestedAction = SpecialAction_None;

	if ((request->HttpVersionMinor == HTTP_INVALID_VERSION) || (request->HttpVersionMajor == HTTP_INVALID_VERSION)
		|| (request->Method == HttpMethod_UNKNOWN))
	{
		Response.Code = HttpResponseCode_BadRequest;
	}
	else
	{
		RequestedAction = ExecuteValidHttpRequest(request, &Response, responseBuilder);
	}
	
	BuildHttpResponse(responseBuilder, &Response);
	send(clientSocket, responseBuilder->Data, (int)responseBuilder->Length, NULL);
	return RequestedAction;
}

static void ClearHttpRequestStruct(HttpRequest* request)
{
	*request->Body = '\0';
	request->Method = HttpMethod_UNKNOWN;
	request->RequestTarget[0] = '\0';
	for (int i = 0; i < MAX_COOKIE_COUNT; i++)
	{
		(request->CookieArray[i].Value[0]) = '\0';
		(request->CookieArray[i].Name[0]) = '\0';
	}
	request->CookieCount = 0;
}


/* Client listener. */
static ErrorCode SetSocketError(const char* message, int wsaCode)
{
	char ErrorMessage[128];
	sprintf(ErrorMessage, "%s (Code: %d)", message, wsaCode);
	return Error_SetError(ErrorCode_SocketError, ErrorMessage);
}

static void HandleSpecialAction(SOCKET clientSocket, SpecialAction action, ServerRuntimeData* runtimeData)
{
	if (action == SpecialAction_ShutdownServer)
	{
		runtimeData->IsStopRequested = true;
	}
}

static ErrorCode AcceptSingleClient(SOCKET serverSocket,
	HttpRequest* request, 
	StringBuilder* responseBuilder,
	char* requestMessageBuffer,
	ServerRuntimeData* runtimeData)
{
	// Accept client.
	SOCKET ClientSocket;
	ClientSocket = accept(serverSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET)
	{
		return SetSocketError("AcceptSingleClient: Failed to accept client.", WSAGetLastError());
	}
	runtimeData->RequestCount += 1;

	int ReceivedLength = recv(ClientSocket, requestMessageBuffer, REQUEST_MESSAGE_BUFFER_LENGTH - 1, NULL);
	requestMessageBuffer[ReceivedLength] = '\0';

	// Process.
	ClearHttpRequestStruct(request);
	ReadHttpRequest(requestMessageBuffer, request);
	SpecialAction RequestedAction = HandleHttpRequest(ClientSocket, request, responseBuilder);

	// Handle any special actions.
	if (RequestedAction != SpecialAction_None)
	{
		HandleSpecialAction(ClientSocket, RequestedAction, runtimeData);
	}


	// Close connection.
	if (closesocket(ClientSocket) == SOCKET_ERROR)
	{
		return SetSocketError("AcceptSingleClient: Failed to close the socket.", WSAGetLastError());
	}

	return ErrorCode_Success;
}


static void AcceptClients(SOCKET serverSocket)
{
	// Initialize memory.
	char* RequestBuffer = (char*)Memory_SafeMalloc(REQUEST_MESSAGE_BUFFER_LENGTH);

	HttpRequest Request;
	Request.Body = (char*)Memory_SafeMalloc(REQUEST_MESSAGE_BUFFER_LENGTH);
	Request.CookieArray = (HttpCookie*)Memory_SafeMalloc(sizeof(HttpCookie) * MAX_COOKIE_COUNT);

	StringBuilder ResponseBuilder;
	StringBuilder_Construct(&ResponseBuilder, REQUEST_MESSAGE_BUFFER_LENGTH);

	ServerRuntimeData RuntimeData =
	{
		false,
		0
	};

	// Main listening loop.
	Logger_LogInfo("Started accepting clients.");
	while (!RuntimeData.IsStopRequested)
	{
		if (AcceptSingleClient(serverSocket, &Request, &ResponseBuilder, RequestBuffer, &RuntimeData) != ErrorCode_Success)
		{
			Logger_LogError(Error_GetLastErrorMessage());
		}
	}
	Logger_LogInfo("Stopped accepting clients.");

	// Cleanup.
	StringBuilder_Deconstruct(&ResponseBuilder);
	Memory_Free(Request.CookieArray);
}


/* Socket. */
static ErrorCode InitializeSocket(SOCKET* targetSocket, const char* address)
{
	// Startup.
	WSADATA	WinSocketData;
	int Result = WSAStartup(MAKEWORD(TARGET_WSA_VERSION_MAJOR, TARGET_WSA_VERSION_MINOR), &WinSocketData);
	if (Result)
	{
		return SetSocketError("WSA startup failed.", Result);
	}

	// Create socket.
	*targetSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (*targetSocket == INVALID_SOCKET)
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
	if (bind(*targetSocket, &Address, sizeof(Address)) == SOCKET_ERROR)
	{
		return SetSocketError("Failed to bind socket.", WSAGetLastError());;
	}
}

static ErrorCode CloseSocket(SOCKET* targetSocket)
{
	if (closesocket(*targetSocket) == SOCKET_ERROR)
	{
		return SetSocketError("Failed to close socket.", WSAGetLastError());
	}

	if (WSACleanup() == SOCKET_ERROR)
	{
		return SetSocketError("Failed to accept client.", WSAGetLastError());
	}

	return ErrorCode_Success;
}


// Functions.
ErrorCode HttpListener_Listen(const char* address)
{
	// Initialize.
	SOCKET Socket = INVALID_SOCKET;
	if (InitializeSocket(&Socket, address) != ErrorCode_Success)
	{
		return Error_GetLastErrorCode();
	}

	// Listen.
	if (listen(Socket, SOMAXCONN) == SOCKET_ERROR)
	{
		return SetSocketError("Failed to listen to client.", WSAGetLastError());
	}

	AcceptClients(Socket);

	// End.
	if (CloseSocket(&Socket) != ErrorCode_Success)
	{
		return Error_GetLastErrorCode();
	}
	return ErrorCode_Success;
}