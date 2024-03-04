#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "LTTErrors.h"


// Macros.
#define GHDT_TYPE_ARRAY_BIT 0b10000000

#define COMPOUND_DEFAULT_CAPACITY 32


// Types.
typedef enum GHDFTypeEnum
{
	GHDFType_None = 0,
	GHDFType_SByte = 1,
	GHDFType_UByte = 2,
	GHDFType_Short =  3,
	GHDFType_UShort = 4,
	GHDFType_Int = 5,
	GHDFType_UInt = 6,
	GHDFType_Long = 7,
	GHDFType_ULong = 8,

	GHDFType_Float = 9,
	GHDFType_Double = 10,

	GHDFType_Bool = 11,

	GHDFType_Char = 12,

	GHDFType_String = 13,
	GHDFType_Compound = 14
} GHDFType;

typedef union GHDFPrimitiveUnion
{
	signed char SByte;
	unsigned char UByte;
	signed short Short;
	unsigned short UShort;
	signed int Int;
	unsigned int UInt;
	signed long long Long;
	unsigned long long ULong;

	int Char;

	float Float;
	double Double;

	bool Bool;

	char* String;

	struct GHDFCompoundStruct* Compound;
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

ErrorCode GHDFCompound_ReadFromFile(const char* path, GHDFCompound* emptyBaseCompound);

ErrorCode GHDFCompound_SaveToFile(GHDFCompound* compound, const char* path);

void GHDFCompound_Deconstruct(GHDFCompound* self);