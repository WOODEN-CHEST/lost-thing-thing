#pragma once
#include <stdbool.h>

// Structures.
struct StringBuilderStruct
{
	char* Data;
	int Length;
	int _capacity;
};

typedef struct StringBuilderStruct StringBuilder;

// Functions.
// String.
int String_IndexOf(char* string, const char character);

int String_LastIndexOf(char* string, const char character);

char* String_CreateCopy(char* string);

char* String_Substring(char* string, const int startIndex, const int endIndex);

char* String_Trim(char* string);

char* String_ToLower(char* string);

char* String_ToUpper(char* string);


// String builder.