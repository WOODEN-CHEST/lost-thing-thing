#include "LttString.h"
#include "ErrorCodes.h"
#include "Memory.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Functions.
string* String_Construct()
{
	string* NewString;

	NewString = SafeMalloc(sizeof(string));

	NewString->Data = SafeMalloc(sizeof(char));
	NewString->Data = 0;
	NewString->Length = 0;

	return NewString;
}

string* String_Construct2(const char* value, const bool copyValue)
{
	if (value == NULL)
	{
		return NULL;
	}

	string* NewString = SafeMalloc(sizeof(string));
	int StringLength = strlen(value);
	int TotalStringLength = StringLength + 1;

	if (copyValue)
	{
		char* NewValue = SafeMalloc(TotalStringLength);
		strcpy_s(NewValue, TotalStringLength, value);
		NewString->Data = NewValue;
	}
	else
	{
		NewString->Data = value;
	}
	NewString->Length = StringLength;

	return NewString;
}

int String_Deconstruct(string* this)
{
	if (this == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	free(this->Data);
	free(this);

	return SUCCESS_CODE;
}

int String_IndexOf(string* this, const char character)
{
	if (this == NULL)
	{
		return -1;
	}

	for (int i = 0; i < this->Length; i++)
	{
		if (this->Data[i] == character)
		{
			return i;
		}
	}

	return -1;
}

int String_LastIndexOf(string* this, const char character)
{
	if (this == NULL)
	{
		return -1;
	}

	for (int i = this->Length - 1; i >= 0; i--)
	{
		if (this->Data[i] == character)
		{
			return i;
		}
	}

	return -1;
}

string* String_CreateCopy(string* this)
{
	if (this == NULL)
	{
		return NULL;
	}

	string* NewString = String_Construct2(this->Data, true);
	return NewString;
}

string* String_Substring(string* this, const int startIndex, const int endIndex)
{
	if ((this == NULL) || (startIndex < 0) || (endIndex < 0) || (endIndex < startIndex)
		|| (startIndex > this->Length) || (endIndex > this->Length))
	{
		return NULL;
	}

	int StringLength = endIndex - startIndex;
	int TotalStringLength = StringLength + 1;

	char* NewValue = SafeMalloc(TotalStringLength);
	for (int Cur = startIndex, New = 0; Cur < endIndex; Cur++, New++)
	{
		NewValue[New] = this->Data[Cur];
	}
	NewValue[StringLength] = '\0';

	string* NewString = String_Construct2(NewValue, false);

	return NewString;
}

bool IsCharWhitespace(char character)
{
	return (character == ' ') || (character == '\n') || (character == '\r') || (character == '\t');
}

string* String_Trim(string* this)
{
	if (this == NULL)
	{
		return NULL;
	}
	if (this->Length == 0)
	{
		return String_Construct();
	}

	int StartIndex = 0;
	while (IsCharWhitespace(this->Data[StartIndex]))
	{
		StartIndex++;
		
		if (StartIndex >= this->Length)
		{
			StartIndex--;
			break;
		}
	}

	int EndIndex = this->Length - 1;
	while ((EndIndex >= 0) && (EndIndex >= StartIndex) && IsCharWhitespace(this->Data[EndIndex]))
	{
		EndIndex--;
	}
	EndIndex++;

	return String_Substring(this, StartIndex, EndIndex);
}

string* String_ToLower(string* this)
{
	if (this == NULL)
	{
		return NULL;
	}

	string* NewString = String_CreateCopy(this);
	for (int i = 0; i < NewString->Length; i++)
	{
		NewString->Data[i] = tolower(NewString->Data[i]);
	}

	return NewString;
}

string* String_ToUpper(string* this)
{
	if (this == NULL)
	{
		return NULL;
	}

	string* NewString = String_CreateCopy(this);
	for (int i = 0; i < NewString->Length; i++)
	{
		NewString->Data[i] = toupper(NewString->Data[i]);
	}

	return NewString;
}