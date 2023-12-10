#include "HttpListener.h"
#include <stdio.h>
#include <WS2tcpip.h>
#include <WinSock2.h>

#pragma comment(lib, "Ws2_32.lib")

void Main()
{
	// Startup.
	WSADATA	WinSocketData;
		
	int Result = WSAStartup(MAKEWORD(2, 2), &WinSocketData);
	if (Result != 0)
	{
		return;
	}

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

	char Data[1000];
	recv(ClientSocket, Data, 1000, 0);
	int a = 5;
}