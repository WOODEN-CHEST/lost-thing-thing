#include "LttErrors.h"
#include "Logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "LTTServerC.h"


// Macros.
#define ERROR_MESSAGE_DEFAULT_CAPACITY 512
#define ERROR_MESSAGE_CAPACITY_GROWTH 2


// Variables.


// Static functions.
static void CopyErrorNameIntoBuffer(int code, char* buffer)
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

static void EnsureMessageBufferCapacity(size_t capacity)
{
	ErrorContext* Context = &LTTServerC_GetCurrentContext()->Errors;
	if (Context->_lastErrorMessageCapacity < capacity)
	{
		do
		{
			Context->_lastErrorMessageCapacity *= ERROR_MESSAGE_CAPACITY_GROWTH;
		} while (Context->_lastErrorMessageCapacity < capacity);

		void* NewPointer = realloc(Context->_lastErrorMessage , Context->_lastErrorMessageCapacity);
		if (!NewPointer)
		{
			free(Context->_lastErrorMessage);
			Error_AbortProgram("Error handler failed to reallocate memory.");
			return;
		}
		Context->_lastErrorMessage = (char*)NewPointer;
	}
}


// Functions.
void Error_ConstructContext(ErrorContext* context)
{
	context->_lastErrorCode = ErrorCode_Success;
	context->_lastErrorMessageCapacity = ERROR_MESSAGE_DEFAULT_CAPACITY;

	context->_lastErrorMessage = (char*)malloc(context->_lastErrorMessageCapacity);
	if (!context->_lastErrorMessage)
	{
		Error_AbortProgram("Failed to initialize error handling.");
		return;
	}

	context->_lastErrorMessage[0] = '\0';
}

void Error_CloseContext(ErrorContext* context)
{
	free(context->_lastErrorMessage);
}

void Error_AbortProgram(const char* message)
{
	printf(message);
	exit(EXIT_FAILURE);
}

ErrorCode Error_SetError(int code, const char* message)
{
	if ((code < ErrorCode_Success) || (code > ErrorCode_Unknown))
	{
		code = ErrorCode_Unknown;
	}

	ErrorContext* Context = &LTTServerC_GetCurrentContext()->Errors;
	Context->_lastErrorCode = code;

	char ErrorName[128];
	CopyErrorNameIntoBuffer(code, ErrorName);

	size_t ErrorTypeMessageLength = strlen(ErrorName);
	size_t MessageLength = strlen(message);
	size_t RequiredMemory = ErrorTypeMessageLength + MessageLength + 1;
	EnsureMessageBufferCapacity(RequiredMemory);

	strcpy(Context->_lastErrorMessage, ErrorName);
	strcpy(Context->_lastErrorMessage + ErrorTypeMessageLength, message);
}

ErrorCode Error_GetLastErrorCode()
{
	return LTTServerC_GetCurrentContext()->Errors._lastErrorCode;
}

const char* Error_GetLastErrorMessage()
{
	return LTTServerC_GetCurrentContext()->Errors._lastErrorMessage;
}

void Error_ClearError()
{
	LTTServerC_GetCurrentContext()->Errors._lastErrorCode = ErrorCode_Success;
	LTTServerC_GetCurrentContext()->Errors._lastErrorMessage[0] = '\0';
}