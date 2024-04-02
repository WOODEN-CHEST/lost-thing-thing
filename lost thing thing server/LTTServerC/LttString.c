#include "LttString.h"
#include "LttErrors.h"
#include "Memory.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "LTTChar.h"


// Macros.
#define STRING_BUILDER_CAPACITY_GROWTH 4

#define STRING_ARRAY_CAPACITY_GROWTH 2


// Static functions.
// Functions.
// String.
size_t String_LengthCodepointsUTF8(const char* string)
{
	size_t Length = 0;

	for (size_t i = 0; string[i] != '\0'; i += Char_GetByteCount(string + i))
	{
		Length++;
	}

	return Length;
}

_Bool String_IsValidUTF8String(const char* string)
{
	int ExpectedByteCount;
	for (size_t Index = 0; string[Index] != '\0'; Index++)
	{
		ExpectedByteCount = Char_GetByteCount(string + Index);

		for (int SubByteIndex = 1; SubByteIndex < ExpectedByteCount; SubByteIndex++)
		{
			if ((string[Index + SubByteIndex] & UTF8_TRAILING_BYTE_COMBINED) != UTF8_TRAILING_BYTE)
			{
				return false;
			}
		}
	}
	return true;
}

size_t String_LengthBytes(const char* string)
{
	size_t Length = 0;
	while (string[Length] != '\0')
	{
		Length++;
	}

	return Length;
}

char* String_CreateCopy(const char* stringToCopy)
{
	size_t StringLengthBytes = String_LengthBytes(stringToCopy) + 1;
	char* NewString = Memory_SafeMalloc(StringLengthBytes);

	for (int i = 0; i < StringLengthBytes; i++)
	{
		NewString[i] = stringToCopy[i];
	}

	return NewString;
}

void String_CopyTo(const char* sourceString, char* destinationString)
{
	int Index;
	for (Index = 0; sourceString[Index] != '\0'; Index++)
	{
		destinationString[Index] = sourceString[Index];
	}
	destinationString[Index] = '\0';
}

int String_CharIndexOf(const char* string, const char* sequence)
{
	if (sequence[0] == '\0')
	{
		return -1;
	}

	int SequenceByteLength = String_LengthBytes(sequence);
	int Index = 0, SequenceIndex = 0, StartIndex = -1;

	while (string[Index] != '\0')
	{
		if (string[Index] == sequence[SequenceIndex])
		{
			if (StartIndex == -1)
			{
				StartIndex = Index;
			}

			SequenceIndex++;
			if (SequenceIndex >= SequenceByteLength)
			{
				return StartIndex;
			}
		}
		else
		{
			SequenceIndex = 0;
			StartIndex = -1;
		}

		Index++;
	}

	return -1;
}

int String_LastCharIndexOf(const char* string, const char* sequence)
{
	if (sequence[0] == '\0')
	{
		return -1;
	}

	size_t StringLengthBytes = String_LengthBytes(string);
	int MaxSequenceIndex = String_LengthBytes(sequence) - 1;
	int Index = StringLengthBytes - 1, SequenceIndex = MaxSequenceIndex;

	while (Index >= 0)
	{
		if (string[Index] == sequence[SequenceIndex])
		{
			SequenceIndex--;
			if (SequenceIndex < 0)
			{
				return Index;
			}
		}
		else
		{
			SequenceIndex = MaxSequenceIndex;
		}

		Index--;
	}

	return -1;
}

char* String_SubString(const char* string, size_t startIndex, size_t endIndex)
{
	if (endIndex < startIndex)
	{
		Error_SetError(ErrorCode_IndexOutOfRange, "String_SubString: endIndex is lower than startIndex");
		return NULL;
	}

	int ByteLength = String_LengthBytes(string);
	if (startIndex > ByteLength)
	{
		Error_SetError(ErrorCode_IndexOutOfRange, "String_SubString: startIndex is greater than the length of the string.");
		return NULL;
	}
	if (endIndex > ByteLength)
	{
		Error_SetError(ErrorCode_IndexOutOfRange, "String_SubString: endIndex is greater than the length of the string.");
		return NULL;
	}

	size_t SubStringLength = endIndex - startIndex;
	size_t TotalStringLength = SubStringLength + 1;

	char* NewValue = Memory_SafeMalloc(TotalStringLength);
	for (int Cur = startIndex, New = 0; Cur < endIndex; Cur++, New++)
	{
		NewValue[New] = string[Cur];
	}
	NewValue[SubStringLength] = '\0';

	return NewValue;
}

