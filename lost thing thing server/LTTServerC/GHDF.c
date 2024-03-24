#include "GHDF.h"
#include "Memory.h"
#include "Directory.h"
#include "File.h"
#include <stdio.h>
#include <stdbool.h>

// Macros.
#define COMPOUND_CAPCITY_GROWHT 2
#define GHDF_SIGNATURE_BYTES 102, 37, 143, 181, 3, 205, 123, 185, 148, 157, 98, 177, 178, 151, 43, 170
#define UNDETERMINABLE_ENTRY_SIZE

#define GetValueType(type) ((type) & (~GHDF_TYPE_ARRAY_BIT))

#define ENCODED_INT_INDICATOR_BIT 0b10000000
#define ENCODED_INT_MASK 0b01111111
#define ENCODED_INT_BIT_COUNT_PER_BYTE 7



// Static functions.
/* Compound. */
static void EnsureCompoundCapacity(GHDFCompound* self, size_t capacity)
{
	if (self->_capacity >= capacity)
	{
		return;
	}

	if (self->_capacity == 0)
	{
		self->_capacity = 1;
	}

	while (self->_capacity < capacity)
	{
		self->_capacity *= COMPOUND_CAPCITY_GROWHT;
	} 

	self->Entries = (GHDFEntry*)Memory_SafeRealloc(self->Entries, sizeof(GHDFEntry) * self->_capacity);
}

static void FreeSingleValue(GHDFPrimitive value, GHDFType type)
{
	if (type == GHDFType_String)
	{
		Memory_Free(value.String);
	}
	else if (type == GHDFType_Compound)
	{
		GHDFCompound_Deconstruct(value.Compound);
		Memory_Free(value.Compound);
	}
}

static void FreeEntryMemory(GHDFEntry* entry)
{
	if (entry->ValueType & GHDF_TYPE_ARRAY_BIT)
	{
		for (size_t i = 0; i < entry->Value.ValueArray.Size; i++)
		{
			FreeSingleValue(entry->Value.ValueArray.Array[i], GetValueType(entry->ValueType));
		}
		Memory_Free(entry->Value.ValueArray.Array);
	}
	else
	{
		FreeSingleValue(entry->Value.SingleValue, entry->ValueType);
	}
}


/* Data. */
static size_t TryGetTypeSize(GHDFType type)
{
	switch (GetValueType(type))
	{
		case GHDFType_UByte:
			return GHDF_SIZE_UBYTE;

		case GHDFType_SByte:
			return GHDF_SIZE_SBYTE;

		case GHDFType_UShort:
			return GHDF_SIZE_USHORT;

		case GHDFType_Short:
			return GHDF_SIZE_SHORT;

		case GHDFType_UInt:
			return GHDF_SIZE_UINT;

		case GHDFType_Int:
			return GHDF_SIZE_INT;

		case GHDFType_ULong:
			return GHDF_SIZE_ULONG;

		case GHDFType_Long:
			return GHDF_SIZE_LONG;

		case GHDFType_Float:
			return GHDF_SIZE_FLOAT;

		case GHDFType_Double:
			return GHDF_SIZE_DOUBLE;

		case GHDFType_Bool:
			return GHDF_SIZE_BOOL;

		default:
			return UNDETERMINABLE_ENTRY_SIZE;
	}
}


/* Writing */
static ErrorCode WriteEntry(FILE* file, GHDFEntry* entry);

static ErrorCode WriteCompound(FILE* file, GHDFCompound* compound)
{
	if (Write7BitEncodedInt(file, (unsigned int)compound->Count) != ErrorCode_Success)
	{
		return Error_GetLastErrorCode();
	}

	for (size_t i = 0; i < compound->Count; i++)
	{
		if (WriteEntry(file, &compound->Entries[i]) != ErrorCode_Success)
		{
			return Error_GetLastErrorCode();
		}
	}

	return ErrorCode_Success;
}

static ErrorCode Write7BitEncodedInt(FILE* file, int integer)
{
	unsigned int Value = (unsigned int)integer;

	do
	{
		ErrorCode Result = File_WriteByte(file, (Value & ENCODED_INT_MASK) | (Value > ENCODED_INT_MASK ? ENCODED_INT_INDICATOR_BIT : 0));
		if (Result != ErrorCode_Success)
		{
			return Error_SetError(ErrorCode_IO, "Write7BitEncodedInt: Failed to write char.");
		}

		Value >>= ENCODED_INT_BIT_COUNT_PER_BYTE;
	} while (Value > 0);
}

