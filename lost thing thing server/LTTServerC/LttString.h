#pragma once
#include <stdbool.h>

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
int String_LengthCodepoints(char* string);

int String_LengthBytes(char* string);

int String_ByteIndexOf(char* string, char* sequence);

int String_LastByteIndexOf(char* string, char* sequence);

char* String_CreateCopy(char* string);

char* String_SubString(char* string, const int startIndex, const int endIndex);

char* String_Trim(char* string);

char* String_ToLower(char* string);

char* String_ToUpper(char* string);

int String_Count(char* string, char* sequence);

bool String_Contains(char* string, char* sequence);

bool String_StartsWith(char* string, char* sequence);

bool String_EndsWith(char* string, char* sequence);

bool String_Equals(char* string1, char* string2);

char* String_Replace(char* string, char* oldSequence, char* newSequence);


// StringBuilder
void StringBuilder_Construct(StringBuilder* builder, int capacity);

StringBuilder* StringBuilder_Construct2(int capacity);

int StringBuilder_Append(StringBuilder* this, char* string);

int StringBuilder_AppendChar(StringBuilder* this, char character);

int StringBuilder_Insert(StringBuilder* this, char* string, int byteIndex);

int StringBuilder_InsertCharacter(StringBuilder* this, char character, int byteIndex);

int StringBuilder_Remove(StringBuilder* this, int startIndex, int endIndex);

int StringBuilder_Clear(StringBuilder* this);