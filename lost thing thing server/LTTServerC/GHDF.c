#include "GHDF.h"
#include "Memory.h"
#include "Directory.h"
#include "File.h"
#include <stdio.h>

// Macros.
#define COMPOUND_CAPCITY_GROWHT 2



// Static functions.
/* Compound. */
static void EnsureCompoundCapacity(GHDFCompound* self, size_t capacity)
{
	if (self->_capacity >= capacity)
	{
		return;
	}

	do
	{
		self->_capacity *= COMPOUND_CAPCITY_GROWHT;
	} while (self->_capacity < capacity);

	self->Entries = (GHDFEntry*)Memory_SafeRealloc(self->Entries, sizeof(GHDFEntry) * self->_capacity);
}

static void FreeEntryMemory(GHDFEntry* entry)
{
	if (entry->ValueType != GHDFType_Compound)
	{
		return;
	}
	
	if (entry->ValueType & GHDT_TYPE_ARRAY_BIT)
	{
		for (size_t i = 0; i < entry->Value.ValueArray.Size; i++)
		{
			GHDFCompound_Deconstruct(entry->Value.ValueArray.Array[i].Compound);
		}
	}
	else
	{
		GHDFCompound_Deconstruct(entry->Value.SingleValue.Compound);
	}
}


/* File IO. */
static ErrorCode Write7BitEncodedInt(FILE* file, int integer)
{
	unsigned long long Value = *((long*)&integer);

	do
	{
		int Result = fputc((Value & 0b01111111) | (Value > 0xff ? 0b10000000 : 0), file);
		if (Result == EOF)
		{
			return ErrorContext_SetError(ErrorCode_IO, "Write7BitEncodedInt: Failed to write char.");
		}

		Value >> 7;
	} while (Value > 0);
}

static ErrorCode Read7BitEncodedInt(FILE* file, int* result)
{
	//int Value = 0;
	//int LastByte;

	//do
	//{
	//	LastByte = File_ReadChar(file);
	//}
	//while ()
}


// Functions.
void GHDFCompound_Construct(GHDFCompound* self, size_t capacity)
{
	if (capacity <= 0)
	{
		capacity = 1;
	}

	self->Entries = (GHDFEntry*)Memory_SafeMalloc(sizeof(GHDFEntry) * capacity);
	self->Count = 0;
	self->_capacity = capacity;
}

ErrorCode GHDFCompound_AddSingleValueEntry(GHDFCompound* self, GHDFType type, int id, GHDFPrimitive value)
{
	if (id == 0)
	{
		return ErrorContext_SetError(ErrorCode_InvalidArgument, "GHDFCompound_AddSingleValueEntry: An ID of 0 is not allowed.");
	}

	EnsureCompoundCapacity(self, self->Count + 1);
	self->Entries[self->Count].ID = id;
	self->Entries[self->Count].Value.SingleValue = value;
	self->Entries[self->Count].ValueType = type;

	self->Count += 1;
}

ErrorCode GHDFCompound_AddArrayEntry(GHDFCompound* self, GHDFType type, int id, GHDFPrimitive* valueArray, size_t count)
{
	if (id == 0)
	{
		return ErrorContext_SetError(ErrorCode_InvalidArgument, "GHDFCompound_AddArrayEntry: An ID of 0 is not allowed.");
	}

	EnsureCompoundCapacity(self, self->Count + 1);
	self->Entries[self->Count].ID = id;
	self->Entries[self->Count].Value.ValueArray.Array = valueArray;
	self->Entries[self->Count].Value.ValueArray.Size = count;
	self->Entries[self->Count].ValueType = type;

	self->Count += 1;
}

void GHDFCompound_RemoveEntry(GHDFCompound* self, int id)
{
	long long TargetIndex = -1;

	for (long long i = 0; i < (long long)self->Count; i++)
	{
		if (self->Entries[i].ID == id)
		{
			TargetIndex = i;
			FreeEntryMemory(self->Entries + i);
			break;
		}
	}

	if (TargetIndex == -1)
	{
		return;
	}

	for (long long i = TargetIndex + 1; i < self->Count; i++)
	{
		self->Entries[i - 1] = self->Entries[i];
	}

	self->Count -= 1;
}

void GHDFCompound_Clear(GHDFCompound* self)
{
	for (size_t i = 0; i < self->Count; i++)
	{
		FreeEntryMemory(self->Entries + i);
	}

	self->Count = 0;
}

GHDFEntry* GHDFCompound_GetEntry(GHDFCompound* self, int id)
{
	for (size_t i = 0; i < self->Count; i++)
	{
		if (self->Entries[i].ID == id)
		{
			return self->Entries + i;
		}
	}

	return NULL;
}

GHDFEntry* GHDFCompound_GetEntryOrDefault(GHDFCompound* self, int id, GHDFEntry* defaultEntry)
{
	for (size_t i = 0; i < self->Count; i++)
	{
		if (self->Entries[i].ID == id)
		{
			return self->Entries + i;
		}
	}

	return defaultEntry;
}

GHDFCompound GHDFCompound_ReadFromFile(const char* path)
{

}

ErrorCode GHDFCompound_SaveToFile(GHDFCompound* compound, const char* path)
{

}

void GHDFCompound_Deconstruct(GHDFCompound* self)
{
	for (size_t i = 0; i < self->Count; i++)
	{
		FreeEntryMemory(self->Entries + i);
	}

	Memory_Free(self->Entries);
}