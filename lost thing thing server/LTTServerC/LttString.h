#pragma once
#include <stdbool.h>

// Structures.
struct String
{
	char* Data;
	int Length;
};

typedef struct String string;

struct StringBuilderStruct
{
	char* Data;
	int Length;
	int _capacity;
};

typedef struct StringBuilderStruct StringBuilder;

// Functions.
// String.
string* String_Construct();

string* String_Construct2(const char* value, const bool copyValue);

int String_Deconstruct(string* this);

int String_IndexOf(string* this, const char character);

int String_LastIndexOf(string* this, const char character);

string* String_CreateCopy(string* this);

string* String_Substring(string* this, const int startIndex, const int endIndex);

string* String_Trim(string* this);

string* String_ToLower(string* this);

string* String_ToUpper(string* this);


// String builder.