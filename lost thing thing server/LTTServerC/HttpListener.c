#include "HttpListener.h"
#include <stdio.h>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include "LttErrors.h"
#include <stdbool.h>
#include <string.h>
#include "LTTServerResourceManager.h"
#include "LttString.h"
#include "Memory.h"
#include "LttString.h"
#include "Logger.h"
#include "ConfigFile.h"


// Macros.
#define REQUEST_MESSAGE_BUFFER_LENGTH 6000000

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
#define IsEndOfMessageLine(character) ((character == '\0') || (character == '\n')) // Note: They end in \r\n
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

#define HTTP_PORT 80

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
	StringBuilder Body;
	StringBuilder FinalMessage;
} HttpResponse;


// Static functions.
static Error SetSocketError(const char* message, int wsaCode)
{
	char ErrorMessage[256];
	snprintf(ErrorMessage, sizeof(ErrorMessage), "%s (Code: %d)", message, wsaCode);
	return Error_CreateError(ErrorCode_SocketError, ErrorMessage);
}

/* Responses. */
static void AppendResponseCode(HttpResponse* response)
{
	char Code[16];
	snprintf(Code, sizeof(Code), "%d", (int)response->Code);
	StringBuilder_Append(&response->FinalMessage, Code);
	StringBuilder_AppendChar(&response->FinalMessage, ' ');

	switch (response->Code)
	{
		case HttpResponseCode_OK:
			StringBuilder_Append(&response->FinalMessage, "OK");
			break;

		case HttpResponseCode_Created:
			StringBuilder_Append(&response->FinalMessage, "Created");
			break;

		case HttpResponseCode_BadRequest:
			StringBuilder_Append(&response->FinalMessage, "Bad Request");
			break;

		case HttpResponseCode_Unauthorized:
			StringBuilder_Append(&response->FinalMessage, "Unauthorized");
			break;

		case HttpResponseCode_Forbidden:
			StringBuilder_Append(&response->FinalMessage, "Forbidden");
			break;

		case HttpResponseCode_NotFound:
			StringBuilder_Append(&response->FinalMessage, "Not Found");
			break;

		case HttpResponseCode_ImATeapot:
			StringBuilder_Append(&response->FinalMessage, "I'm a Teapot");
			break;

		case HttpResponseCode_InternalServerError:
			StringBuilder_Append(&response->FinalMessage, "Internal Server Error");
			break;
	}
}

static void AppendResponseBody(HttpResponse* response)
{
	char NumberBuffer[16];
	snprintf(NumberBuffer, sizeof(NumberBuffer), "%llu", response->Body.Length);
	StringBuilder_Append(&response->FinalMessage, HTTP_RESPONSE_CONTENT_LENGTH_NAME);
	StringBuilder_AppendChar(&response->FinalMessage, HEADER_VALUE_DEFINER);
	StringBuilder_AppendChar(&response->FinalMessage, ' ');
	StringBuilder_Append(&response->FinalMessage, NumberBuffer);

	StringBuilder_Append(&response->FinalMessage, HTTP_STANDART_NEWLINE);
	StringBuilder_Append(&response->FinalMessage, HTTP_STANDART_NEWLINE);

	StringBuilder_Append(&response->FinalMessage, response->Body.Data);
}

