#include "HttpListener.h"
#include <stdio.h>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include "LttCommon.h"
#include "LttErrors.h"
#include <stdbool.h>


// Types.
typedef struct HttpRequestStruct
{
	char Method[8];
} HttpRequest;


// Macros.
#define REQUEST_MESSAGE_BUFFER_LENGTH 16384
#define TARGET_WSA_VERSION_MAJOR 2
#define TARGET_WSA_VERSION_MINOR 2


// Static functions.
static bool ReadHttpRequest(const char* message, HttpRequest* finalRequest)
{

}

static const char* HandleHttpRequest(HttpRequest* resuest)
{
	return "<!DOCTYPE html> <html lang=\"lv\"><body><h1>Hello World!</h1></body></html>";
}

static ErrorCode SetSocketError(const char* message, int wsaCode)
{
	char ErrorMessage[128];
	sprintf(ErrorMessage, "%s (Code: %d)", message, wsaCode);
	return Error_SetError(ErrorCode_SocketError, ErrorMessage);
}

static ErrorCode AcceptClients(SOCKET serverSocket)
{
	char* RequestMessage = Memory_SafeMalloc(REQUEST_MESSAGE_BUFFER_LENGTH);

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
		HttpRequest Request;
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
ErrorCode HttpListener_MainLoop()
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
	inet_pton(AF_INET, "127.0.0.1", &Address.sin_addr.S_un.S_addr);

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