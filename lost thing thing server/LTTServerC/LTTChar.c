#include "LTTChar.h"

// Functions.
int Char_GetByteCount(const char* character)
{
	if ((*character & UTF8_FOUR_BYTES_COMBINED) == UTF8_FOUR_BYTES)
	{
		return 4;
	}
	if ((*character & UTF8_THREE_BYTES_COMBINED) == UTF8_THREE_BYTES)
	{
		return 3;
	}
	if ((*character & UTF8_TWO_BYTES_COMBINED) == UTF8_TWO_BYTES)
	{
		return 2;
	}
	return 1;
}

int Char_GetCodepoint(const char* character)
{
	int ByteCount = Char_GetByteCount(character);

	switch (ByteCount)
	{
		case 4:
			return 0;
		case 3:
			return 0;
		case 2:
			int first = ((int)(*character & UTF8_TWO_BYTES_COMBINED)) << 6;
			int second = (int)(*(character + 1) & (~UTF8_TRAILING_BYTE_COMBINED));
			return (first | second);
		default:
			return (*character) & (~UTF8_ONE_BYTE_COMBINED);
	}
}

int Char_SetCodePoint(char* character, int codePoint)
{

}

void Char_ToUpper(char* character)
{

}

void Char_ToLower(char* character)
{

}

bool IsLetter(char* character)
{

}

bool IsDigit(char* character)
{

}

bool IsWhitespace(char* character)
{

}