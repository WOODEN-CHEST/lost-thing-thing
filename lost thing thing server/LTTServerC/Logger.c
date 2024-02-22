#include "Logger.h"
#include "File.h"
#include "LTTErrors.h"
#include "Directory.h"
#include "Memory.h"
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>
#include "LTTServerC.h"


// Macros.
#define LOG_TEMP_BUFFER_SIZE 20
#define YEAR_MEASURE_START 1900
#define OLD_LOG_DIR_NAME "old"
#define LOGS_DIR_NAME "logs"
#define LOG_FILE_EXTENSION ".log"
#define NEW_LOG_FILE_NAME "latest" LOG_FILE_EXTENSION


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
	struct tm* DateTime = localtime(&CurrentTime);

	StringBuilder_AppendChar(builder, '[');

	char Year[LOG_TEMP_BUFFER_SIZE];
	sprintf_s(Year, LOG_TEMP_BUFFER_SIZE, "%i", DateTime->tm_year + YEAR_MEASURE_START);
	StringBuilder_Append(builder, Year);
	StringBuilder_Append(builder, "y;");
	
	AddTwoDigitNumber(builder, DateTime->tm_mon, 0);
	StringBuilder_Append(builder, "m;");
	AddTwoDigitNumber(builder, DateTime->tm_mday, 0);
	StringBuilder_Append(builder, "d][");

	AddTwoDigitNumber(builder, DateTime->tm_hour, ':');
	AddTwoDigitNumber(builder, DateTime->tm_min, ':');
	AddTwoDigitNumber(builder, DateTime->tm_sec, 0);
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


static char* CreateBackupLogFileName(char* oldLogDirectory, char* oldLogFilePath)
{
	// Create file name.
	StringBuilder FileNameBuilder;
	StringBuilder_Construct(&FileNameBuilder, DEFAULT_STRING_BUILDER_CAPACITY);
	StringBuilder_Append(&FileNameBuilder, oldLogDirectory);
	StringBuilder_AppendChar(&FileNameBuilder, PATH_SEPARATOR);

	struct stat FileInfo;
	stat(oldLogFilePath, &FileInfo);
	struct tm* Time = localtime(&FileInfo.st_mtime);

	char NumberBuffer[16];
	sprintf(NumberBuffer, "%d", Time->tm_year + YEAR_MEASURE_START);
	StringBuilder_Append(&FileNameBuilder, NumberBuffer);
	StringBuilder_AppendChar(&FileNameBuilder, 'y');
	AddTwoDigitNumber(&FileNameBuilder, Time->tm_mon, '\0');
	StringBuilder_AppendChar(&FileNameBuilder, 'm');
	AddTwoDigitNumber(&FileNameBuilder, Time->tm_mday, '\0');
	StringBuilder_AppendChar(&FileNameBuilder, 'd');
	StringBuilder_Append(&FileNameBuilder, " 1");
	size_t LogNumberCharIndex = FileNameBuilder.Length - 1;
	StringBuilder_Append(&FileNameBuilder, LOG_FILE_EXTENSION);

	int LogNumber = 1;

	while (File_Exists(FileNameBuilder.Data))
	{
		StringBuilder_Remove(&FileNameBuilder, LogNumberCharIndex, LogNumberCharIndex + CountDigitsInInt(LogNumber));
		LogNumber++;
		sprintf(NumberBuffer, "%d", LogNumber);
		StringBuilder_Insert(&FileNameBuilder, NumberBuffer, LogNumberCharIndex);
	}

	return FileNameBuilder.Data;
}

static void BackupLog(const char* oldLogFilePath, const char* logRootDirectoryPath)
{
	// Verify that file exists.
	if (!File_Exists(oldLogFilePath))
	{
		return;
	}

	// Backup log.
	const char* OldLogDirectory = Directory_Combine(logRootDirectoryPath, OLD_LOG_DIR_NAME);
	Directory_Create(OldLogDirectory);
	char* OldLogNewPath = CreateBackupLogFileName(OldLogDirectory, oldLogFilePath);
	MoveFileA(oldLogFilePath, OldLogNewPath);

	// Free memory.
	Memory_Free(OldLogNewPath);
	Memory_Free(OldLogDirectory);
}

// Functions.
char* Logger_ConstructContext(LoggerContext* context, const char* rootDirectoryPath)
{
	// Verify state and args.
	if (context->LogFile != NULL)
	{
		return "Logger already initialized.";
	}

	// Create directories.
	char* LogDirPath = Directory_Combine(rootDirectoryPath, LOGS_DIR_NAME);
	Directory_CreateAll(LogDirPath);

	// Backup old logs.
	const char* LogFilePath = Directory_Combine(LogDirPath, NEW_LOG_FILE_NAME);
	BackupLog(LogFilePath, LogDirPath);

	// Create current log  file.
	File_Delete(LogFilePath);
	context->LogFile = File_Open(LogFilePath, FileOpenMode_Write);

	if (context->LogFile == NULL)
	{
		return "IO Error creating logger file.";
	}

	StringBuilder_Construct(&context->_logTextBuilder, DEFAULT_STRING_BUILDER_CAPACITY);


	// Free memory.
	Memory_Free(LogFilePath);
	Memory_Free(LogDirPath);

	return NULL;
}


ErrorCode Logger_Close()
{
	LoggerContext* Context = &LTTServerC_GetCurrentContext()->Logger;
	if (Context->LogFile == NULL)
	{
		return ErrorContext_SetError(ErrorCode_IllegalOperation, "Cannot close logger since it isnt initialized yet.");
	}

	File_Close(Context->LogFile);
	StringBuilder_Deconstruct(&Context->_logTextBuilder);

	return ErrorCode_Success;
}

ErrorCode Logger_Log(Logger_LogLevel level, const char* string)
{
	LoggerContext* Context = &LTTServerC_GetCurrentContext()->Logger;

	if (Context->LogFile == NULL)
	{
		return ErrorContext_SetError(ErrorCode_IllegalOperation, "Cannot log, logger not initialized.");
	}

	AddDateTime(&Context->_logTextBuilder);
	AddLevel(&Context->_logTextBuilder, level);
	StringBuilder_AppendChar(&Context->_logTextBuilder, ' ');
	StringBuilder_Append(&Context->_logTextBuilder, string);
	StringBuilder_AppendChar(&Context->_logTextBuilder, '\n');

	File_WriteText(Context->LogFile, Context->_logTextBuilder.Data);
	printf(Context->_logTextBuilder.Data);

	StringBuilder_Clear(&Context->_logTextBuilder);

	return ErrorCode_Success;
}

ErrorCode Logger_LogInfo(const char* string)
{
	return Logger_Log(LogLevel_Info, string);
}

ErrorCode Logger_LogWarning(const char* string)
{
	return Logger_Log(LogLevel_Warning, string);
}

ErrorCode Logger_LogError(const char* string)
{
	return Logger_Log(LogLevel_Error, string);
}

ErrorCode Logger_LogCritical(const char* string)
{
	return Logger_Log(LogLevel_Critical, string);
}