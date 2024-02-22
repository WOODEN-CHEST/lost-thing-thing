#pragma once
#include <stdbool.h>
#include <stddef.h>


// Types.
union GHDFPrimitive
{
	signed char SChar;
	unsigned char UCchar;
	signed short Short;
	unsigned short UShort;
	signed int Int;
	unsigned int UInt;
	signed long long Long;
	unsigned long long ULong;

	float Float;
	double Double;

	bool Bool;

	char* String;
};

typedef struct GHDFArrayStruct
{
	GHDFPrimitive* Array;
	size_t Size;
} GHDFArray;

union GHDFEntry
{
	GHDFPrimitive Value;
	GHDFArray ValueArray;
};


// Functions.