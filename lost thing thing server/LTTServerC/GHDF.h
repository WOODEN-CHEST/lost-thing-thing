#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "LTTErrors.h"


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

typedef union GHDFPrimitiveUnion
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

	struct GHDFEntryStruct* Compound;
} GHDFPrimitive;

typedef struct GHDFArrayStruct
{
	GHDFPrimitive* Array;
	size_t Size;
} GHDFArray;

typedef union GHDFEntryValueUnion
{
	GHDFPrimitive SingleValue;
	GHDFArray ValueArray;
} GHDFEntryValue;

typedef struct GHDFEntryStruct
{
	int ID;
	GHDFType ValueType;
	GHDFEntryValue Value;
} GHDFEntry;

typedef struct GHDFCompoundStruct
{
	GHDFEntry* Entries;
	size_t Count;
	size_t _capacity;
} GHDFCompound;


// Functions.
void GHDFCompound_Construct(GHDFCompound* self, size_t capacity);

ErrorCode GHDFCompound_AddSingleValueEntry(GHDFCompound* self, GHDFType type, int id, GHDFPrimitive value);

ErrorCode GHDFCompound_AddArrayEntry(GHDFCompound* self, GHDFType type, int id, GHDFPrimitive* valueArray, size_t count);

void GHDFCompound_RemoveEntry(GHDFCompound* self, int id);

void GHDFCompound_Clear(GHDFCompound* self);

GHDFEntry* GHDFCompound_GetEntry(GHDFCompound* self, int id);

GHDFEntry* GHDFCompound_GetEntryOrDefault(GHDFCompound* self, int id, GHDFEntry* defaultEntry);

GHDFCompound GHDFCompound_ReadFromFile(const char* path);

ErrorCode GHDFCompound_SaveToFile(GHDFCompound* compound, const char* path);

void GHDFCompound_Deconstruct(GHDFCompound* self);