static ErrorCode WriteMetadata(FILE* file)
{
	unsigned char Signature[] = { GHDF_SIGNATURE_BYTES };
	if (File_Write(file, Signature, sizeof(Signature)) != sizeof(Signature))
	{
		return Error_SetError(ErrorCode_IO, "WriteMetadata: Failed to write GHDF signature.");
	}
	
	int Version = GHDF_FORMAT_VERSION;
	if (File_Write(file, (char*)(&Version), GHDF_SIZE_INT) != GHDF_SIZE_INT)
	{
		return Error_SetError(ErrorCode_IO, "WriteMetadata: Failed to write format version.");
	}
	return ErrorCode_Success;
}

static ErrorCode WriteSingleValue(FILE* file, GHDFPrimitive value, GHDFType type)
{
	if (GetValueType(type) == GHDFType_String)
	{
		size_t StringLength = String_LengthBytes(value.String);

		if (Write7BitEncodedInt(file, (unsigned int)StringLength) != ErrorCode_Success)
		{
			return Error_GetLastErrorCode();
		}
		if (File_Write(file, value.String, StringLength) != ErrorCode_Success)
		{
			char Message[128];
			sprintf(Message, "WriteSingleValue: Failed to write string of size %d.", (int)StringLength);
			return Error_SetError(ErrorCode_IO, Message);
		}
	}
	else if (GetValueType(type) == GHDFType_Compound)
	{
		return WriteCompound(file, value.Compound);
	}
	else
	{
		size_t Size = TryGetTypeSize(type);
		if (File_Write(file, (const char*)(&value), Size) != ErrorCode_Success)
		{
			char Message[128];
			sprintf(Message, "WriteSingleValue: Failed to write simple entry with type %d and size %d.", (int)type, (int)Size);
			return Error_SetError(ErrorCode_IO, Message);
		}
	}

	return ErrorCode_Success;
}


static ErrorCode WriteArrayValue(FILE* file, GHDFArray array, GHDFType type)
{
	if (Write7BitEncodedInt(file, (unsigned int)array.Size) != ErrorCode_Success)
	{
		return Error_SetError(ErrorCode_IO, "WriteArrayValue: Failed to write array size.");
	}

	for (size_t i = 0; i < array.Size; i++)
	{
		if (WriteSingleValue(file, array.Array[i], type) != ErrorCode_Success) // Not optimal solution.
		{
			return Error_GetLastErrorCode();
		} 
	}

	return ErrorCode_Success;
}

static ErrorCode WriteEntry(FILE* file, GHDFEntry* entry)
{
	if (Write7BitEncodedInt(file, entry->ID) != ErrorCode_Success)
	{
		return Error_SetError(ErrorCode_IO, "WriteEntry: Failed to write entry's ID.");
	}
	if (File_WriteByte(file, (int)entry->ValueType) != ErrorCode_Success)
	{
		return Error_SetError(ErrorCode_IO, "WriteEntry: Failed to write entry's type.");
	}

	if (entry->ValueType & GHDF_TYPE_ARRAY_BIT)
	{
		return WriteArrayValue(file, entry->Value.ValueArray, entry->ValueType);
	}
	else
	{
		return WriteSingleValue(file, entry->Value.SingleValue, entry->ValueType);
	}
}



/* Reading. */
static ErrorCode ReadMetadata(FILE* file)
{
	unsigned char Signature[] = { GHDF_SIGNATURE_BYTES };
	unsigned char ReadSignature[sizeof(Signature)];

	if (File_Read(file, ReadSignature, sizeof(ReadSignature)) < sizeof(ReadSignature))
	{
		return Error_SetError(ErrorCode_InvalidGHDFFile, "Failed to verify GHDF meta-data: Signature too short.");
	}

	for (int i = 0; i < sizeof(Signature); i++)
	{
		if (Signature[i] != ReadSignature[i])
		{
			return Error_SetError(ErrorCode_InvalidGHDFFile, "Failed to verify GHDF meta-data: Wrong signature.");
		}
	}

	int Version;
	if (File_Read(file, (char*)(&Version), sizeof(GHDF_SIZE_INT)) != GHDF_SIZE_INT)
	{
		return Error_SetError(ErrorCode_InvalidGHDFFile, "Failed to verify GHDF meta-data: Version not an integer.");
	}

	return Version == GHDF_FORMAT_VERSION ? ErrorCode_Success 
		: Error_SetError(ErrorCode_InvalidGHDFFile, "Failed to verify GHDF meta-data: Unsupported format version.");;
}

