#include "GHDF.h"
#include "Memory.h"
#include "Directory.h"
#include "File.h"
#include <stdio.h>

// Macros.
#define COMPOUND_CAPCITY_GROWHT 2
#define GHDF_FILE_EXTENSTION ".ghdf"
#define GHDF_FORMAT_VERSION 1
#define GHDF_SIGNATURE_BYTES 102, 37, 143, 181, 3, 205, 123, 185, 148, 157, 98, 177, 178, 151, 43, 170

#define ONE_BYTE_UTF8_BYTE 0b00000000
#define TWO_BYTE_UTF8_BYTE 0b11000000
#define THREE_BYTE_UTF8_BYTE 0b11100000
#define FOUR_BYTE_UTF8_BYTE 0b11110000

#define ONE_BYTE_UTF8_MASK 0b10000000
#define TWO_BYTE_UTF8_MASK 0b11100000
#define THREE_BYTE_UTF8_MASK 0b11110000
#define FOUR_BYTE_UTF8_MASK 0b11111000



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

static bool IsIDAlreadyPresent(GHDFCompound* self, int id)
{
	for (size_t i = 0; i < self->Count; i++)
	{
		if (self->Entries[i].ID == id)
		{
			return true;
		}
	}

	return false;
}


/* File IO. */
static ErrorCode Write7BitEncodedInt(FILE* file, int integer)
{
	unsigned long long Value = (unsigned long long) integer;

	do
	{
		int Result = fputc((Value & 0b01111111) | (Value > 0xff ? 0b10000000 : 0), file);
		if (Result == EOF)
		{
			return ErrorContext_SetError(ErrorCode_IO, "Write7BitEncodedInt: Failed to write char.");
		}

		Value >>= 7;
	} while (Value > 0);

	return ErrorCode_Success;
}

static ErrorCode Read7BitEncodedInt(FILE* file, int* result)
{
	int Value = 0;
	int LastByte;

	do
	{
		LastByte = fgetc(file);
		if (LastByte == EOF)
		{
			return ErrorContext_SetError(ErrorCode_IO, "Read7BitEncodedInt: Failed to read char.");
		}

		Value = (Value << 7) | (LastByte & 0b01111111);
	} while (LastByte & 0b10000000);

	*result = Value;
	return ErrorCode_Success;
}

static ErrorCode WriteValue(FILE* file, GHDFPrimitive primitive, GHDFType type)
{
	switch (type)
	{
		case GHDFType_SByte:
		case GHDFType_UByte:
			return fputc(primitive.SByte, file) != EOF ? ErrorCode_Success
				: ErrorContext_SetError(ErrorCode_IO, "WriteValue: Failed to write SChar or UChar");

		case GHDFType_Short:
		case GHDFType_UShort:
			return fwrite(&primitive.Short, sizeof(short), 1, file) != EOF ? ErrorCode_Success
				: ErrorContext_SetError(ErrorCode_IO, "WriteValue: Failed to write Short or UShort");

		case GHDFType_Int:
		case GHDFType_UInt:
			return fwrite(&primitive.Int, sizeof(int), 1, file) != EOF ? ErrorCode_Success
				: ErrorContext_SetError(ErrorCode_IO, "WriteValue: Failed to write Int or UInt");

		case GHDFType_Long:
		case GHDFType_ULong:
			return fwrite(&primitive.Long, sizeof(long long), 1, file) != EOF ? ErrorCode_Success
				: ErrorContext_SetError(ErrorCode_IO, "WriteValue: Failed to write Long or ULong");

		case GHDFType_Float:
			return fwrite(&primitive.Float, sizeof(float), 1, file) != EOF ? ErrorCode_Success
				: ErrorContext_SetError(ErrorCode_IO, "WriteValue: Failed to write float");

		case GHDFType_Double:
			return fwrite(&primitive.Double, sizeof(double), 1, file) != EOF ? ErrorCode_Success
				: ErrorContext_SetError(ErrorCode_IO, "WriteValue: Failed to write double");

		case GHDFType_Bool:
			return fwrite(&primitive.Bool, sizeof(bool), 1, file) != EOF ? ErrorCode_Success
				: ErrorContext_SetError(ErrorCode_IO, "WriteValue: Failed to write bool");

		case GHDFType_Char:
			int Count;

			if ((primitive.Char & FOUR_BYTE_UTF8_MASK) == FOUR_BYTE_UTF8_BYTE)
			{
				Count = 4;
			}
			else if ((primitive.Char & THREE_BYTE_UTF8_MASK) == THREE_BYTE_UTF8_BYTE)
			{
				Count = 3;
			}
			else if ((primitive.Char & TWO_BYTE_UTF8_MASK) == TWO_BYTE_UTF8_BYTE)
			{
				Count = 2;
			}
			else
			{
				Count = 1;
			}
			return fwrite(&primitive.Char, sizeof(char), Count, file) != EOF ? ErrorCode_Success
				: ErrorContext_SetError(ErrorCode_IO, "WriteValue: Failed to write char");
	}
}

