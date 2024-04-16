#pragma once
#include "LTTErrors.h"



// Types.
typedef struct SMTPCredentialsStruct
{
	char Email[256];
	char Password[128];
} SMPTCredentials;


// Functions.
Error SMTP_SendEmail(SMPTCredentials* credentials, const char* targetEmail, const char* subject, const char* body);