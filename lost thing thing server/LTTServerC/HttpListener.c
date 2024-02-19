#include "HttpListener.h"
#include <stdio.h>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include "LttCommon.h"
#include "LttErrors.h"
#include <stdbool.h>
#include <string.h>


// Types.
typedef enum HttpMethodEnum
{
	HttpMethod_GET,
	HttpMethod_POST,
	HttpMethod_UNKNOWN
} HttpMethod;

typedef struct HttpRequestStruct
{
	HttpMethod Method;
	char RequestTarget[512];
	int HttpVersionMajor;
	int HttpVersionMinor;
	char* Body;
	bool KeepAlive;
} HttpRequest;

typedef enum HttpResponseCodeEnum
{
	HttpResponseCode_NotFound = 404
} HttpResponseCode;

typedef struct HttpResponse
{
	HttpResponseCode Code;
	char* Body;
};



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

#define IsWhitespace(character) ((0 <= character) && (character <= 32))
#define IsEndOfMessageLine(character) ((character == '\0') || (character == '\n')) // Note: They usually end in \r\n
#define IsNewline(character) ((character == '\n') || (character == '\r'))
#define IsDigit(character) (('0' <= character) || (character <= '9'))


// Static functions.
static char* SkipLineUntilNonWhitespace(char* message)
{
	while (!IsEndOfMessageLine(*message) && IsWhitespace(*message))
	{
		message++;
	}
	return message;
}

static const char* ReadMethod(const char* message, HttpRequest* finalRequest)
{
	message = SkipLineUntilNonWhitespace(message);

	char MethodNameString[MAX_METHOD_LENGTH + 1];
	for (int i = 0; i < sizeof(MethodNameString); i++)
	{
		MethodNameString[i] = '\0';
	}

	int Index;
	for (Index = 0; (Index < MAX_METHOD_LENGTH) && !IsWhitespace(message[Index]); Index++)
	{
		MethodNameString[Index] = message[Index];
	}

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

	return message + Index;
}

static const char* ReadHttpRequestTarget(const char* message, HttpRequest* finalRequest)
{
	message = SkipLineUntilNonWhitespace(message);

	int Index;
	for (Index = 0; (Index < sizeof(finalRequest->RequestTarget)) && !IsWhitespace(message[Index]); Index++)
	{
		finalRequest->RequestTarget[Index] = *(message + Index);
	}
	finalRequest->RequestTarget[Index] = '\0';

	return message + Index;
}

static void SetInvalidHttpVersion(HttpRequest* request)
{
	request->HttpVersionMajor = HTTP_INVALID_VERSION;
	request->HttpVersionMinor = HTTP_INVALID_VERSION;
}

static const char* ReadHttpVersion(const char* message, HttpRequest* finalRequest)
{
	message = SkipLineUntilNonWhitespace(message);

	const char* Prefix = HTTP_VERSION_PREFIX;
	int Index;
	for (Index = 0; Index < HTTP_VERSION_PREFIX_LENGTH; Index++)
	{
		if (message[Index] != Prefix[Index])
		{
			SetInvalidHttpVersion(finalRequest);
			return message + Index;
		}
	}

	message += Index;
	Index = 0;

	char Version[16];
	int SecondVerIndex = -1;
	for (Index = 0; (Index < sizeof(Version) - 1) && !IsWhitespace(message[Index]); Index++)
	{
		if (message[Index] == HTTP_VERSION_SEPARATOR)
		{
			if (SecondVerIndex != -1)
			{
				SetInvalidHttpVersion(finalRequest);
				return message + Index;
			}

			Version[Index] = '\0';
			SecondVerIndex = Index + 1;
		}
		else if (IsDigit(message[Index]))
		{
			Version[Index] = message[Index];
		}
		else
		{
			SetInvalidHttpVersion(finalRequest);
			return message + Index;
		}
	}

	Version[Index] = '\0';

	finalRequest->HttpVersionMajor = strtol(Version, NULL, 10);
	finalRequest->HttpVersionMinor = SecondVerIndex != -1 ? strtol(Version + SecondVerIndex, NULL, 10) : 0;

	return message + Index;
}

static char* ParseUntilBody(const char* message)
{
	message = SkipLineUntilNonWhitespace(message);

	int LineLength = 0;
	while (*message != '\0')
	{
		if (!IsNewline(*message))
		{
			LineLength++;
		}
		else if (*message == '\n')
		{
			if (LineLength == 0)
			{
				return message + 1;
			}

			LineLength = 0;
		}

		message++;
	}

	return message;
}


static void ReadHttpRequestLine(const char* message, HttpRequest* finalRequest)
{
	message = ReadMethod(message, finalRequest);
	message = ReadHttpRequestTarget(message, finalRequest);
	message = ReadHttpVersion(message, finalRequest);
	finalRequest->Body = ParseUntilBody(message);
}

static bool ReadHttpRequest(const char* message, HttpRequest* finalRequest)
{
	ReadHttpRequestLine(message, finalRequest);
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
}

static ErrorCode AcceptClients(SOCKET serverSocket)
{
	char* RequestMessage = Memory_SafeMalloc(REQUEST_MESSAGE_BUFFER_LENGTH);
	HttpRequest Request;
	Request.Body = Memory_SafeMalloc(REQUEST_MESSAGE_BUFFER_LENGTH);

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