#include "GHDF.h"
#include "Memory.h"
#include "Directory.h"
#include "File.h"
#include <stdio.h>
#include <stdbool.h>

// Macros.
#define COMPOUND_CAPCITY_GROWHT 2
#define GHDF_SIGNATURE_BYTES 102, 37, 143, 181, 3, 205, 123, 185, 148, 157, 98, 177, 178, 151, 43, 170
#define UNDETERMINABLE_ENTRY_SIZE 0

#define GetValueType(type) ((type) & (~GHDF_TYPE_ARRAY_BIT))

#define ENCODED_INT_INDICATOR_BIT 0b10000000
#define ENCODED_INT_MASK 0b01111111
#define ENCODED_INT_BIT_COUNT_PER_BYTE 7



// Static functions.
/* Compound. */
static void EnsureCompoundCapacity(GHDFCompound* self, unsigned int capacity)
{
	if (self->_capacity >= capacity)
	{
		return;
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
		for (unsigned int i = 0; i < entry->Value.ValueArray.Size; i++)
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
static Error WriteEntry(FILE* file, GHDFEntry* entry);

static Error Write7BitEncodedInt(FILE* file, int integer)
{
	unsigned int Value = (unsigned int)integer;

	do
	{
		Error ReturnedError = File_WriteByte(file, (Value & ENCODED_INT_MASK) | (Value > ENCODED_INT_MASK ? ENCODED_INT_INDICATOR_BIT : 0));
		if (ReturnedError.Code != ErrorCode_Success)
		{
			return ReturnedError;
		}

		Value >>= ENCODED_INT_BIT_COUNT_PER_BYTE;
	} while (Value > 0);

	return Error_CreateSuccess();
}

static Error WriteCompound(FILE* file, GHDFCompound* compound)
{
	Error ReturnedError = Write7BitEncodedInt(file, (unsigned int)compound->Count);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	for (unsigned int i = 0; i < compound->Count; i++)
	{
		ReturnedError = WriteEntry(file, &compound->Entries[i]);
		if (ReturnedError.Code != ErrorCode_Success)
		{
			return ReturnedError;
		}
	}

	return Error_CreateSuccess();
}

static Error WriteMetadata(FILE* file)
{
	unsigned const char Signature[] = { GHDF_SIGNATURE_BYTES };
	Error ReturnedError = File_Write(file, (const char*)Signature, sizeof(Signature));

	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	
	int Version = GHDF_FORMAT_VERSION;
	ReturnedError = File_Write(file, (char*)(&Version), GHDF_SIZE_INT);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	return Error_CreateSuccess();
}

static Error WriteSingleValue(FILE* file, GHDFPrimitive value, GHDFType type)
{
	Error ReturnedError;
	if (GetValueType(type) == GHDFType_String)
	{
		unsigned int StringLength = (unsigned int)String_LengthBytes(value.String);
		ReturnedError = Write7BitEncodedInt(file, (unsigned int)StringLength);
		if (ReturnedError.Code != ErrorCode_Success)
		{
			return ReturnedError;
		}
		ReturnedError = File_Write(file, value.String, StringLength);
		if (ReturnedError.Code != ErrorCode_Success)
		{
			return ReturnedError;
		}
	}
	else if (GetValueType(type) == GHDFType_Compound)
	{
		return WriteCompound(file, value.Compound);
	}
	else
	{
		size_t Size = TryGetTypeSize(type);
		ReturnedError = File_Write(file, (const char*)(&value), Size);
		if (ReturnedError.Code != ErrorCode_Success)
		{
			return ReturnedError;
		}
	}

	return Error_CreateSuccess();
}


static Error WriteArrayValue(FILE* file, GHDFArray array, GHDFType type)
{
	Error ReturnedError = Write7BitEncodedInt(file, (unsigned int)array.Size);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	for (unsigned int i = 0; i < array.Size; i++)
	{
		ReturnedError = WriteSingleValue(file, array.Array[i], type); // Not optimal solution for writing in terms of performance.
		if (ReturnedError.Code != ErrorCode_Success) 
		{
			return ReturnedError;
		} 
	}

	return Error_CreateSuccess();
}

static Error WriteEntry(FILE* file, GHDFEntry* entry)
{
	Error ReturnedError = Write7BitEncodedInt(file, entry->ID);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	ReturnedError = File_WriteByte(file, (int)entry->ValueType);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	if (entry->ValueType & GHDF_TYPE_ARRAY_BIT)
	{
		return WriteArrayValue(file, entry->Value.ValueArray, entry->ValueType);
	}
	return WriteSingleValue(file, entry->Value.SingleValue, entry->ValueType);
}



/* Reading. */
static Error ReadCompound(FILE* file, GHDFCompound* compound);

static Error ReadMetadata(FILE* file)
{
	unsigned char Signature[] = { GHDF_SIGNATURE_BYTES };
	unsigned char ReadSignature[sizeof(Signature)];

	if (File_Read(file, (char*)ReadSignature, sizeof(ReadSignature)) < sizeof(ReadSignature))
	{
		return Error_CreateError(ErrorCode_InvalidGHDFFile, "Failed to verify GHDF meta-data: Signature too short.");
	}

	for (int i = 0; i < sizeof(Signature); i++)
	{
		if (Signature[i] != ReadSignature[i])
		{
			return Error_CreateError(ErrorCode_InvalidGHDFFile, "Failed to verify GHDF meta-data: Wrong signature.");
		}
	}

	int Version;
	if (File_Read(file, (char*)(&Version), sizeof(GHDF_SIZE_INT)) != GHDF_SIZE_INT)
	{
		return Error_CreateError(ErrorCode_InvalidGHDFFile, "Failed to verify GHDF meta-data: Version not an integer.");
	}

	return Version == GHDF_FORMAT_VERSION ? Error_CreateSuccess()
		: Error_CreateError(ErrorCode_InvalidGHDFFile, "Failed to verify GHDF meta-data: Unsupported format version.");;
}

static Error Read7BitEncodedInt(FILE* file, int* result)
{
	int Value = 0;
	int LastByte;
	int ShiftCount = 0;

	do
	{
		Error ReturnedError;
		LastByte = File_ReadByte(file, &ReturnedError);

		if (ReturnedError.Code != ErrorCode_Success)
		{
			return ReturnedError;
		}

		Value |= (LastByte & ENCODED_INT_MASK) << (ENCODED_INT_BIT_COUNT_PER_BYTE * ShiftCount);
		ShiftCount++;
	} while (LastByte & ENCODED_INT_INDICATOR_BIT);

	*result = Value;
	return Error_CreateSuccess();
}

static Error ReadSingleValue(FILE* file, GHDFType type, GHDFPrimitive* value)
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
		Error ReturnedError = Read7BitEncodedInt(file, (int*)(&StringLength));
		if (ReturnedError.Code != ErrorCode_Success)
		{
			return ReturnedError;
		}
		Value.String = (char*)Memory_SafeMalloc((sizeof(char) * StringLength) + 1);
		if (File_Read(file, Value.String, StringLength) != StringLength)
		{
			Memory_Free(Value.String);
			return Error_CreateError(ErrorCode_IO, "ReadSingleValue: Failed to read string.");
		}
		Value.String[StringLength] = '\0';
	}
	else
	{
		size_t TypeSize = TryGetTypeSize(type);
		if (File_Read(file, (char*)(&Value), TypeSize) != TypeSize)
		{
			return Error_CreateError(ErrorCode_IO, "ReadSingleValue: Failed to read value.");
		}
	}

	*value = Value;
	return Error_CreateSuccess();
}

