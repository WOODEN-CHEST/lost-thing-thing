#pragma once
#include <stdbool.h>
#include <stddef.h>


// Macros.
#define GHDT_TYPE_ARRAY_BIT 0b100000000000000000000000000000000

#define COMPOUND_DEFAULT_CAPACITY 32


// Types.
typedef enum GHDFTypeEnum
{
	GHDFType_SChar = 0,
	GHDFType_UChar = 1,
	GHDFType_Short =  2,
	GHDFType_UShort = 3,
	GHDFType_Int = 4,
	GHDFType_UInt = 5,
	GHDFType_Long = 6,
	GHDFType_ULong = 7,
	GHDFType_Float = 8,
	GHDFType_Double = 9,
	GHDFType_Bool = 10,
	GHDFType_String = 11,
	GHDFType_Compound = 12
} GHDFType;

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

	GHDFEntryStruct*
};

typedef struct GHDFArrayStruct
{
	GHDFPrimitive* Array;
	size_t Size;
} GHDFArray;

typedef union GHDFEntryValue
{
	GHDFPrimitive SingleValue;
	GHDFArray ValueArray;
};

typedef struct GHDFEntryStruct
{
	int ID;
	GHDFType ValueType;
	GHDFEntryValue Value;
} GHDFEntry;

typedef struct GHDFEntryBucket
{
	GHDFEntry* Entries;
	size_t Count;
	size_t _capacity;
};

typedef struct GHDFCompound
{
	GHDFEntry* Entries;
};


// Functions.