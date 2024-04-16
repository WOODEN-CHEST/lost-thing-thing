#include "LTTBase64.h"
#include "LTTString.h"
#include "Memory.h"

#define BITS_PER_CHARACTER 6
#define BIT_LOSS_PER_CHARACTER 2

#define PADDING '='

// Static functions.
static char ByteValueToChar(char byteValue)
{
	if (0 <= byteValue && byteValue <= 25)
	{
		return byteValue + 'A';
	}
	if (26 <= byteValue && byteValue <= 51)
	{
		return byteValue - 26 + 'a';
	}
	if (52 <= byteValue && byteValue <= 61)
	{
		return byteValue - 52 + '0';
	}
	if (byteValue == 62)
	{
		return '+';
	}
	if (byteValue == 63)
	{
		return '/';
	}
	return '=';
}

static unsigned char CharToByteValue(char character)
{
	if (('A' <= character) && (character <= 'Z'))
	{
		return character - 'A';
	}
	if (('a' <= character) && (character <= 'z'))
	{
		return character - 'a' + 26;
	}
	if (('0' <= character) && (character <= '9'))
	{
		return character - '0' + 52;
	}
	if (character == '+')
	{
		return 62;
	}
	return 63;
}


// Functions.
// Magic numbers here we go!
char* Base64_Decode(const char* data, size_t* decodedDataLength)
{
	size_t DataLength = String_LengthBytes(data);
	if ((DataLength < 4) || (DataLength % 4 != 0))
	{
		*decodedDataLength = 0;
		return NULL;
	}

	unsigned char* DecodedData = (unsigned char*)Memory_SafeMalloc((size_t)(DataLength * 0.8));

	size_t DataIndex = 0;
	for (size_t CharIndex = 0; CharIndex < DataLength; CharIndex += 4, DataIndex += 3)
	{
		unsigned char DecodedByte;
		DecodedByte = (CharToByteValue(data[CharIndex]) << 2) | ((CharToByteValue(data[CharIndex + 1]) & 0b110000) >> 4);
		DecodedData[DataIndex] = DecodedByte;

		DecodedByte = ((CharToByteValue(data[CharIndex + 1]) & 0b001111) << 4) | (((CharToByteValue(data[CharIndex + 2]) & 0b111100) >> 2));
		DecodedData[DataIndex + 1] = DecodedByte;

		DecodedByte = ((CharToByteValue(data[CharIndex + 2]) & 0b000011) << 6) | CharToByteValue(data[CharIndex + 3]);
		DecodedData[DataIndex + 2] = DecodedByte;
	}

	if (data[DataLength - 2] == '=')
	{
		*decodedDataLength = DataIndex - 2;
	}
	else if (data[DataLength - 1] == '=')
	{
		*decodedDataLength = DataIndex - 1;
	}
	else *decodedDataLength = DataIndex;

	return DecodedData;
}

char* Base64_Encode(const unsigned char* data, size_t dataLength)
{
	StringBuilder Builder;
	StringBuilder_Construct(&Builder, DEFAULT_STRING_BUILDER_CAPACITY);

	for (size_t i = 0; i < dataLength; i += 3)
	{
		unsigned char Value = data[i] >> 2;
		StringBuilder_AppendChar(&Builder, ByteValueToChar(*((char*)&Value)));

		if (i + 1 >= dataLength)
		{
			Value = ((data[i] & 0b00000011) << 4);
			StringBuilder_AppendChar(&Builder, ByteValueToChar(*((char*)&Value)));
			StringBuilder_Append(&Builder, "==");
			return Builder.Data;
		}
		else
		{
			Value = ((data[i] & 0b00000011) << 4) | (data[i + 1] >> 4);
			StringBuilder_AppendChar(&Builder, ByteValueToChar(*((char*)&Value)));
		}

		if (i + 2 >= dataLength)
		{
			Value = ((data[i + 1] & 0b00001111) << 2);
			StringBuilder_AppendChar(&Builder, ByteValueToChar(*((char*)&Value)));
			StringBuilder_Append(&Builder, "=");
			return Builder.Data;
		}
		else
		{
			Value = ((data[i + 1] & 0b00001111) << 2) | (data[i + 2] >> 6);
			StringBuilder_AppendChar(&Builder, ByteValueToChar(*((char*)&Value)));
		}

		Value = (data[i + 2] & 0b00111111);
		StringBuilder_AppendChar(&Builder, ByteValueToChar(*((char*)&Value)));
	}

	return Builder.Data;
}