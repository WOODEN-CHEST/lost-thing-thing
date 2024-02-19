#include <stdbool.h>
#include "LttCommon.h"
#include "File.h"
#include <stdio.h>
#include "HttpListener.h"
#include <signal.h>
#include <Stdlib.h>
#include "ConfigFile.h"
#include "Directory.h"
#include "LTTHTML.h"

// Static functions.
static void Close(int sig)
{
	Logger_LogInfo("Server closed.");
	Logger_Close();
	exit(0);
}

// Functions.
int main(int argc, const char** argv)
{
	// Initialize components.
	Error_InitErrorHandling();
	Logger_Initialize("C:/Users/KČerņavskis/Desktop/logs");
	Logger_LogInfo("Starting server...");

	signal(SIGINT, &Close);
	signal(SIGTERM, &Close);


	// Load config.
	char* ServerPath = argv[0];
	ServerConfig Config;
	char* ConfigPath = Directory_Combine(ServerPath, SERVER_CONFIG_FILE_NAME);
	ServerConfig_Read(ConfigPath, &Config);

	HTMLDocument Document;
	HTMLDocument_Construct(&Document);

	HTMLElement* Para = HTMLElement_AddElement(Document.Body, "p");
	HTMLElement_SetAttribute(Para, "id", "MyID");
	HTMLElement_SetAttribute(Document.Body, "onClick", "myFunc(this)");

	char* HTML = HTMLDocument_ToString(&Document);

	HTMLElement* Element = HTMLDocument_GetElementByID(&Document, "MyID");



	// Start server.
	//ErrorCode Error = HttpListener_Listen();
	//if (Error != ErrorCode_Success)
	//{
	//	Logger_LogInfo(Error_GetLastErrorMessage());
	//}

	// Stop server.
	Close(SIGTERM);

	return 0;
}