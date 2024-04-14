#include "LttErrors.h"
#include "Logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "LTTServerC.h"
#include "Memory.h"


// Static functions.
static void CopyErrorNameIntoBuffer(ErrorCode code, char* buffer)
{
	switch (code)
	{
		case ErrorCode_NullReference:
			strcpy(buffer, "Null Reference Error. ");
			break;

		case ErrorCode_InvalidArgument:
			strcpy(buffer, "Invalid Argument Error. ");
			break;

		case ErrorCode_ArgumentOutOfRange:
			strcpy(buffer, "Argument Out Of Range Error. ");
			break;

		case ErrorCode_IllegalOperation:
			strcpy(buffer, "Illegal Operation Error. ");
			break;

		case ErrorCode_IllegalState:
			strcpy(buffer, "Illegal State Error. ");
			break;

		case ErrorCode_IO:
			strcpy(buffer, "Input Output Error. ");
			break;

		case ErrorCode_IndexOutOfRange:
			strcpy(buffer, "Index Out of Range Error. ");
			break;
			
		case ErrorCode_SocketError:
			strcpy(buffer, "Socket Error. ");
			break;

		case ErrorCode_InvalidGHDFFile:
			strcpy(buffer, "Invalid GHDF File. ");
			break;

		case ErrorCode_DatabaseError:
			strcpy(buffer, "Database Error. ");
			break;

		case ErrorCode_InvalidRequest:
			strcpy(buffer, "Invalid Request. ");
			break;

		case ErrorCode_InvalidConfigFile:
			strcpy(buffer, "Invalid Config File. ");
			break;

		default:
			strcpy(buffer, "Unknown Error. ");
			break;
	}
}

static const char* GetErrorMessage(ErrorCode code, const char* message)
{
	if (code == ErrorCode_Success)
	{
		return NULL;
	}

	const size_t MAX_ERROR_NAME_SIZE = 50;
	size_t MessageLength = strlen(message);

	char* FinalMessage = (char*)Memory_SafeMalloc(MessageLength + MAX_ERROR_NAME_SIZE);

	CopyErrorNameIntoBuffer(code, FinalMessage);
	size_t CodeNameLength = strlen(FinalMessage);
	strcpy(FinalMessage + CodeNameLength, message);

	return FinalMessage;
}


// Functions.
Error Error_CreateError(ErrorCode code, const char* message)
{
	Error CreatedError =
	{
		.Code = code,
		.Message = GetErrorMessage(code, message)
	};
	return CreatedError;
}

Error Error_CreateSuccess()
{
	Error CreatedError =
	{
		.Code = ErrorCode_Success,
		.Message = NULL
	};
	return CreatedError;
}

void Error_Deconstruct(Error* error)
{
	if (error->Message)
	{
		free((char*)error->Message);
	}
}

void Error_AbortProgram(const char* message)
{
	printf(message);
	exit(EXIT_FAILURE);
}