static ErrorCode Read7BitEncodedInt(FILE* file, int* result)
{
	int Value = 0;
	int LastByte;
	int ShiftCount = 0;

	do
	{
		LastByte = File_ReadByte(file);

		if (Error_GetLastErrorCode() != ErrorCode_Success)
		{
			return Error_GetLastErrorCode();
		}

		Value |= (LastByte & ENCODED_INT_MASK) << (ENCODED_INT_BIT_COUNT_PER_BYTE * ShiftCount);
		ShiftCount++;
	} while (LastByte & ENCODED_INT_INDICATOR_BIT);

	*result = Value;
	return ErrorCode_Success;
}

static ErrorCode ReadSingleValue(FILE* file, GHDFType type, GHDFPrimitive* value)
{
	GHDFPrimitive Value;

	if (type == GHDFType_Compound)
	{
		Value.Compound = Memory_SafeMalloc(sizeof(GHDFCompound));
		ReadCompound(file, Value.Compound);
	}
	else if (type == GHDFType_String)
	{
		unsigned int StringLength;
		if (Read7BitEncodedInt(file, (unsigned int*)(&StringLength)) != ErrorCode_Success)
		{
			return Error_GetLastErrorCode();
		}
		Value.String = (char*)Memory_SafeMalloc((sizeof(char) * StringLength) + 1);
		if (File_Read(file, Value.String, StringLength) != StringLength)
		{
			Memory_Free(Value.String);
			return Error_SetError(ErrorCode_IO, "ReadSingleValue: Failed to read string.");
		}
		Value.String[StringLength] = '\0';
	}
	else
	{
		size_t TypeSize = TryGetTypeSize(type);
		if (File_Read(file, (char*)(&Value), TypeSize) != TypeSize)
		{
			return Error_SetError(ErrorCode_IO, "ReadSingleValue: Failed to read value.");
		}
	}

	*value = Value;
	return ErrorCode_Success;
}

static ErrorCode ReadArrayValue(FILE* file, GHDFType type, GHDFPrimitive** arrayPtr, size_t* arraySizePtr)
{
	unsigned int ArraySize;
	if (Read7BitEncodedInt(file, (unsigned int*)(&ArraySize)) != ErrorCode_Success)
	{
		return Error_SetError(ErrorCode_IO, "ReadArrayValue: Failed to read length of array");
	}

	GHDFPrimitive* PrimitiveArray = (GHDFPrimitive*)Memory_SafeMalloc(sizeof(GHDFPrimitive) * ArraySize);
	for (int i = 0; i < ArraySize; i++)
	{
		if (ReadSingleValue(file, type, PrimitiveArray + i) != ErrorCode_Success)
		{
			return Error_GetLastErrorCode();
		}
	}

	*arrayPtr = PrimitiveArray;
	*arraySizePtr = (size_t)ArraySize;
	return ErrorCode_Success;
}

static ErrorCode ReadEntryInfo(FILE* file, int* id, GHDFType* type)
{
	int ID;
	if (Read7BitEncodedInt(file, &ID) != ErrorCode_Success)
	{
		return Error_GetLastErrorCode();
	}
	if (ID == 0)
	{
		return Error_SetError(ErrorCode_InvalidGHDFFile, "An ID of 0 is not allowed.");
	}

	GHDFType EntryType = File_ReadByte(file);
	if (Error_GetLastErrorCode() != ErrorCode_Success)
	{
		return Error_GetLastErrorCode();
	}
	if ((GetValueType(EntryType) <= GHDFType_None) || (GetValueType(EntryType) > GHDFType_Compound))
	{
		char Message[128];
		sprintf(Message, "Invalid data type: %d", (int)GetValueType(EntryType));
		return Error_SetError(ErrorCode_InvalidGHDFFile, Message);
	}

	*id = ID;
	*type = EntryType;
	return ErrorCode_Success;
}

