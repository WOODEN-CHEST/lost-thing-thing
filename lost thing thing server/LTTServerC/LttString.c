#include "LttString.h"
#include "LttErrors.h"
#include "Memory.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


// Macros.
#define STRING_BUILDER_CAPACITY_GROWTH 4

#define UTF8_TWO_BYTES  0b11000000
#define UTF8_THREE_BYTES 0b11100000
#define UTF8_FOUR_BYTES 0b11110000
#define UTF8_TWO_BYTES_COMBINED 0b11100000
#define UTF8_THREE_BYTES_COMBINED 0b11110000
#define UTF8_FOUR_BYTES_COMBINED 0b11111000

#define ASCII_OFFSET 32

#define IsLatinACharacter(characterNumber) ((256 <= characterNumber) && (characterNumber <= 383))


// Static functions.
static inline unsigned short GetTwoByteUTF8Number(unsigned short value)
{
	return ((value & 0b00011111) << 6) | ((value & 0b0011111100000000) >> 8);
}

static inline unsigned short AddUTF8TwoByte(unsigned short value, short amountToAdd)
{
	unsigned short RawValue = GetTwoByteUTF8Number(value);
	RawValue += amountToAdd;

	unsigned short FullValue = ((RawValue & 0b111111) << 8) | ((RawValue & 0b11111000000) >> 6) | 0b1000000011000000;

	return FullValue;
}


// Functions.
// String.
size_t String_LengthCodepointsUTF8(const char* string)
{
	size_t Length = 0;
	char Character;

	for (size_t i = 0; string[i] != '\0'; i++)
	{
		Character = string[i];

		if ((Character & UTF8_TWO_BYTES_COMBINED) == UTF8_TWO_BYTES)
		{
			i += 1;
		}
		else if ((Character & UTF8_THREE_BYTES_COMBINED) == UTF8_THREE_BYTES)
		{
			i += 2;
		}
		else if ((Character & UTF8_FOUR_BYTES_COMBINED) == UTF8_FOUR_BYTES)
		{
			i += 3;
		}

		Length++;
	}

	return Length;
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

bool IsCharWhitespace(char character)
{
	return (character == ' ') || (character == '\n') || (character == '\r') || (character == '\t');
}

char* String_Trim(const char* string)
{
	int StartIndex = 0;
	while (IsCharWhitespace(string[StartIndex]))
	{
		StartIndex++;
		
		if (string[StartIndex] == '\0')
		{
			StartIndex--;
			break;
		}
	}

	int EndIndex = String_LengthBytes(string) - 1;
	while ((EndIndex >= StartIndex) && IsCharWhitespace(string[EndIndex]))
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
		if ((string[i] >= 'A') && (string[i] <= 'Z'))
		{
			string[i] += ASCII_OFFSET;
			continue;
		}

		if ((string[i] & UTF8_TWO_BYTES_COMBINED) != UTF8_TWO_BYTES)
		{
			continue;
		}

		unsigned short* ValuePointer = ((unsigned short*)(string + i));
		unsigned short CharacterNumber = GetTwoByteUTF8Number(*ValuePointer);

		if (IsLatinACharacter(CharacterNumber) && (CharacterNumber % 2 == 0))
		{
			*ValuePointer = AddUTF8TwoByte(*ValuePointer, 1);
		}
	}
}

void String_ToUpperUTF8(char* string)
{
	for (int i = 0; string[i] != '\0'; i++)
	{
		if ((string[i] >= 'a') && (string[i] <= 'z'))
		{
			string[i] -= ASCII_OFFSET;
			continue;
		}

		if ((string[i] & UTF8_TWO_BYTES_COMBINED) != UTF8_TWO_BYTES)
		{
			continue;
		}

		unsigned short* ValuePointer = ((unsigned short*)(string + i));
		unsigned short CharacterNumber = GetTwoByteUTF8Number(*ValuePointer);

		if (IsLatinACharacter(CharacterNumber) && (CharacterNumber % 2 == 1))
		{
			*ValuePointer = AddUTF8TwoByte(*ValuePointer, -1);
		}
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

bool String_EndsWith(char* string, char* sequence)
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

bool String_Equals(char* string1, char* string2)
{
	for (int i = 0; (string1[i] != '\0') && (string2[i] != '\0'); i++)
	{
		if (string1[i] != string2[i])
		{
			return false;
		}
	}

	return true;
}

char* String_Replace(char* string, char* oldSequence, char* newSequence)
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

//char** String_Split(char* string, const char* sequence, int* listOfNewStrings)
//{
//	int SequenceStartIndex = 0;
//	int StringStartIndex = 0;
//	for (int Index = 0, SequenceIndex = 0; string[Index] != '\0'; Index++)
//	{
//		if (string[Index] == sequence[SequenceIndex])
//		{
//			if (SequenceIndex == 0)
//			{
//				SequenceStartIndex = Index;
//			}
//
//			SequenceIndex++;
//
//			if (sequence[SequenceIndex] == '\0')
//			{
//				SequenceIndex = 0;
//
//				string[SequenceStartIndex] = '\0';
//				ArrayList_Add(listOfNewStrings, string + StringStartIndex);
//				StringStartIndex = Index + 1;
//			}
//		}
//		else
//		{
//			SequenceIndex = 0;
//		}
//	}
//
//	if (string[StringStartIndex] != '\0')
//	{
//		ArrayList_Add(listOfNewStrings, string + StringStartIndex);
//	}
//}

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

static void StringBuilder_EnsureCapacity(StringBuilder* this, int capacity)
{
	if (this->_capacity < capacity)
	{

	}


	while (this->_capacity < capacity)
	{
		this->_capacity *= STRING_BUILDER_CAPACITY_GROWTH;
		this->Data = Memory_SafeRealloc(this->Data, this->_capacity * sizeof(char));
	}
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

ErrorCode StringBuilder_Insert(StringBuilder* this, char* string, size_t charIndex)
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