#pragma once
#include <stdbool.h>
#include "ArrayList.h"

#define DEFAULT_STRING_BUILDER_CAPACITY 128

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

/// <summary>
/// Returns the amount of UTF8 code-points in the string, excluding the null terminator.
/// A badly formatted  string may result in buffer over-read.
/// </summary>
int String_LengthCodepointsUTF8(char* string);

/// <summary>
/// Returns the amount of bytes in the provided string, excluding the null terminator.
/// </summary>
int String_LengthBytes(char* string);

int String_ByteIndexOf(char* string, char* sequence);

int String_LastByteIndexOf(char* string, char* sequence);

/// <summary>
/// Creates a copy of the string.
/// </summary>
/// <param name="string">The string to copy.</param>
/// <returns>A pointer to the copied string.</returns>
char* String_CreateCopy(char* string);

char* String_SubString(char* string, const int startIndex, const int endIndex);

/// <summary>
/// Creates a trimmed copy of the string. Trims whitespace characters '\n', '\t', ' ' and '\r'.
/// </summary>
/// <param name="string">A null terminated string.</param>
/// <returns>A pointer to the trimmed string.</returns>
char* String_Trim(char* string);

/// <summary>
/// Only supports Latvian and English alphabet.
/// </summary>
int String_ToLowerUTF8(char* string);

/// <summary>
/// Only supports Latvian and English alphabet.
/// </summary>
int String_ToUpperUTF8(char* string);

int String_Count(char* string, char* sequence);

bool String_Contains(char* string, char* sequence);

bool String_StartsWith(char* string, char* sequence);

bool String_EndsWith(char* string, char* sequence);

bool String_Equals(char* string1, char* string2);

char* String_Replace(char* string, char* oldSequence, char* newSequence);

int String_Split(char* string, char* sequence, ArrayList* listOfNewStrings);


// StringBuilder
int StringBuilder_Construct(StringBuilder* builder, int capacity);

StringBuilder* StringBuilder_Construct2(int capacity);

int StringBuilder_Append(StringBuilder* this, char* string);

int StringBuilder_AppendChar(StringBuilder* this, char character);

int StringBuilder_Insert(StringBuilder* this, char* string, int byteIndex);

int StringBuilder_InsertChar(StringBuilder* this, char character, int byteIndex);

int StringBuilder_Remove(StringBuilder* this, int startIndex, int endIndex);

int StringBuilder_Clear(StringBuilder* this);

int StringBuilder_Deconstruct(StringBuilder* this);