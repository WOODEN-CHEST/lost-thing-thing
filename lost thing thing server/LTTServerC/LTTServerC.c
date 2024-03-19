#include <stdbool.h>
#include "LttCommon.h"
#include "File.h"
#include <stdio.h>
#include "HttpListener.h"
#include <signal.h>
#include <Stdlib.h>
#include "ConfigFile.h"
#include "Directory.h"
#include "Memory.h"
#include "LTTServerC.h"
#include "Logger.h"
#include "GHDF.h"
#include "LTTChar.h"


// Static variables.
static ServerContext s_currentContext;


// Static functions.
static char* CreateContext(ServerContext* context, const char* serverExecutablePath)
{
	ErrorContext_Construct(&context->Errors);
	char* RootDirectory = Directory_GetParentDirectory(serverExecutablePath);
	context->RootPath = RootDirectory;

	const char* ReturnErrorMessage = Logger_ConstructContext(&context->Logger, RootDirectory);
	if (ReturnErrorMessage)
	{
		return ReturnErrorMessage;
	}

	ResourceManager_ConstructContext(&context->Resources, RootDirectory);

	return NULL;
}

// Functions.
int main(int argc, const char** argv)
{
	// Create context.
	const char* ReturnErrorMessage = CreateContext(&s_currentContext, argv[0]);
	if (ReturnErrorMessage)
	{
		printf("Failed to create server context: %s", ReturnErrorMessage);
		return EXIT_FAILURE;
	}
	Logger_LogInfo("Created global server context");


	// Load config.
	ServerConfig Config;
	char* ConfigPath = Directory_Combine(LTTServerC_GetCurrentContext()->RootPath, SERVER_CONFIG_FILE_NAME);
	ServerConfig_Read(ConfigPath, &Config);
	Logger_LogInfo("Read config");


	//GHDFCompound Compound;
	//GHDFCompound_Construct(&Compound, COMPOUND_DEFAULT_CAPACITY);
	//GHDFCompound NestedCompound;
	//GHDFCompound_Construct(&NestedCompound, COMPOUND_DEFAULT_CAPACITY);

	//GHDFPrimitive Value;

	//Value.Float = 3.5f;
	//GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_Float, 1, Value);

	//Value.Bool = false;
	//GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_Bool, 2, Value);

	//Value.Double = 4.0;
	//GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_Double, 3, Value);

	//Value.Int = 92;
	//GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_Int, 4, Value);

	//Value.String = "Hello, String!";
	//GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_String, 5, Value);

	//Value.Short = 32000;
	//GHDFCompound_AddSingleValueEntry(&NestedCompound, GHDFType_Short, 1, Value);
	//Value.SByte = 'h';
	//GHDFCompound_AddSingleValueEntry(&NestedCompound, GHDFType_SByte, 2, Value);
	//Value.Compound = &NestedCompound;
	//GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_Compound, 6, Value);

	//GHDFPrimitive IntArray[5];
	//IntArray[0].Int = 1;
	//IntArray[1].Int = 2;
	//IntArray[2].Int = 3;
	//IntArray[3].Int = 4;
	//IntArray[4].Int = 5;

	//GHDFCompound_AddArrayEntry(&Compound, GHDFType_Float, 7, IntArray, 5);

	//for (size_t i = 0; i < Compound.Count; i++)
	//{
	//	GHDFEntry* Entry = Compound.Entries + i;
	//	int c = 3;
	//}

	//GHDFCompound_Deconstruct(&Compound);

	FILE* File = File_Open("C:\\Users\\KČerņavskis\\Desktop\\text.txt", FileOpenMode_Read);
	const char* Text = File_ReadAllText(File);
	File_Close(File);

	for (int i = 0; i < String_LengthBytes(Text);)
	{
		int Codepoint = Char_GetCodepoint(Text + i);
		i += Char_GetByteCount(Text + i);
	}

	// Start server.
	ErrorCode Error = HttpListener_Listen();
	if (Error != ErrorCode_Success)
	{
		Logger_LogInfo(Error_GetLastErrorMessage());
	}

	// Stop server.
	Logger_LogInfo("Server closed.");
	Logger_Close();
	Memory_Free(LTTServerC_GetCurrentContext()->RootPath);
	ResourceManager_CloseContext();

	return 0;
}

ServerContext* LTTServerC_GetCurrentContext()
{
	return &s_currentContext;
}