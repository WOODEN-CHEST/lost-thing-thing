#include <stdbool.h>
#include "LttCommon.h"
#include "File.h"
#include <stdio.h>
#include "HttpListener.h"

// Static functions.


// Functions.
int main()
{
	// Initialize components.
	Error_InitErrorHandling();
	Logger_Initialize("C:/Users/KČerņavskis/Desktop/logs");
	Logger_LogInfo("Starting server...");

	// Start server.
	HttpListener_MainLoop();


	// Stop server.
	Logger_Close();

	return 0;
}