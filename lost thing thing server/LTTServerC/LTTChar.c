#include "LTTChar.h"

// https://en.wikipedia.org/wiki/List_of_Unicode_characters
// https://en.wikipedia.org/wiki/UTF-8

// Functions.
int Char_GetByteCount(const char* character)
{
	const unsigned char* Character = (const unsigned char*)character;

	if ((*Character & UTF8_FOUR_BYTES_COMBINED) == UTF8_FOUR_BYTES)
	{
		return 4;
	}
	if ((*Character & UTF8_THREE_BYTES_COMBINED) == UTF8_THREE_BYTES)
	{
		return 3;
	}
	if ((*Character & UTF8_TWO_BYTES_COMBINED) == UTF8_TWO_BYTES)
	{
		return 2;
	}
	return 1;
}

int Char_GetCodepoint(const char* character)
{
	const unsigned char* Character = (const unsigned char*)character;
	int ByteCount = Char_GetByteCount(character);

	switch (ByteCount)
	{
		case 4:
			return ((int)(*(Character + 3) & (~UTF8_TRAILING_BYTE_COMBINED))) | ((int)(*(Character + 2) & ((~UTF8_TRAILING_BYTE_COMBINED))) << 6)
				| ((int)(*(Character + 1) & ((~UTF8_TRAILING_BYTE_COMBINED))) << 12) | (((int)(*Character & (~UTF8_FOUR_BYTES_COMBINED))) << 18);
		case 3:
			return ((int)(*(Character + 2) & (~UTF8_TRAILING_BYTE_COMBINED))) | ((int)(*(Character + 1) & ((~UTF8_TRAILING_BYTE_COMBINED))) << 6)
				| (((int)(*Character & (~UTF8_THREE_BYTES_COMBINED))) << 12);
		case 2:
			return (((int)(*Character & (~UTF8_TWO_BYTES_COMBINED))) << 6) | ((int)(*(Character + 1) & (~UTF8_TRAILING_BYTE_COMBINED)));
		default:
			return (int)((*Character) & (~UTF8_ONE_BYTE_COMBINED));
	}
}

int Char_SetCodePoint(char* character, int codePoint)
{
	unsigned char* Character = (unsigned char*)character;

	if (codePoint <= UTF8_MAX_CODEPOINT_ONE_BYTE)
	{
		*Character = codePoint;
	}
	else if (codePoint <= UTF8_MAX_CODEPOINT_TWO_BYTES)
	{
		*Character = ((codePoint & (~UTF8_TWO_BYTES_COMBINED)) | );
	}
	else if (codePoint <= UTF8_MAX_CODEPOINT_THREE_BYTES)
	{

	}
	else
	{
		
	}
}

void Char_ToUpper(char* character)
{

}

void Char_ToLower(char* character)
{

}

bool IsLetter(char* character)
{
	int Codepoint = Char_GetCodepoint(character);

	return ((65 <= Codepoint) && (Codepoint <= 90)) || ((97 <= Codepoint) && (Codepoint <= 122))
		|| ((192 <= Codepoint) && (Codepoint <= 214)) || ((216 <= Codepoint) && (Codepoint <= 246))
		|| ((248 <= Codepoint) && (Codepoint <= 687)) || ((880 <= Codepoint) && (Codepoint <= 1023));
}

bool IsDigit(char* character)
{
	return ('0' <= character) && (character <= '9');
}

bool IsWhitespace(char* character)
{
	return (character <= 0 && character <= 32);
}