#include "LTTChar.h"

// https://en.wikipedia.org/wiki/List_of_Unicode_characters
// https://en.wikipedia.org/wiki/UTF-8
// https://unicodelookup.com/


// Macros.
#define OFFSET_DIRECTION_TO_LOWER 1
#define OFFSET_DIRECTION_TO_UPPER -1

#define IsCodepointInRange(codepoint, lower, upper) ((lower <= codepoint) && (codepoint <= upper))


// Static functions.
static void OffsetLetter(char* character, int direction)
{
	// This does not account for all letters, but fits the project's requirements.
	int Codepoint = Char_GetCodepoint(character);

	if (IsCodepointInRange(Codepoint, 65, 90))
	{
		Char_SetCodePoint(character, Codepoint + (UTF8_ASCII_CODEPOINT_OFFSET * direction));
	}
	else if (IsCodepointInRange(Codepoint, 192, 222) && (Codepoint != 215))
	{
		Char_SetCodePoint(character, Codepoint + (UTF8_LATIN1_CODEPOINT_OFFSET * direction));
	}
	else if (IsCodepointInRange(Codepoint, 256, 382) || IsCodepointInRange(Codepoint, 384, 392))
	{
		Char_SetCodePoint(character, Codepoint + (UTF8_EXTENDED_CODEPOINT_OFFSET * direction));
	}
}


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

int Char_GetByteCountCodepoint(int codepoint)
{
	if (codepoint <= UTF8_MAX_CODEPOINT_ONE_BYTE)
	{
		return 1;
	}
	else if (codepoint <= UTF8_MAX_CODEPOINT_TWO_BYTES)
	{
		return 2;
	}
	else if (codepoint <= UTF8_MAX_CODEPOINT_THREE_BYTES)
	{
		return 3;
	}
	return 4;
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

void Char_SetCodePoint(char* character, int codepoint)
{
	unsigned char* Character = (unsigned char*)character;
	int ByteCount = Char_GetByteCountCodepoint(codepoint);

	switch (ByteCount)
	{
		case 1:
			*Character = codepoint;
			break;
		case 2:
			*Character = (((codepoint >> 6) & (~UTF8_TWO_BYTES_COMBINED))) | UTF8_TWO_BYTES;
			*(Character + 1) = ((codepoint & (~UTF8_TRAILING_BYTE_COMBINED))) | UTF8_TRAILING_BYTE;
			break;
		case 3:
			*Character = (((codepoint >> 12) & (~UTF8_THREE_BYTES_COMBINED))) | UTF8_THREE_BYTES;
			*(Character + 1) = (((codepoint >> 6) & (~UTF8_TRAILING_BYTE_COMBINED))) | UTF8_TRAILING_BYTE;
			*(Character + 2) = ((codepoint & (~UTF8_TRAILING_BYTE_COMBINED))) | UTF8_TRAILING_BYTE;
			break;
		default:
			*Character = (((codepoint >> 18) & (~UTF8_FOUR_BYTES_COMBINED))) | UTF8_FOUR_BYTES;
			*(Character + 1) = (((codepoint >> 12) & (~UTF8_TRAILING_BYTE_COMBINED))) | UTF8_TRAILING_BYTE;
			*(Character + 2) = (((codepoint >> 6) & (~UTF8_TRAILING_BYTE_COMBINED))) | UTF8_TRAILING_BYTE;
			*(Character + 3) = ((codepoint & (~UTF8_TRAILING_BYTE_COMBINED))) | UTF8_TRAILING_BYTE;
			break;
	}
}

void Char_ToUpper(char* character)
{
	OffsetLetter(character, OFFSET_DIRECTION_TO_UPPER);
}

void Char_ToLower(char* character)
{
	OffsetLetter(character, OFFSET_DIRECTION_TO_LOWER);
}

bool Char_IsLetter(const char* character)
{
	int Codepoint = Char_GetCodepoint(character);

	return ((65 <= Codepoint) && (Codepoint <= 90)) || ((97 <= Codepoint) && (Codepoint <= 122))
		|| ((192 <= Codepoint) && (Codepoint <= 214)) || ((216 <= Codepoint) && (Codepoint <= 246))
		|| ((248 <= Codepoint) && (Codepoint <= 687)) || ((880 <= Codepoint) && (Codepoint <= 1023));
}

bool Char_IsDigit(const char* character)
{
	return ('0' <= character) && (character <= '9');
}

bool Char_IsWhitespace(const char* character)
{
	return (0 <= *character) && (*character <= 32);
}