static void BuildHttpResponse(HttpResponse* response)
{
	StringBuilder_Clear(&response->FinalMessage);
	StringBuilder_Append(&response->FinalMessage, HTTP_RESPONSE_TARGET_VERSION);
	StringBuilder_AppendChar(&response->FinalMessage, ' ');
	AppendResponseCode(response);

	if (response->Body.Length > 0)
	{
		StringBuilder_Append(&response->FinalMessage, HTTP_STANDART_NEWLINE);
		AppendResponseBody(response);
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
static const char* ReadMethod(const char* message, HttpClientRequest* finalRequest)
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

static const char* ReadHttpRequestTarget(const char* message, HttpClientRequest* finalRequest)
{
	message = SkipLineUntilNonWhitespace(message);
	message = message + ParseLineUntil(finalRequest->RequestTarget, sizeof(finalRequest->RequestTarget), message, HTTP_SEPARATOR);
	return message;
}

static void SetInvalidHttpVersion(HttpClientRequest* request)
{
	request->HttpVersionMajor = HTTP_INVALID_VERSION;
	request->HttpVersionMinor = HTTP_INVALID_VERSION;
}

static const char* ReadHttpVersion(const char* message, HttpClientRequest* finalRequest)
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

static const char* ReadHttpRequestLine(const char* message, HttpClientRequest* finalRequest)
{
	message = ReadMethod(message, finalRequest);
	message = ReadHttpRequestTarget(message, finalRequest);
	message = ReadHttpVersion(message, finalRequest);
	return message;
}


/* HTTP Cookies. */
static const char* ReadSingleHttpCookie(const char* message, HttpClientRequest* finalRequest)
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

static const char* ReadHttpCookies(const char* message, HttpClientRequest* finalRequest)
{
	for (int i = 0; (i < MAX_COOKIE_COUNT) && !IsEndOfMessageLine(*message); i++)
	{
		message = ReadSingleHttpCookie(message, finalRequest);
	}

	return message;
}


/* HTTP Headers. */
static const char* ReadHttpHeader(const char* message, HttpClientRequest* finalRequest)
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
static Error ParseHttpRequestMessage(const char* message, HttpClientRequest* finalRequest)
{
	if (!String_IsValidUTF8String(message))
	{
		return Error_CreateError(ErrorCode_InvalidRequest, "HTTP request was not a valid UTF-8 string.");
	}

	message = ReadHttpRequestLine(message, finalRequest);

	do
	{
		message = ReadHttpHeader(message, finalRequest);
	} while ((*message != '\r') && !IsEndOfMessageLine(*message));

	message = SkipUntilNextLine(message);

	String_CopyTo(message, finalRequest->Body);
	return Error_CreateSuccess();
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

static SpecialAction ExecuteValidHttpRequest(ServerContext* context, HttpClientRequest* request, HttpResponse* response)
{
	ResourceResult Result = ResourceResult_Invalid;
	ServerResourceRequest ResourceRequestData =
	{
		request->RequestTarget,
		request->Body,
		request->CookieArray,
		request->CookieCount,
		&response->Body
	};

	if (request->Method == HttpMethod_GET)
	{
		Result = ResourceManager_Get(context, &ResourceRequestData);
	}
	else if (request->Method == HttpMethod_POST)
	{
		Result = ResourceManager_Post(context, &ResourceRequestData);
	}

	response->Code = ResourceResponseToHttpResponseCode(Result);
	return Result == ResourceResult_ShutDownServer ? SpecialAction_ShutdownServer : SpecialAction_None;
}

static void ClearHttpResponse(HttpResponse* response)
{
	response->Code = HttpResponseCode_InternalServerError;
	StringBuilder_Clear(&response->Body);
	StringBuilder_Clear(&response->FinalMessage);
}

static void ClearHttpRequestStruct(HttpClientRequest* request)
{
	request->Body[0] = '\0';
	request->Method = HttpMethod_UNKNOWN;
	request->RequestTarget[0] = '\0';
	for (int i = 0; i < MAX_COOKIE_COUNT; i++)
	{
		(request->CookieArray[i].Value[0]) = '\0';
		(request->CookieArray[i].Name[0]) = '\0';
	}
	request->CookieCount = 0;
}

static void HandleSpecialAction(SOCKET clientSocket, SpecialAction action, ServerRuntimeData* runtimeData)
{
	if (action == SpecialAction_ShutdownServer)
	{
		runtimeData->IsStopRequested = true;
	}
}

static Error ProcessHttpRequest(ServerContext* context,
	SOCKET clientSocket,
	char* unparsedRequestMessage,
	HttpClientRequest* requestToBuild,
	HttpResponse* responseToBuild,
	ServerRuntimeData* runtimeData)
{
	// Parse request.
	ClearHttpRequestStruct(requestToBuild);
	Error ReturnedError = ParseHttpRequestMessage(unparsedRequestMessage, requestToBuild))
	{
		return ReturnedError;
	}

	// Verify it.
	ClearHttpResponse(responseToBuild);
	SpecialAction RequestedAction = SpecialAction_None;

	if ((requestToBuild->HttpVersionMinor == HTTP_INVALID_VERSION) || (requestToBuild->HttpVersionMajor == HTTP_INVALID_VERSION)
		|| (requestToBuild->Method == HttpMethod_UNKNOWN))
	{
		responseToBuild->Code = HttpResponseCode_BadRequest;
	}
	else
	{
		RequestedAction = ExecuteValidHttpRequest(context, requestToBuild, responseToBuild);
	}

	// Respond.
	BuildHttpResponse(responseToBuild);
	if (send(clientSocket, responseToBuild->FinalMessage.Data, (int)responseToBuild->FinalMessage.Length, 0) == INVALID_SOCKET)
	{
		closesocket(clientSocket);
		return SetSocketError("Failed to send data to client.", WSAGetLastError());
	}

	// Handle any special actions.
	if (RequestedAction != SpecialAction_None)
	{
		HandleSpecialAction(clientSocket, RequestedAction, runtimeData);
	}

	return Error_CreateSuccess();
}

/* Client listener. */
static Error AcceptSingleClient(ServerContext* context, 
	SOCKET serverSocket,
	char* unparsedRequestMessage,
	HttpClientRequest* requestToBuild, 
	HttpResponse* responseToBuild,
	ServerRuntimeData* runtimeData)
{
	// Accept client.
	SOCKET ClientSocket;
	Error ReturnedError;
	ClientSocket = accept(serverSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET)
	{
		return SetSocketError("AcceptSingleClient: Failed to accept client.", WSAGetLastError());
	}
	runtimeData->RequestCount += 1;

	int ReceivedLength = recv(ClientSocket, unparsedRequestMessage, REQUEST_MESSAGE_BUFFER_LENGTH - 1, 0);
	if (ReceivedLength == SOCKET_ERROR)
	{
		ReturnedError = SetSocketError("Failed to receive client data", WSAGetLastError());
		closesocket(ClientSocket);
		return ReturnedError;
	}
	unparsedRequestMessage[ReceivedLength] = '\0';

	// Process.
	ReturnedError = ProcessHttpRequest(context, ClientSocket, unparsedRequestMessage, requestToBuild, responseToBuild, runtimeData);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	// Close connection.
	if (closesocket(ClientSocket) == SOCKET_ERROR)
	{
		return SetSocketError("AcceptSingleClient: Failed to close the socket.", WSAGetLastError());
	}

	return Error_CreateSuccess();
}


static void AcceptClients(ServerContext* context, SOCKET serverSocket)
{
	// Initialize memory.
	char* UnparsedRequestBuffer = (char*)Memory_SafeMalloc(REQUEST_MESSAGE_BUFFER_LENGTH);

	HttpClientRequest Request;
	Request.Body = (char*)Memory_SafeMalloc(REQUEST_MESSAGE_BUFFER_LENGTH);
	Request.CookieArray = (HttpCookie*)Memory_SafeMalloc(sizeof(HttpCookie) * MAX_COOKIE_COUNT);

	HttpResponse Response;
	StringBuilder_Construct(&Response.Body, DEFAULT_STRING_BUILDER_CAPACITY);
	StringBuilder_Construct(&Response.FinalMessage, DEFAULT_STRING_BUILDER_CAPACITY);

	ServerRuntimeData RuntimeData = { false, 0 };

	// Main listening loop.
	Logger_LogInfo(context->Logger, "Started accepting clients.");
	while (!RuntimeData.IsStopRequested)
	{
		Error ReturnedError = AcceptSingleClient(context, serverSocket, UnparsedRequestBuffer, &Request, &Response, &RuntimeData);
		if (ReturnedError.Code != ErrorCode_Success)
		{
			Logger_LogError(context->Logger, ReturnedError.Message);
			Error_Deconstruct(&ReturnedError);
		}
	}
	Logger_LogInfo(context->Logger, "Stopped accepting clients.");

	// Cleanup.
	StringBuilder_Deconstruct(&Response.Body);
	Memory_Free(Request.CookieArray);
}


/* Socket. */
static Error InitializeSocket(SOCKET* targetSocket, const char* address)
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
	Address.sin_port = htons(HTTP_PORT);
	//inet_pton(AF_INET, address, &Address.sin_addr.S_un.S_addr);
	inet_pton(AF_INET, address, &Address.sin_addr.S_un.S_addr);

	// Bind socket.
	if (bind(*targetSocket, (const struct sockaddr*)&Address, sizeof(Address)) == SOCKET_ERROR)
	{
		return SetSocketError("Failed to bind socket.", WSAGetLastError());;
	}

	return Error_CreateSuccess();
}

static Error CloseSocket(SOCKET* targetSocket)
{
	if (closesocket(*targetSocket) == SOCKET_ERROR)
	{
		return SetSocketError("Failed to close socket.", WSAGetLastError());
	}

	if (WSACleanup() == SOCKET_ERROR)
	{
		return SetSocketError("Failed to accept client.", WSAGetLastError());
	}

	return Error_CreateSuccess();
}


// Functions.
Error HttpListener_Listen(ServerContext* context)
{
	Error ReturnedError;

	// Initialize.
	SOCKET Socket = INVALID_SOCKET;
	ReturnedError = InitializeSocket(&Socket, context->Configuration->Address);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	// Listen.
	if (listen(Socket, SOMAXCONN) == SOCKET_ERROR)
	{
		return SetSocketError("Failed to listen to client.", WSAGetLastError());
	}

	AcceptClients(context, Socket);

	// End.
	ReturnedError = CloseSocket(&Socket);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	return Error_CreateSuccess();
}