static Error ReadArrayValue(FILE* file, GHDFType type, GHDFPrimitive** arrayPtr, unsigned int* arraySizePtr)
{
	unsigned int ArraySize;
	Error ReturnedError = Read7BitEncodedInt(file, (int*)(&ArraySize));

	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	GHDFPrimitive* PrimitiveArray = (GHDFPrimitive*)Memory_SafeMalloc(sizeof(GHDFPrimitive) * ArraySize);
	for (unsigned int i = 0; i < ArraySize; i++)
	{
		ReturnedError = ReadSingleValue(file, GetValueType(type), PrimitiveArray + i);
		if (ReturnedError.Code != ErrorCode_Success)
		{
			ReturnedError;
		}
	}

	*arrayPtr = PrimitiveArray;
	*arraySizePtr = ArraySize;
	return Error_CreateSuccess();
}

static Error ReadEntryInfo(FILE* file, int* id, GHDFType* type)
{
	int ID;

	Error ReturnedError = Read7BitEncodedInt(file, &ID);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	if (ID == 0)
	{
		return Error_CreateError(ErrorCode_InvalidGHDFFile, "An ID of 0 is not allowed.");
	}

	GHDFType EntryType = File_ReadByte(file, &ReturnedError);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	if ((GetValueType(EntryType) <= GHDFType_None) || (GetValueType(EntryType) > GHDFType_Compound))
	{
		char Message[128];
		sprintf(Message, "Invalid data type: %d", (int)GetValueType(EntryType));
		return Error_CreateError(ErrorCode_InvalidGHDFFile, Message);
	}
	*id = ID;
	*type = EntryType;

	return Error_CreateSuccess();
}