static ErrorCode ReadEntryValue(FILE* file, int id, GHDFType type, GHDFCompound* compound)
{
	if (type & GHDF_TYPE_ARRAY_BIT)
	{
		GHDFPrimitive* Array;
		size_t ArraySize;
		if (ReadArrayValue(file, type, &Array, &ArraySize) != ErrorCode_Success)
		{
			return Error_GetLastErrorCode();
		}
		GHDFCompound_AddArrayEntry(compound, type, id, Array, ArraySize);
	}
	else
	{
		GHDFPrimitive Value;
		if (ReadSingleValue(file, type, &Value) != ErrorCode_Success)
		{
			return Error_GetLastErrorCode();
		}
		GHDFCompound_AddSingleValueEntry(compound, type, id, Value);
	}

	return ErrorCode_Success;
}

static ErrorCode ReadEntry(FILE* file, GHDFCompound* compound)
{
	int ID;
	GHDFType EntryType;
	if (ReadEntryInfo(file, &ID, &EntryType) != ErrorCode_Success)
	{
		return Error_GetLastErrorCode();
	}

	return ReadEntryValue(file, ID, EntryType, compound);
}

static ErrorCode ReadCompound(FILE* file, GHDFCompound* compound)
{
	GHDFCompound_Construct(compound, COMPOUND_DEFAULT_CAPACITY);

	int EntryCount;
	if (Read7BitEncodedInt(file, &EntryCount) != ErrorCode_Success)
	{
		return Error_GetLastErrorCode();
	}

	for (int i = 0; i < EntryCount; i++)
	{
		if (ReadEntry(file, compound) != ErrorCode_Success)
		{
			return Error_GetLastErrorCode();
		}
	}

	return ErrorCode_Success;
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

GHDFCompound* GHDFCompound_Construct2(size_t capacity)
{
	GHDFCompound* Compound = (GHDFCompound*)Memory_SafeMalloc(sizeof(GHDFCompound));
	GHDFCompound_Construct(Compound, capacity);
	return Compound;
}

ErrorCode GHDFCompound_AddSingleValueEntry(GHDFCompound* self, GHDFType type, int id, GHDFPrimitive value)
{
	if (id == 0)
	{
		return Error_SetError(ErrorCode_InvalidArgument, "GHDFCompound_AddSingleValueEntry: An ID of 0 is not allowed.");
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
		return Error_SetError(ErrorCode_InvalidArgument, "GHDFCompound_AddArrayEntry: An ID of 0 is not allowed.");
	}

	EnsureCompoundCapacity(self, self->Count + 1);
	self->Entries[self->Count].ID = id;
	self->Entries[self->Count].Value.ValueArray.Array = valueArray;
	self->Entries[self->Count].Value.ValueArray.Size = count;
	self->Entries[self->Count].ValueType = (type | GHDF_TYPE_ARRAY_BIT);

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
	GHDFCompound_Construct(emptyBaseCompound, COMPOUND_DEFAULT_CAPACITY);
	FILE* File = File_Open(path, FileOpenMode_ReadBinary);
	if (!File)
	{
		return Error_SetError(ErrorCode_IO, "GHDFCompound_ReadFromFile: Failed to open file");
	}

	if (ReadMetadata(File) != ErrorCode_Success)
	{
		File_Close(File);
		return Error_GetLastErrorCode();
	}
	
	if (ReadCompound(File, emptyBaseCompound) != ErrorCode_Success)
	{
		File_Close(File);
		GHDFCompound_Deconstruct(emptyBaseCompound);
		return Error_GetLastErrorCode();
	}

	File_Close(File);
}

ErrorCode GHDFCompound_WriteToFile(const char* path, GHDFCompound* compound)
{
	const char* Path = Directory_ChangePathExtension(path, GHDF_FILE_EXTENSION);

	File_Delete(Path);

	FILE* File = File_Open(Path, FileOpenMode_WriteBinary);
	Memory_Free(Path);

	if (!File)
	{
		return Error_GetLastErrorCode();
	}

	if (WriteMetadata(File) != ErrorCode_Success)
	{
		File_Close(File);
		return Error_GetLastErrorCode();
	}

	if (WriteCompound(File, compound) != ErrorCode_Success)
	{
		File_Close(File);
		return Error_GetLastErrorCode();
	}

	File_Close(File);
	return ErrorCode_Success;
}

void GHDFCompound_Deconstruct(GHDFCompound* self)
{
	for (size_t i = 0; i < self->Count; i++)
	{
		FreeEntryMemory(self->Entries + i);
	}

	Memory_Free(self->Entries);
}