#pragma once
#include <stdbool.h>


// Macros.
#define UTF8_ONE_BYTE 0
#define UTF8_TWO_BYTES 0b11000000
#define UTF8_THREE_BYTES 0b11100000
#define UTF8_FOUR_BYTES 0b11110000
#define UTF8_ONE_BYTE_COMBINED 0b10000000
#define UTF8_TWO_BYTES_COMBINED 0b11100000
#define UTF8_THREE_BYTES_COMBINED 0b11110000
#define UTF8_FOUR_BYTES_COMBINED 0b11111000

#define UTF8_TRAILING_BYTE 0b10000000
#define UTF8_TRAILING_BYTE_COMBINED 0b11000000

#define UTF8_MAX_CODEPOINT_ONE_BYTE 127
#define UTF8_MAX_CODEPOINT_TWO_BYTES 2047
#define UTF8_MAX_CODEPOINT_THREE_BYTES 65535


#define IsLatinACharacter(characterNumber) ((256 <= characterNumber) && (characterNumber <= 383))


// Functions.
int Char_GetByteCount(const char* character);

int Char_GetCodepoint(const char* character);

int Char_SetCodePoint(char* character, int codePoint);

void Char_ToUpper(char* character);

void Char_ToLower(char* character);

bool IsLetter(const char* character);

bool IsDigit(const char* character);

bool IsWhitespace(const char* character);