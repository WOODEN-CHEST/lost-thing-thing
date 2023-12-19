#include "Logger.h"
#include "File.h"
#include "ErrorCodes.h"
#include "Directory.h"
#include "Memory.h"
#include <stdbool.h>
#include <time.h>

#define LOG_TEMP_BUFFER_SIZE 20
#define YEAR_MEASURE_START 1900

// Variables.
static FILE* LogFile = NULL;
static StringBuilder TextBuilder;


// Static functions.
static void Logger_AddTwoDigitNumber(StringBuilder* builder, int number, char separator)
{
	if (number < 10)
	{
		StringBuilder_AppendChar(builder, '0');
	}

	char FormattedData[LOG_TEMP_BUFFER_SIZE];

	sprintf_s(FormattedData, LOG_TEMP_BUFFER_SIZE, "%i", number);
	StringBuilder_Append(builder, FormattedData);

	if (separator != 0)
	{
		StringBuilder_AppendChar(builder, separator);
	}
}

static void Logger_AddDateTime(StringBuilder* builder)
{
	time_t CurrentTime = time(NULL);
	struct tm DateTime;
	localtime_s(&DateTime, &CurrentTime);

	StringBuilder_AppendChar(builder, '[');

	char Year[LOG_TEMP_BUFFER_SIZE];
	sprintf_s(Year, LOG_TEMP_BUFFER_SIZE, "%i", DateTime.tm_year + YEAR_MEASURE_START);
	StringBuilder_Append(builder, Year);
	StringBuilder_Append(builder, "y;");
	
	Logger_AddTwoDigitNumber(builder, DateTime.tm_mon, 0);
	StringBuilder_Append(builder, "m;");
	Logger_AddTwoDigitNumber(builder, DateTime.tm_mday, 0);
	StringBuilder_Append(builder, "d][");

	Logger_AddTwoDigitNumber(builder, DateTime.tm_hour, ':');
	Logger_AddTwoDigitNumber(builder, DateTime.tm_min, ':');
	Logger_AddTwoDigitNumber(builder, DateTime.tm_sec, 0);
	StringBuilder_AppendChar(builder, ']');
}

static void Logger_AddLevel(StringBuilder* builder, Logger_LogLevel level)
{
	StringBuilder_AppendChar(builder, '[');

	switch (level)
	{
		case Info:
			StringBuilder_Append(builder, "Info");
			break;
		case Warning:
			StringBuilder_Append(builder, "Warning");
			break;
		case Error:
			StringBuilder_Append(builder, "Error");
			break;
		case Critical:
			StringBuilder_Append(builder, "CRITICAL");
			break;
		default:
			StringBuilder_Append(builder, "Unknown severity");
	}

	StringBuilder_AppendChar(builder, ']');
}

// Functions.
int Logger_Initialize(char* path)
{
	if (LogFile != NULL)
	{
		return ILLEGAL_OPERATION_ERRCODE;
	}
	if (path == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	// Create directories.
	Directory_CreateAll(path);

	StringBuilder Builder;
	StringBuilder_Construct(&Builder, DEFAULT_STRING_BUILDER_CAPACITY);
	StringBuilder_Append(&Builder, path);
	StringBuilder_AppendChar(&Builder, PathSeparator);
	StringBuilder_Append(&Builder, "old");
	


	// Backup old logs.



	File_Delete(path);
	LogFile = File_Open(path, Write);

	if (LogFile == NULL)
	{
		return IO_ERROR_ERRCODE;
	}

	StringBuilder_Construct(&TextBuilder, DEFAULT_STRING_BUILDER_CAPACITY);

	return 0;
}

int Logger_Close()
{
	if (LogFile == NULL)
	{
		return ILLEGAL_OPERATION_ERRCODE;
	}

	File_Close(LogFile);
	StringBuilder_Deconstruct(&TextBuilder);
}

int Logger_Log(Logger_LogLevel level, char* string)
{
	if (LogFile == NULL)
	{
		return ILLEGAL_OPERATION_ERRCODE;
	}

	Logger_AddDateTime(&TextBuilder);
	Logger_AddLevel(&TextBuilder, level);
	StringBuilder_AppendChar(&TextBuilder, ' ');
	StringBuilder_Append(&TextBuilder, string);
	StringBuilder_AppendChar(&TextBuilder, '\n');

	File_WriteString(LogFile, TextBuilder.Data);

	StringBuilder_Clear(&TextBuilder);
}

int Logger_LogInfo(char* string)
{
	return Logger_Log(Info, string);
}

int Logger_LogWarning(char* string)
{
	return Logger_Log(Warning, string);
}

int Logger_LogError(char* string)
{
	return Logger_Log(Error, string);
}

int Logger_LogCritical(char* string)
{
	return Logger_Log(Critical, string);
}