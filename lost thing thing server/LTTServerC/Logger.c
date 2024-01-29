#include "Logger.h"
#include "File.h"
#include "LTTErrors.h"
#include "Directory.h"
#include "Memory.h"
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>
#include <Windows.h>


// Macros.
#define LOG_TEMP_BUFFER_SIZE 20
#define YEAR_MEASURE_START 1900
#define OLD_LOG_DIR_NAME "old"
#define LOG_FILE_EXTENSION ".log"
#define NEW_LOG_FILE_NAME "latest" LOG_FILE_EXTENSION


// Variables.
static FILE* _logFile = NULL;
static StringBuilder _textBuilder;


// Static functions.
static void AddTwoDigitNumber(StringBuilder* builder, int number, char separator)
{
	if (number < 10)
	{
		StringBuilder_AppendChar(builder, '0');
	}

	char FormattedData[LOG_TEMP_BUFFER_SIZE];

	sprintf(FormattedData, "%i", number);
	StringBuilder_Append(builder, FormattedData);

	if (separator != 0)
	{
		StringBuilder_AppendChar(builder, separator);
	}
}

static void AddDateTime(StringBuilder* builder)
{
	time_t CurrentTime = time(NULL);
	struct tm DateTime;
	localtime(&DateTime, &CurrentTime);

	StringBuilder_AppendChar(builder, '[');

	char Year[LOG_TEMP_BUFFER_SIZE];
	sprintf_s(Year, LOG_TEMP_BUFFER_SIZE, "%i", DateTime.tm_year + YEAR_MEASURE_START);
	StringBuilder_Append(builder, Year);
	StringBuilder_Append(builder, "y;");
	
	AddTwoDigitNumber(builder, DateTime.tm_mon, 0);
	StringBuilder_Append(builder, "m;");
	AddTwoDigitNumber(builder, DateTime.tm_mday, 0);
	StringBuilder_Append(builder, "d][");

	AddTwoDigitNumber(builder, DateTime.tm_hour, ':');
	AddTwoDigitNumber(builder, DateTime.tm_min, ':');
	AddTwoDigitNumber(builder, DateTime.tm_sec, 0);
	StringBuilder_AppendChar(builder, ']');
}

static void AddLevel(StringBuilder* builder, Logger_LogLevel level)
{
	StringBuilder_AppendChar(builder, '[');

	switch (level)
	{
		case LogLevel_Info:
			StringBuilder_Append(builder, "Info");
			break;
		case LogLevel_Warning:
			StringBuilder_Append(builder, "Warning");
			break;
		case LogLevel_Error:
			StringBuilder_Append(builder, "Error");
			break;
		case LogLevel_Critical:
			StringBuilder_Append(builder, "CRITICAL");
			break;
		default:
			StringBuilder_Append(builder, "Unknown severity");
	}

	StringBuilder_AppendChar(builder, ']');
}

static int CountDigitsInInt(int number)
{
	int Digits = 0;

	do
	{
		number /= 10;
		Digits++;
	} while (number > 0);

	return Digits;
}

