#include <stdbool.h>
#include "LttCommon.h"
#include "File.h"
#include <stdio.h>
#include "HttpListener.h"
#include <signal.h>
#include <Stdlib.h>

// Static functions.
static void Close(int sig)
{
	Logger_LogInfo("Server closed.");
	Logger_Close();
	exit(0);
}

// Functions.
int main()
{
	// Initialize components.
	Error_InitErrorHandling();
	Logger_Initialize("C:/Users/KČerņavskis/Desktop/logs");
	Logger_LogInfo("Starting server...");

	signal(SIGINT, &Close);
	signal(SIGTERM, &Close);

	// Start server.
	ErrorCode Error = HttpListener_MainLoop();
	if (Error != ErrorCode_Success)
	{
		Logger_LogInfo(Error_GetLastErrorMessage());
	}

	// Stop server.
	Close(SIGTERM);

	return 0;
}