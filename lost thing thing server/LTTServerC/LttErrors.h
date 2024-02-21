#pragma once

// Types.
enum ErrorCodeEnum
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

	ErrorCode_Unknown
};

typedef enum ErrorCodeEnum ErrorCode;

typedef struct ErrorContextStruct
{
	ErrorCode _lastErrorCode;
	char* _lastErrorMessage;
	size_t _lastErrorMessageCapacity;
} ErrorContext;


// Functions.

/// <summary>
/// Initializes error handling for the program, aborts the program if it fails to do so.
/// </summary>
void ErrorContext_Construct(ErrorContext* context);

/// <summary>
/// Immediately closes the program, sends the message to stdout and attempts to log it.
/// Should only be used in absolutely critical cases because this function does close gracefully.
/// </summary>
/// <param name="message">The message to write. May be null to indicate no message.</param>
void ErrorContext_AbortProgram(const char* message);

/// <summary>
/// Sets the last error code and message.
/// </summary>
/// <param name="code">The error code. Must be a valid code from ErrorCode enum. Set to ErrorCode_Unknown if outside the range.</param>
/// <param name="message">Error message. May be null to indicate no message.</param>
/// <returns>The same error code passed as an argument to this function for easier use in returns.</returns>
ErrorCode ErrorContext_SetError(int code, const char* message);

/// <summary>
/// Retrieves the last error code.
/// </summary>
/// <returns>Returns the last error code</returns>
ErrorCode ErrorContext_GetLastErrorCode();


/// <summary>
/// Gets the last error message.
/// </summary>
/// <returns>A pointer to the error message character array. Modifying the data pointed to may cause undefined behavior.</returns>
const char* Error_GetLastErrorMessage();