static void BackupLog(const char* oldLogFilePath, const char* logRootDirectoryPath)
{
	// Verify that file exists.
	if (!File_Exists(oldLogFilePath))
	{
		return;
	}

	// Create backup directory.
	const char* OldLogDirectory = Directory_Combine(logRootDirectoryPath, OLD_LOG_DIR_NAME);
	Directory_Create(OldLogDirectory);


	// Create file name.
	StringBuilder FileNameBuilder;
	StringBuilder_Construct(&FileNameBuilder, DEFAULT_STRING_BUILDER_CAPACITY);
	StringBuilder_Append(&FileNameBuilder, OldLogDirectory);
	StringBuilder_AppendChar(&FileNameBuilder, PATH_SEPARATOR);

	struct stat FileInfo;
	stat(oldLogFilePath, &FileInfo);
	struct tm Time;
	localtime(&FileInfo.st_mtime);

	char NumberBuffer[16];
	sprintf(NumberBuffer, "%d", Time.tm_year + YEAR_MEASURE_START);
	StringBuilder_Append(&FileNameBuilder, NumberBuffer);
	StringBuilder_AppendChar(&FileNameBuilder, 'y');
	AddTwoDigitNumber(&FileNameBuilder, Time.tm_mon + YEAR_MEASURE_START, '\0');
	StringBuilder_AppendChar(&FileNameBuilder, 'm');
	AddTwoDigitNumber(&FileNameBuilder, Time.tm_mday + YEAR_MEASURE_START, '\0');
	StringBuilder_AppendChar(&FileNameBuilder, 'd');
	StringBuilder_Append(&FileNameBuilder, " 1");
	size_t LogNumberCharIndex = FileNameBuilder.Length - 1;
	StringBuilder_Append(&FileNameBuilder, LOG_FILE_EXTENSION);

	int LogNumber = 1;

	while (File_Exists(FileNameBuilder.Data))
	{
		StringBuilder_Remove(&FileNameBuilder, LogNumberCharIndex, CountDigitsInInt(LogNumber));
		LogNumber++;
		sprintf(NumberBuffer, "%d", LogNumber);
		StringBuilder_Insert(&FileNameBuilder, NumberBuffer, LogNumberCharIndex);
	}

	// Backup log.
	MoveFileA(oldLogFilePath, FileNameBuilder.Data);

	// Free memory.
	StringBuilder_Deconstruct(&FileNameBuilder);
	Memory_Free(OldLogDirectory);
}

// Functions.
ErrorCode Logger_Initialize(const char* rootDirectoryPath)
{
	// Verify state and args.
	if (_logFile != NULL)
	{
		return Error_SetError(ErrorCode_IllegalOperation, "Logger alread initialized.");
	}

	// Create directories.
	Directory_CreateAll(rootDirectoryPath);

	// Backup old logs.
	const char* LogFilePath = Directory_Combine(rootDirectoryPath, NEW_LOG_FILE_NAME);
	BackupLog(LogFilePath, rootDirectoryPath);

	// Create current log  file.
	File_Delete(LogFilePath);
	_logFile = File_Open(rootDirectoryPath, FileOpenMode_Write);

	if (_logFile == NULL)
	{
		return Error_SetError(ErrorCode_IO, "Failed to create log file.");
	}

	StringBuilder_Construct(&_textBuilder, DEFAULT_STRING_BUILDER_CAPACITY);


	// Free memory.
	Memory_Free(LogFilePath);

	return ErrorCode_Success;
}

_Bool Logger_IsInitialized()
{
	return _logFile != NULL;
}

ErrorCode Logger_Close()
{
	if (_logFile == NULL)
	{
		return Error_SetError(ErrorCode_IllegalOperation, "Cannot close logger since it isnt initialized yet.");
	}

	File_Close(_logFile);
	StringBuilder_Deconstruct(&_textBuilder);

	return ErrorCode_Success;
}

ErrorCode Logger_Log(Logger_LogLevel level, char* string)
{
	if (_logFile == NULL)
	{
		return Error_SetError(ErrorCode_IllegalOperation, "Cannot log, logger not initialized.");
	}

	AddDateTime(&_textBuilder);
	AddLevel(&_textBuilder, level);
	StringBuilder_AppendChar(&_textBuilder, ' ');
	StringBuilder_Append(&_textBuilder, string);
	StringBuilder_AppendChar(&_textBuilder, '\n');

	File_WriteString(_logFile, _textBuilder.Data);

	StringBuilder_Clear(&_textBuilder);

	return ErrorCode_Success;
}

ErrorCode Logger_LogInfo(char* string)
{
	return Logger_Log(LogLevel_Info, string);
}

ErrorCode Logger_LogWarning(char* string)
{
	return Logger_Log(LogLevel_Warning, string);
}

ErrorCode Logger_LogError(char* string)
{
	return Logger_Log(LogLevel_Error, string);
}

ErrorCode Logger_LogCritical(char* string)
{
	return Logger_Log(LogLevel_Critical, string);
}