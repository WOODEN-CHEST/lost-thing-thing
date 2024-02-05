#include "LttErrors.h"
#include "Logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// Macros.
#define ERROR_MESSAGE_DEFAULT_CAPACITY 512
#define ERROR_MESSAGE_CAPACITY_GROWTH 2


// Variables.
static ErrorCode LastErrorCode;
static char* LastErrorMessage;
static size_t LastErrorMessageCapacity = 512;


// Functions.
void Error_InitErrorHandling()
{
	LastErrorCode = ErrorCode_Success;
	LastErrorMessageCapacity = ERROR_MESSAGE_DEFAULT_CAPACITY;

	LastErrorMessage = malloc(LastErrorMessageCapacity);
	if (!LastErrorMessage)
	{
		Error_AbortProgram("Failed to initialize error handling.");
		return;
	}

	*LastErrorMessage = '\0';
}

void Error_AbortProgram(const char* message)
{
	if (Logger_IsInitialized())
	{
		Logger_LogCritical(message);
	}

	exit(EXIT_FAILURE);
}

ErrorCode Error_SetError(int code, const char* message)
{
	if ((code < ErrorCode_Success) || (code > ErrorCode_Unknown))
	{
		code = ErrorCode_Unknown;
	}

	LastErrorCode = code;

	char ErrorName[128];
	switch (code)
	{
		case ErrorCode_NullReference:
			strcpy(ErrorName, "Null Reference Error. ");
			break;

		case ErrorCode_InvalidArgument:
			strcpy(ErrorName, "Invalid Argument Error. ");
			break;

		case ErrorCode_ArgumentOutOfRange:
			strcpy(ErrorName, "Argument Out Of Range Error. ");
			break;

		case ErrorCode_IllegalOperation:
			strcpy(ErrorName, "Illegal Operation Error. ");
			break;

		case ErrorCode_IllegalState:
			strcpy(ErrorName, "Illegal State Error. ");
			break;

		case ErrorCode_IO:
			strcpy(ErrorName, "Input Output Error. ");
			break;

		case ErrorCode_IndexOutOfRange:
			strcpy(ErrorName, "Index Out of Range Error. ");
			break;

		case ErrorCode_SocketError:
			strcpy(ErrorName, "Socket Error. ");
			break;
		
		default:
			strcpy(ErrorName, "Unknown Error. ");
			break;
	}

	size_t ErrorTypeMessageLength = strlen(ErrorName);
	size_t MessageLength = strlen(message);
	size_t RequiredMemory = ErrorTypeMessageLength + MessageLength + 1;

	if (LastErrorMessageCapacity < RequiredMemory)
	{
		do
		{
			LastErrorMessageCapacity *= ERROR_MESSAGE_CAPACITY_GROWTH;
		} while (LastErrorMessageCapacity < RequiredMemory);

		void* NewPointer = realloc(LastErrorMessage, LastErrorMessageCapacity);
		if (!NewPointer)
		{
			free(LastErrorMessage);
			Error_AbortProgram("Error handler failed to reallocate memory.");
			return;
		}
		LastErrorMessage = NewPointer;
	}

	strcpy(LastErrorMessage, ErrorName);
	strcpy(LastErrorMessage + ErrorTypeMessageLength, message);
}

ErrorCode Error_GetLastErrorCode()
{
	return LastErrorCode;
}

const char* Error_GetLastErrorMessage()
{
	return LastErrorMessage;
}