char* String_Trim(const char* string)
{
	int StartIndex = 0;
	while (Char_IsWhitespace(string[StartIndex]))
	{
		StartIndex++;
		
		if (string[StartIndex] == '\0')
		{
			StartIndex--;
			break;
		}
	}
	
	int EndIndex = String_LengthBytes(string) - 1;
	while ((EndIndex >= StartIndex) && Char_IsWhitespace(string[EndIndex]))
	{
		EndIndex--;
	}
	EndIndex++;

	return String_SubString(string, StartIndex, EndIndex);
}

void String_ToLowerUTF8(char* string)
{
	for (int i = 0; string[i] != '\0'; i++)
	{
		Char_ToLower(string + i);
		i += Char_GetByteCount(string + i);
	}
}

void String_ToUpperUTF8(char* string)
{
	for (int i = 0; string[i] != '\0';)
	{
		Char_ToUpper(string + i);
		i += Char_GetByteCount(string + i);
	}
}

size_t String_Count(const char* string, const char* sequence)
{
	if (sequence[0] == '\0')
	{
		return 0;
	}

	size_t Count = 0;
	for (int Index = 0, SequenceIndex = 0; string[Index] != '\0'; Index++)
	{
		if (string[Index] == sequence[SequenceIndex])
		{
			SequenceIndex++;

			if (sequence[SequenceIndex] == '\0')
			{
				SequenceIndex = 0;
				Count++;
			}
		}
		else
		{
			SequenceIndex = 0;
		}
	}

	return Count;
}

bool String_Contains(const char* string, const char* sequence)
{
	return String_CharIndexOf(string, sequence) != -1;
}

bool String_StartsWith(const char* string, const char* sequence)
{
	if (sequence[0] == '\0')
	{
		return false;
	}

	for (int Index = 0; sequence[Index] != '\0'; Index++)
	{
		if (string[Index] != sequence[Index])
		{
			return false;
		}
	}

	return true;
}

bool String_EndsWith(const char* string, const char* sequence)
{
	if (sequence[0] == '\0')
	{
		return false;
	}

	int StringLength = String_LengthBytes(string);
	int SequenceLength = String_LengthBytes(sequence);

	if (StringLength < SequenceLength)
	{
		return false;
	}

	for (int Index = StringLength - 1, SequenceIndex = SequenceLength - 1; SequenceIndex >= 0; Index--, SequenceIndex--)
	{
		if (string[Index] != sequence[SequenceIndex])
		{
			return false;
		}
	}

	return true;
}

bool String_Equals(const char* string1, const char* string2)
{
	size_t Index;
	for (Index = 0; (string1[Index] != '\0') && (string2[Index] != '\0'); Index++)
	{
		if (string1[Index] != string2[Index])
		{
			return false;
		}
	}

	return string1[Index] == string2[Index];
}

char* String_Replace(const char* string, const char* oldSequence, const char* newSequence)
{
	if (oldSequence[0] == '\0')
	{
		return string;
	}

	StringBuilder Builder;
	StringBuilder_Construct(&Builder, DEFAULT_STRING_BUILDER_CAPACITY);

	for (int Index = 0, SequenceIndex = 0; string[Index] != '\0'; Index++)
	{
		if (string[Index] == oldSequence[SequenceIndex])
		{
			SequenceIndex++;
			if (oldSequence[SequenceIndex] == '\0')
			{
				SequenceIndex = 0;
				StringBuilder_Append(&Builder, newSequence);
			}
		}
		else
		{
			StringBuilder_AppendChar(&Builder, string[Index]);
			SequenceIndex = 0;
		}
	}

	return Builder.Data;
}

_Bool String_IsFuzzyMatched(const char* stringToSearchIn, const char* stringToMatch)
{
	if (stringToMatch[0] == '\0')
	{
		return stringToSearchIn[0] == '\0';
	}

	for (size_t OriginIndex = 0, MatchIndex = 0; stringToSearchIn[OriginIndex] != '\0'; OriginIndex++)
	{
		if (stringToSearchIn[OriginIndex] == stringToMatch[MatchIndex])
		{
			MatchIndex++;
			if (stringToMatch[MatchIndex] == '\0')
			{
				return true;
			}
		}
	}

	return false;
}

