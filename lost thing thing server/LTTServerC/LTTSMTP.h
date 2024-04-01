#pragma once
#include "LTTErrors.h"


// Types.
typedef struct SMTPCredentialsStruct
{
	char Email[256];
	char Password[256];
} SMPTCredentials;


// Functions.
ErrorCode SMTP_SendEmail(SMPTCredentials* credentials, const char* targetEmail, const char* subject, const char* body);