static ErrorCode WriteArray(FILE* file, GHDFPrimitive* primitiveArray, size_t arraySize, GHDFType type)
{

}

static ErrorCode WriteCompound(FILE* file, GHDFCompound* compound)
{
	for (int i = 0; i < compound->Count; i++)
	{
		GHDFEntry* Entry = compound->Entries + i;
		if (Write7BitEncodedInt(file, Entry->ID) != ErrorCode_Success)
		{
			return ErrorContext_GetLastErrorCode();
		}

		if (fputc(Entry->ValueType, file) == EOF)
		{
			return ErrorContext_SetError(ErrorCode_IO, "WriteCompound: Failed to write data type.");
		}

		if (Entry->ValueType & GHDT_TYPE_ARRAY_BIT)
		{
			return WriteArray();
		}
		else
		{
			return WriteValue(file, Entry->Value.SingleValue, Entry->ValueType);
		}
	}
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
	if (IsIDAlreadyPresent(self, id))
	{
		char ErrorMessage[128];
		snprintf(ErrorMessage, sizeof(ErrorMessage), "GHDFCompound_AddSingleValueEntry: ID of %d is already present in the compound.", id);
		return ErrorContext_SetError(ErrorCode_InvalidArgument, ErrorMessage);
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
	if (IsIDAlreadyPresent(self, id))
	{
		char ErrorMessage[128];
		snprintf(ErrorMessage, sizeof(ErrorMessage), "GHDFCompound_AddArrayEntry: ID of %d is already present in the compound.", id);
		return ErrorContext_SetError(ErrorCode_InvalidArgument, ErrorMessage);
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

ErrorCode GHDFCompound_ReadFromFile(const char* path, GHDFCompound* emptyBaseCompound)
{
	

}

ErrorCode GHDFCompound_SaveToFile(GHDFCompound* compound, const char* path)
{
	FILE* File = File_Open(path, FileOpenMode_Write);
	if (!File)
	{
		return ErrorContext_GetLastErrorCode();
	}

	unsigned char Signature[] = { GHDF_SIGNATURE_BYTES };

	ErrorCode Result = File_Write(File, Signature, sizeof(Signature));
	if (Result != ErrorCode_Success)
	{
		File_Close(File);
		return Result;
	}

	Result = File_Write(File, GHDF_FORMAT_VERSION, sizeof(int));
	if (Result != ErrorCode_Success)
	{
		File_Close(File);
		return Result;
	}

	Result = WriteCompound(File, compound);
	File_Close(File);
	return Result;
}

void GHDFCompound_Deconstruct(GHDFCompound* self)
{
	for (size_t i = 0; i < self->Count; i++)
	{
		FreeEntryMemory(self->Entries + i);
	}

	Memory_Free(self->Entries);
}