static Error ReadEntryValue(FILE* file, int id, GHDFType type, GHDFCompound* compound)
{
	Error ReturnedError;

	if (type & GHDF_TYPE_ARRAY_BIT)
	{
		GHDFPrimitive* Array;
		unsigned int ArraySize;
		ReturnedError = ReadArrayValue(file, type, &Array, &ArraySize);
		if (ReturnedError.Code != ErrorCode_Success)
		{
			return ReturnedError;
		}
		GHDFCompound_AddArrayEntry(compound, type, id, Array, ArraySize);
	}
	else
	{
		GHDFPrimitive Value;
		ReturnedError = ReadSingleValue(file, type, &Value);
		if (ReturnedError.Code != ErrorCode_Success)
		{
			return ReturnedError;
		}
		GHDFCompound_AddSingleValueEntry(compound, type, id, Value);
	}

	return Error_CreateSuccess();
}

static Error ReadEntry(FILE* file, GHDFCompound* compound)
{
	int ID;
	GHDFType EntryType;
	Error ReturnedError = ReadEntryInfo(file, &ID, &EntryType);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	return ReadEntryValue(file, ID, EntryType, compound);
}

static Error ReadCompound(FILE* file, GHDFCompound* compound)
{
	GHDFCompound_Construct(compound, COMPOUND_DEFAULT_CAPACITY);

	int EntryCount;
	Error ReturnedError = Read7BitEncodedInt(file, &EntryCount);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	for (int i = 0; i < EntryCount; i++)
	{
		ReturnedError = ReadEntry(file, compound);
		if (ReturnedError.Code != ErrorCode_Success)
		{
			return ReturnedError;
		}
	}

	return Error_CreateSuccess();
}


// Functions.
void GHDFCompound_Construct(GHDFCompound* self, unsigned int capacity)
{
	if (capacity <= 0)
	{
		capacity = 1;
	}

	self->Entries = (GHDFEntry*)Memory_SafeMalloc(sizeof(GHDFEntry) * capacity);
	self->Count = 0;
	self->_capacity = capacity;
}

GHDFCompound* GHDFCompound_Construct2(unsigned int capacity)
{
	GHDFCompound* Compound = (GHDFCompound*)Memory_SafeMalloc(sizeof(GHDFCompound));
	GHDFCompound_Construct(Compound, capacity);
	return Compound;
}

Error GHDFCompound_AddSingleValueEntry(GHDFCompound* self, GHDFType type, int id, GHDFPrimitive value)
{
	if (id == 0)
	{
		return Error_CreateError(ErrorCode_InvalidArgument, "GHDFCompound_AddSingleValueEntry: An ID of 0 is not allowed.");
	}

	EnsureCompoundCapacity(self, self->Count + 1);
	self->Entries[self->Count].ID = id;
	self->Entries[self->Count].Value.SingleValue = value;
	self->Entries[self->Count].ValueType = type;

	self->Count += 1;
	return Error_CreateSuccess();
}

Error GHDFCompound_AddArrayEntry(GHDFCompound* self, GHDFType type, int id, GHDFPrimitive* valueArray, unsigned int count)
{
	if (id == 0)
	{
		return Error_CreateError(ErrorCode_InvalidArgument, "GHDFCompound_AddArrayEntry: An ID of 0 is not allowed.");
	}

	EnsureCompoundCapacity(self, self->Count + 1);
	self->Entries[self->Count].ID = id;
	self->Entries[self->Count].Value.ValueArray.Array = valueArray;
	self->Entries[self->Count].Value.ValueArray.Size = count;
	self->Entries[self->Count].ValueType = (type | GHDF_TYPE_ARRAY_BIT);

	self->Count += 1;
	return Error_CreateSuccess();
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

	if (TargetIndex <= -1)
	{
		return;
	}

	for (unsigned int i = (unsigned int)TargetIndex + 1; i < self->Count; i++)
	{
		self->Entries[i - 1] = self->Entries[i];
	}

	self->Count -= 1;
}

void GHDFCompound_Clear(GHDFCompound* self)
{
	for (unsigned int i = 0; i < self->Count; i++)
	{
		FreeEntryMemory(self->Entries + i);
	}

	self->Count = 0;
}

