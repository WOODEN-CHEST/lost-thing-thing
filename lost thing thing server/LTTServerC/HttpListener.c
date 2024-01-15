#include "HttpListener.h"
#include <stdio.h>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include "LttCommon.h"

#pragma comment(lib, "Ws2_32.lib")


static char* RequestMessageBuffer;
#define REQUEST_MESSAGE_BUFFER_LENGTH 16384

void Main()
{
	// Startup.
	WSADATA	WinSocketData;
		
	int Result = WSAStartup(MAKEWORD(2, 2), &WinSocketData);
	if (Result != 0)
	{
		return;
	}
	RequestMessageBuffer = Memory_SafeMalloc(REQUEST_MESSAGE_BUFFER_LENGTH);

	// Create socket.
	SOCKET Socket = INVALID_SOCKET;
	Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (Socket == INVALID_SOCKET)
	{
		return;
	}

	// Address.
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

	
	int a = 5;
}