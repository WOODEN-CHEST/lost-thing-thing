#include <stdbool.h>
#include "LttCommon.h"
#include "File.h"
#include <stdio.h>

int main()
{
	Logger_Initialize("C:\\Users\\User\\Desktop\\logs\\log.txt");

	Logger_LogInfo("Info text");
	Logger_LogWarning("Warning texts");
	Logger_LogError("Error!");
	Logger_LogCritical("Critical ops.");
	Logger_LogInfo("Shutting down.");

	Logger_Close();

	return 0;
}