GHDFEntry* GHDFCompound_GetEntry(GHDFCompound* self, int id)
{
	for (unsigned int i = 0; i < self->Count; i++)
	{
		if (self->Entries[i].ID == id)
		{
			return self->Entries + i;
		}
	}

	return NULL;
}

Error GHDFCompound_GetVerifiedEntry(GHDFCompound* self, int id, GHDFEntry** entry, GHDFType expectedType, const char* optionalMessage)
{
	GHDFEntry* FoundEntry = GHDFCompound_GetEntry(self, id);
	char Message[256];

	if (!FoundEntry)
	{
		snprintf(Message, sizeof(Message), "Expected GHDF entry of type %d with id %d, found no such entry. %s",
			(int)expectedType, id, optionalMessage ? optionalMessage : "No further information");
		return Error_CreateError(ErrorCode_InvalidGHDFFile, Message);
	}
	if (FoundEntry->ValueType != expectedType)
	{
		snprintf(Message, sizeof(Message), "Expected GHDF entry of type %d with id %d, actual type is %d. %s",
			(int)expectedType, id, (int)FoundEntry->ValueType, optionalMessage ? optionalMessage : "No further information");
		return Error_CreateError(ErrorCode_InvalidGHDFFile, Message);
	}

	*entry = FoundEntry;
	return Error_CreateSuccess();
}

Error GHDFCompound_GetVerifiedOptionalEntry(GHDFCompound* self, int id, GHDFEntry** entry, GHDFType expectedType, const char* optionalMessage)
{
	GHDFEntry* FoundEntry = GHDFCompound_GetEntry(self, id);
	char Message[256];
	*entry = NULL;

	if (!FoundEntry)
	{
		return Error_CreateSuccess();
	}
	if (FoundEntry->ValueType != expectedType)
	{
		snprintf(Message, sizeof(Message), "Expected GHDF entry of type %d with id %d, actual type is %d. %s",
			(int)expectedType, id, (int)FoundEntry->ValueType, optionalMessage ? optionalMessage : "No further information");
		return Error_CreateError(ErrorCode_InvalidGHDFFile, Message);
	}

	*entry = FoundEntry;
	return Error_CreateSuccess();
}

GHDFEntry* GHDFCompound_GetEntryOrDefault(GHDFCompound* self, int id, GHDFEntry* defaultEntry)
{
	for (unsigned int i = 0; i < self->Count; i++)
	{
		if (self->Entries[i].ID == id)
		{
			return self->Entries + i;
		}
	}

	return defaultEntry;
}

Error GHDFCompound_ReadFromFile(const char* path, GHDFCompound* emptyBaseCompound)
{
	GHDFCompound_Construct(emptyBaseCompound, COMPOUND_DEFAULT_CAPACITY);

	if (!File_Exists(path))
	{
		return Error_CreateError(ErrorCode_IO, "GHDFCompound_ReadFromFile: Provided GHDF file doesn't exist.");
	}

	Error ReturnedError;
	FILE* File = File_Open(path, FileOpenMode_ReadBinary, &ReturnedError);
	if (!File)
	{
		return ReturnedError;
	}

	ReturnedError = ReadMetadata(File);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		File_Close(File);
		return ReturnedError;
	}
	
	ReturnedError = ReadCompound(File, emptyBaseCompound);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		File_Close(File);
		GHDFCompound_Deconstruct(emptyBaseCompound);
		return ReturnedError;
	}

	File_Close(File);
	return Error_CreateSuccess();
}

Error GHDFCompound_WriteToFile(const char* path, GHDFCompound* compound)
{
	const char* Path = Directory_ChangePathExtension(path, GHDF_FILE_EXTENSION);

	const char* DirectoryPath = Directory_GetParentDirectory(Path);
	Directory_CreateAll(DirectoryPath);
	Memory_Free((char*)DirectoryPath);

	File_Delete(Path);

	Error ReturnedError;
	FILE* File = File_Open(Path, FileOpenMode_WriteBinary, &ReturnedError);
	Memory_Free((char*)Path);

	if (!File)
	{
		return ReturnedError;
	}

	ReturnedError = WriteMetadata(File);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		File_Close(File);
		return ReturnedError;
	}

	ReturnedError = WriteCompound(File, compound);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		File_Close(File);
		return ReturnedError;
	}

	File_Close(File);
	return Error_CreateSuccess();
}

void GHDFCompound_Deconstruct(GHDFCompound* self)
{
	for (unsigned int i = 0; i < self->Count; i++)
	{
		FreeEntryMemory(self->Entries + i);
	}

	Memory_Free(self->Entries);
}