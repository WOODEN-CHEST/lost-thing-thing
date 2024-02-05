#include "HttpListener.h"
#include <stdio.h>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include "LttCommon.h"
#include "LttErrors.h"
#include <stdbool.h>


// Macros.
#define REQUEST_MESSAGE_BUFFER_LENGTH 16384
#define TARGET_WSA_VERSION_MAJOR 2
#define TARGET_WSA_VERSION_MINOR 2


// Static functions.
static void ListenForClient()
{
	char* RequestMessage = Memory_SafeMalloc(REQUEST_MESSAGE_BUFFER_LENGTH);
	bool CloseServer = false;

	while (!CloseServer)
	{

	}
}


// Functions.
ErrorCode HttpListener_MainLoop()
{
	// Startup.
	WSADATA	WinSocketData;
	int Result = WSAStartup(MAKEWORD(TARGET_WSA_VERSION_MAJOR, TARGET_WSA_VERSION_MINOR), &WinSocketData);
	if (!Result)
	{
		switch (Result)
		{
			case WSASYSNOTREADY:
				return Error_SetError(ErrorCode_SocketError, "System not ready for network communication");
				break;

			case WSAVERNOTSUPPORTED:
				return Error_SetError(ErrorCode_SocketError, "Version not supported.");
				break;

			case WSAEINPROGRESS:
				return Error_SetError(ErrorCode_SocketError, "Blocking windows sockets operation in progress.");
				break;

			case WSAEPROCLIM:
				return Error_SetError(ErrorCode_SocketError, "Maximum number of sockets reached.");
				break;

			case WSAEFAULT:
				return Error_SetError(ErrorCode_SocketError, "Invalid WSDATA pointer.");
				break;

			default:
				return Error_SetError(ErrorCode_SocketError, "Unknown WSA error.");
		}
	}

	// Create socket.
	SOCKET Socket = INVALID_SOCKET;
	Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (Socket == INVALID_SOCKET)
	{
		char ErrorMessage[128];
		sprintf(ErrorMessage, "Failed to create socket. Code: %d", WSAGetLastError());
		return Error_SetError(ErrorCode_SocketError, ErrorMessage);
	}

	// Setup address.
	struct sockaddr_in Address;
	ZeroMemory(&Address, sizeof(Address));
	Address.sin_family = AF_INET;
	Address.sin_port = htons(80);
	inet_pton(AF_INET, "127.0.0.1", &Address.sin_addr.S_un.S_addr);

	// Bind socket.
	Result = bind(Socket, &Address, sizeof(Address));
	if (Result == SOCKET_ERROR)
	{
		return;
	}

	// Listen.
	ListenForClient();

	Result = listen(Socket, SOMAXCONN);
	if (Result == SOCKET_ERROR)
	{
		return;
	}

	// Accept.
	SOCKET ClientSocket = INVALID_SOCKET;
	ClientSocket = accept(Socket, NULL, NULL);

	if (ClientSocket == INVALID_SOCKET)
	{
		return;
	}


	int Length = recv(ClientSocket, RequestMessageBuffer, 1000, 0);
	RequestMessageBuffer[Length] = '\0';

	char DataToSend[] = "<!DOCTYPE html> <html lang=\"lv\"><body><h1>Hello World!</h1></body></html>";
	send(ClientSocket, DataToSend, String_LengthBytes(DataToSend), 0);

	closesocket(ClientSocket);
	WSACleanup();
}