// StringBuilder
void StringBuilder_Construct(StringBuilder* builder, size_t capacity)
{
	if (capacity < 1)
	{
		capacity = DEFAULT_STRING_BUILDER_CAPACITY;
	}

	builder->_capacity = capacity;
	builder->Length = 0;
	builder->Data = Memory_SafeMalloc(capacity * sizeof(char));
	builder->Data[0] = '\0';
}

StringBuilder* StringBuilder_Construct2(size_t capacity)
{
	StringBuilder* Builder = Memory_SafeMalloc(sizeof(StringBuilder));
	StringBuilder_Construct(&Builder, capacity);
	return Builder;
}

static void StringBuilder_EnsureCapacity(StringBuilder* this, size_t capacity)
{
	if (this->_capacity >= capacity)
	{
		return;
	}

	do
	{
		this->_capacity *= STRING_BUILDER_CAPACITY_GROWTH;
	} while (this->_capacity < capacity);

	this->Data = Memory_SafeRealloc(this->Data, this->_capacity * sizeof(char));
}

void StringBuilder_Append(StringBuilder* this, const char* string)
{
	int AppendLength = String_LengthBytes(string);
	StringBuilder_EnsureCapacity(this, this->Length + AppendLength + 1);

	for (int i = 0; i < AppendLength; i++)
	{
		this->Data[this->Length + i] = string[i];
	}
	
	this->Length += AppendLength;
	this->Data[this->Length] = '\0';

	return 0;
}

void StringBuilder_AppendChar(StringBuilder* this, char character)
{
	StringBuilder_EnsureCapacity(this, this->Length + 2);

	this->Data[this->Length] = character;
	this->Data[this->Length + 1] = '\0';
	this->Length++;
}

ErrorCode StringBuilder_Insert(StringBuilder* this, const char* string, size_t charIndex)
{
	if (charIndex > this->Length)
	{
		return Error_SetError(ErrorCode_IndexOutOfRange, "StringBuilder_Insert: Index is larger than the length of the string.");
	}

	const int StringLength = String_LengthBytes(string);
	if (StringLength == 0)
	{
		return ErrorCode_Success;
	}

	StringBuilder_EnsureCapacity(this, this->Length + 1 + StringLength);

	for (int i = this->Length - 1 + StringLength; i >= charIndex + StringLength; i--)
	{
		this->Data[i] = this->Data[i - StringLength];
	}

	for (int i = 0; i < StringLength; i++)
	{
		this->Data[charIndex + i] = string[i];
	}

	this->Length += StringLength;
	this->Data[this->Length] = '\0';

	return ErrorCode_Success;
}

ErrorCode StringBuilder_InsertChar(StringBuilder* this, char character, size_t charIndex)
{
	if (charIndex > this->Length)
	{
		return Error_SetError(ErrorCode_IndexOutOfRange, "StringBuilder_InsertChar: Index is larger than the length of the string.");
	}

	for (int i = this->Length; i > charIndex; i--)
	{
		this->Data[i] = this->Data[i - 1];
	}

	this->Data[charIndex] = character;
	this->Length += 1;
	this->Data[this->Length] = '\0';

	return ErrorCode_Success;
}

ErrorCode StringBuilder_Remove(StringBuilder* this, size_t startIndex, size_t endIndex)
{
	if (startIndex > endIndex)
	{
		return Error_SetError(ErrorCode_IndexOutOfRange, "StringBuilder_Remove: startIndex is greater than endIndex.");
	}
	if (startIndex > this->Length)
	{
		return Error_SetError(ErrorCode_IndexOutOfRange, "StringBuilder_Remove: startIndex is greater than the string's length.");
	}
	if (endIndex > this->Length)
	{
		return Error_SetError(ErrorCode_IndexOutOfRange, "StringBuilder_Remove: endIndex is greater than the string's length.");
	}

	int RemovedLength = endIndex - startIndex;
	if (RemovedLength == 0)
	{
		return ErrorCode_Success;
	}
	
	for (int i = endIndex; i < this->Length; i++)
	{
		this->Data[i - RemovedLength] = this->Data[i];
	}

	this->Length -= RemovedLength;
	this->Data[this->Length] = '\0';

	return ErrorCode_Success;
}

void StringBuilder_Clear(StringBuilder* this)
{
	this->Length = 0;
	this->Data[0] = '\0';
}

void StringBuilder_Deconstruct(StringBuilder* this)
{
	Memory_Free(this->Data);
}