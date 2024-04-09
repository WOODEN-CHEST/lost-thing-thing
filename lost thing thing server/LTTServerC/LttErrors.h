#pragma once

// Types.
typedef enum ErrorCodeEnum
{
	ErrorCode_Success,

	ErrorCode_NullReference,
	ErrorCode_InvalidArgument,
	ErrorCode_ArgumentOutOfRange,
	ErrorCode_IllegalOperation,
	ErrorCode_IllegalState,
	ErrorCode_IO,
	ErrorCode_IndexOutOfRange,
	ErrorCode_SocketError,
	ErrorCode_InvalidGHDFFile,
	ErrorCode_DatabaseError,
	ErrorCode_InvalidRequest,
	ErrorCode_InvalidConfigFile,

	ErrorCode_Unknown
} ErrorCode;

typedef struct ErrorStruct
{
	ErrorCode Code;
	const char* Message;
} Error;


// Functions.

void Error_AbortProgram(const char* message);

Error Error_CreateError(ErrorCode code, const char* message);

Error Error_CreateSuccess();

void Error_Deconstruct(Error* error);