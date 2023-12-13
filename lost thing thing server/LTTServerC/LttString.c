#include "LttString.h"
#include "ErrorCodes.h"
#include "Memory.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "ArrayList.h"

#define STRING_BUILDER_CAPACITY_GROWTH 4

#define UTF8_TWO_BYTES  0b11000000
#define UTF8_THREE_BYTES 0b11100000
#define UTF8_FOUR_BYTES 0b11110000
#define UTF8_TWO_BYTES_COMBINED 0b11100000
#define UTF8_THREE_BYTES_COMBINED 0b11110000
#define UTF8_FOUR_BYTES_COMBINED 0b11111000

#define ASCII_OFFSET 32

// Functions.
// String.
int String_LengthCodepointsUTF8(char* string)
{
	if (string == NULL)
	{
		return 0;
	}

	int Length = 0;
	char Character;

	for (int i = 0; string[i] != '\0'; i++)
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

int String_LengthBytes(char* string)
{
	if (string == NULL)
	{
		return 0;
	}

	int Length = 0;
	while (string[Length] != '\0')
	{
		Length++;
	}

	return Length;
}

char* String_CreateCopy(char* string)
{
	if (string == NULL)
	{
		return NULL;
	}

	int StringLengthBytes = String_LengthBytes(string) + 1;
	char* NewString = SafeMalloc(StringLengthBytes);

	for (int i = 0; i < StringLengthBytes; i++)
	{
		NewString[i] = string[i];
	}

	return NewString;
}

int String_ByteIndexOf(char* string, char* sequence)
{
	if ((string == NULL) || (sequence == NULL) || (sequence[0] == '\0'))
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

int String_LastByteIndexOf(char* string, char* sequence)
{
	if ((string == NULL) || (sequence == NULL) || (sequence[0] == '\0'))
	{
		return -1;
	}

	int StringLengthBytes = String_LengthBytes(string);
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

char* String_SubString(char* string, const int startIndex, const int endIndex)
{
	if ((string == NULL) || (startIndex < 0) || (endIndex < 0) || (endIndex < startIndex))
	{
		return NULL;
	}

	int ByteLength = String_LengthBytes(string);
	if ((startIndex > ByteLength) || (endIndex > ByteLength))
	{
		return NULL;
	}

	int SubStringLength = endIndex - startIndex;
	int TotalStringLength = SubStringLength + 1;

	char* NewValue = SafeMalloc(TotalStringLength);
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

char* String_Trim(char* string)
{
	if (string == NULL)
	{
		return NULL;
	}

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

static int String_ChangeCase(char* string, signed char step)
{
	
}

int String_ToLowerUTF8(char* string)
{
	if (string == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	for (int i = 0; string[i] != '\0'; i++)
	{
		if ((string[i] >= 'A') && (string[i] <= 'Z'))
		{
			string[i] += ASCII_OFFSET;
		}
		else if (((string[i] & UTF8_TWO_BYTES_COMBINED) == UTF8_TWO_BYTES)
			)
		{

		}
	}
}

int String_ToUpperUTF8(char* string)
{
	if (string == NULL)
	{
		return NULL;
	}
}

int String_Count(char* string, char* sequence)
{

}

bool String_Contains(char* string, char* sequence)
{
	return String_ByteIndexOf(string, sequence) != -1;
}

bool String_StartsWith(char* string, char* sequence)
{
	if ((string == NULL) || (sequence == NULL))
	{
		return false;
	}
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
	if ((string == NULL) || (sequence == NULL))
	{
		return false;
	}
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
	if ((string1 == NULL) || (string2 == NULL))
	{
		return false;
	}

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

}

ArrayList Split(char* string, char* sequence)
{

}

// StringBuilder
int StringBuilder_Construct(StringBuilder* builder, int capacity)
{
	if (builder == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}
	if (capacity <= 1)
	{
		capacity = DEFAULT_STRING_BUILDER_CAPACITY;
	}

	builder->_capacity = capacity;
	builder->Length = 0;
	builder->Data = SafeMalloc(capacity * sizeof(char));
	builder->Data[0] = '\0';

	return 0;
}

StringBuilder* StringBuilder_Construct2(int capacity)
{
	StringBuilder* Builder = SafeMalloc(sizeof(Builder));
	StringBuilder_Construct(&Builder, capacity);
	return Builder;
}

static void StringBuilder_EnsureCapacity(StringBuilder* this, int capacity)
{
	while (this->_capacity < capacity)
	{
		this->_capacity *= STRING_BUILDER_CAPACITY_GROWTH;
		this->Data = SafeRealloc(this->Data, this->_capacity * sizeof(char));
	}
}

int StringBuilder_Append(StringBuilder* this, char* string)
{
	if ((this == NULL) || (string == NULL))
	{
		return NULL_REFERENCE_ERRCODE;
	}

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

int StringBuilder_AppendChar(StringBuilder* this, char character)
{
	if (this == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	StringBuilder_EnsureCapacity(this, this->Length + 2);

	this->Data[this->Length] = character;
	this->Data[this->Length + 1] = '\0';
	this->Length += 1;

	return 0;
}

int StringBuilder_Insert(StringBuilder* this, char* string, int byteIndex)
{
	if ((this == NULL) || (string == NULL))
	{
		return NULL_REFERENCE_ERRCODE;
	}
	if ((byteIndex > this->Length) || (byteIndex < 0))
	{
		return INDEX_OUT_OF_RANGE_ERRCODE;
	}

	const int StringLength = String_LengthBytes(string);
	if (StringLength == 0)
	{
		return 0;
	}

	StringBuilder_EnsureCapacity(this, this->Length + 1 + StringLength);

	for (int i = this->Length - 1 + StringLength; i >= byteIndex + StringLength; i--)
	{
		this->Data[i] = this->Data[i - StringLength];
	}

	for (int i = 0; i < StringLength; i++)
	{
		this->Data[byteIndex + i] = string[i];
	}

	this->Length += StringLength;
	this->Data[this->Length] = '\0';

	return 0;
}

int StringBuilder_InsertChar(StringBuilder* this, char character, int byteIndex)
{
	if (this == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	if ((byteIndex > this->Length) || (byteIndex < 0))
	{
		return ARGUMENT_OUT_OF_RANGE_ERRCODE;
	}

	for (int i = this->Length; i > byteIndex; i--)
	{
		this->Data[i] = this->Data[i - 1];
	}

	this->Data[byteIndex] = character;
	this->Length += 1;
	this->Data[this->Length] = '\0';

	return 0;
}

int StringBuilder_Remove(StringBuilder* this, int startIndex, int endIndex)
{
	if (this == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	if ((startIndex > endIndex) || (startIndex < 0) || (endIndex > this->Length))
	{
		return INDEX_OUT_OF_RANGE_ERRCODE;
	}

	int RemovedLength = endIndex - startIndex;
	if (RemovedLength == 0)
	{
		return;
	}
	
	for (int i = endIndex; i < this->Length; i++)
	{
		this->Data[i - RemovedLength] = this->Data[i];
	}

	this->Length -= RemovedLength;
	this->Data[this->Length] = '\0';

	return 0;
}

int StringBuilder_Clear(StringBuilder* this)
{
	if (this == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	this->Length = 0;
	this->Data[0] = '\0';

	return 0;
}