#pragma once
#include "ArrayList.h"
#include "LttErrors.h"

#define DEFAULT_STRING_BUILDER_CAPACITY 128

// Types.
struct StringBuilderStruct
{
	char* Data;
	int Length;
	int _capacity;
};

typedef struct StringBuilderStruct StringBuilder;


// Functions.
/* String */
/// <summary>
/// Counts the amount of UTF8 code-points in the string, excluding the null terminator.
/// A badly formatted string may result in buffer over-read.
/// </summary>
/// <returns>The amount of UTF-8 code-points in the string.</returns>
size_t String_LengthCodepointsUTF8(const char* string);

/// <summary>
/// Returns the amount of bytes in the provided string, excluding the null terminator.
/// </summary>
size_t String_LengthBytes(const char* string);

/// <summary>
/// Searches for the char index of the first sequence occurrence in the string.
/// </summary>
/// <param name="string">The string to search in.</param>
/// <param name="sequence">The character sequence to search for.</param>
/// <returns>The char index of the first occurrence of the sequence in the string.</returns>
int String_CharIndexOf(const char* string, const char* sequence);

/// <summary>
/// Searches for the char index of the last sequence occurrence in the string.
/// </summary>
/// <param name="string">The string to search in.</param>
/// <param name="sequence">The character sequence to search for.</param>
/// <returns>The char index of the last occurrence of the sequence in the string.</returns>
int String_LastCharIndexOf(const char* string, const char* sequence);

/// <summary>
/// Creates a copy of the string.
/// </summary>
/// <param name="string">The string to copy.</param>
/// <returns>A pointer to the copied string.</returns>
char* String_CreateCopy(const char* string);

/// <summary>
/// Copies the contents of the source string into the destination string.
/// </summary>
/// <param name="source">The source string from which to take characters.</param>
/// <param name="destination">The destination string into which the characters are copied.</param>
/// <returns>Error code.</returns>
void String_CopyTo(const char* sourceString, char* destination);

/// <summary>
/// Creates a substring from the provided string using the start index (inclusive) and end index (exclusive).
/// </summary>
/// <param name="string">The string from which to sample the substring.</param>
/// <param name="startIndex">The start index in the string (inclusive)</param>
/// <param name="endIndex">The end index in the string (exclusive). Must be greater or equal to startIndex.</param>
/// <returns>A pointer to the substring.</returns>
char* String_SubString(const char* string, size_t startIndex, size_t endIndex);

/// <summary>
/// Creates a trimmed copy of the string. Trims whitespace characters '\n', '\t', ' ' and '\r'.
/// </summary>
/// <param name="string">A null terminated string.</param>
/// <returns>A pointer to the trimmed string</returns>
char* String_Trim(char* string);

/// <summary>
/// Converts the string's characters to lowercase. Only supports Latvian and English alphabet.
/// </summary>
/// <returns>Error Code</returns>
ErrorCode String_ToLowerUTF8(char* string);

/// <summary>
/// Converts the string's characters to uppercase. Only supports Latvian and English alphabet.
/// </summary>
/// <returns>Error Code</returns>
ErrorCode String_ToUpperUTF8(char* string);

/// <summary>
/// Counts the number of occurrences of the specified character sequence in the string.
/// </summary>
/// <param name="string">The string to search in.</param>
/// <param name="sequence">The character sequence string.</param>
/// <returns>The number of sequence occurrences found in the string.</returns>
size_t String_Count(const char* string, const char* sequence);

_Bool String_Contains(const char* string, const char* sequence);

_Bool String_StartsWith(const char* string, const char* sequence);

_Bool String_EndsWith(const char* string, const char* sequence);

_Bool String_Equals(const char* string1, const char* string2);

char* String_Replace(const char* string, const char* oldSequence, const char* newSequence);

ErrorCode String_Split(char* string, const char* sequence, ArrayList* listOfNewStrings);


// StringBuilder
ErrorCode StringBuilder_Construct(StringBuilder* builder, size_t capacity);

StringBuilder* StringBuilder_Construct2(size_t capacity);

ErrorCode StringBuilder_Append(StringBuilder* this, const char* string);

ErrorCode StringBuilder_AppendChar(StringBuilder* this, char character);

ErrorCode StringBuilder_Insert(StringBuilder* this, const char* string, size_t charIndex);

ErrorCode StringBuilder_InsertChar(StringBuilder* this, char character, size_t charIndex);

ErrorCode StringBuilder_Remove(StringBuilder* this, size_t startIndex, size_t endIndex);

ErrorCode StringBuilder_Clear(StringBuilder* this);

ErrorCode StringBuilder_Deconstruct(StringBuilder* this);