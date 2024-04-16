#include "LTTSMTP.h"
#include <WS2tcpip.h>
#include <WinSock2.h>
#include "LTTErrors.h"
#include <stdlib.h>
#include "LTTString.h"

// Macros.
#define SMTP_PORT 587


// Static functions.
static Error SetSocketError(const char* message, int wsaCode)
{
	char ErrorMessage[256];
	snprintf(ErrorMessage, sizeof(ErrorMessage), "%s (Code: %d)", message, wsaCode);
	return Error_CreateError(ErrorCode_SocketError, ErrorMessage);
}

static Error CreateSocket(SOCKET* socketToCreate, struct sockaddr_in* address)
{
	*socketToCreate = INVALID_SOCKET;

	SOCKET Socket = INVALID_SOCKET;
	Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Socket == INVALID_SOCKET)
	{
		return SetSocketError("Failed to create SMTP socket.", WSAGetLastError());
	}
	*socketToCreate = Socket;

	
	ZeroMemory(address, sizeof(struct sockaddr_in));
	address->sin_family = AF_INET;
	address->sin_port = htons(SMTP_PORT);
	inet_pton(AF_INET, "smtp.gmail.com", &address->sin_addr.S_un.S_addr);

	return Error_CreateSuccess();
}

static Error SendMail(SMPTCredentials* credentials,
	const char* targetEmail,
	const char* subject,
	const char* body,
	SOCKET socket,
	struct sockaddr* address)
{
	char ReceivedBuffer[512];

	int ReturnedValue = connect(socket, address, sizeof(struct sockaddr_in));
	if (ReturnedValue)
	{
		return SetSocketError("Failed to connect to SMTP server.", WSAGetLastError());
	}

	char Message[512];
	String_CopyTo("EHLO 46.109.13.132", Message);
	send(socket, Message, String_LengthBytes(Message), 0);
	int ReceivedLength = recv(socket, Message, sizeof(Message) - 1, 0);
	Message[ReceivedLength] = '\0';
}


// Functions.
Error SMTP_SendEmail(SMPTCredentials* credentials, const char* targetEmail, const char* subject, const char* body)
{	
	WSADATA WinSockData;
	WSAStartup(MAKEWORD(2, 2), &WinSockData);

	struct sockaddr_in Address;
	SOCKET Socket;
	Error ReturnedError = CreateSocket(&Socket, &Address);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	ReturnedError = SendMail(credentials, targetEmail, subject, body, Socket, &Address);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	return Error_CreateSuccess();
}