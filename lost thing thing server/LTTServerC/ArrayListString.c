#include "ArrayListString.h"
#include "Memory.h"
#include "Errors.h"
#include <stdlib.h>

#define ARRAYLIST_CAPACITY_GROWTH 4

// Functions.
// All objects stored in ArrayList must be on the heap.
void ArrayListString_Construct(ArrayListString* self, size_t capacity)
{
	if (capacity < 1)
	{
		capacity = ARRAYLIST_STRING_DEFAULT_CAPACITY;
	}

	self->Data = (char*)Memory_SafeMalloc(sizeof(char*) * capacity);
	self->Length = 0;
	self->_capacity = capacity;

	return 0;
}

ArrayListString* ArrayListString_Construct2(size_t capacity)
{
	ArrayList* List = Memory_SafeMalloc(sizeof(ArrayList));
	ArrayList_Construct(List, capacity);
	return List;
}

void ArrayLisStringt_Deconstruct(ArrayListString* this)
{
	if (this == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	ArrayList_Clear(this);
	free(this->Data);
	return 0;
}

static void EnsureCapacity(ArrayListString* this, int capacity)
{
	if (capacity <= this->_capacity)
	{
		return;
	}

	this->_capacity *= ARRAYLIST_CAPACITY_GROWTH;
	this->Data = Memory_SafeRealloc(this->Data, sizeof(void*) * this->_capacity);
}

void ArrayListString_Add(ArrayListString* this, void* objectPointer)
{
	EnsureCapacity(this, this->Length + 1);
	this->Data[this->Length] = objectPointer;
	this->Length++;

	return 0;
}

ErrorCode ArrayListString_Insert(ArrayListString* this, void* objectPointer, int index)
{
	if (index == this->Length)
	{
		return ArrayList_Add(this, objectPointer);
	}

	EnsureCapacity(this, this->Length + 1);

	for (int i = this->Length; i > index; i--)
	{
		this->Data[i] = this->Data[i - 1];
	}
	this->Data[index] = objectPointer;

	this->Length++;
	return 0;
}

int ArrayListString_Remove(ArrayListString* this, void* objectPointer)
{
	if (this == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	for (int i = 0; i < this->Length; i++)
	{
		if (this->Data[i] == objectPointer)
		{
			return ArrayList_RemoveAt(this, i);
		}
	}

	return 0;
}

int ArrayListString_RemoveAt(ArrayListString* this, int index)
{
	if (this == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}
	if ((index < 0) || (index >= this->Length))
	{
		return INDEX_OUT_OF_RANGE_ERRCODE;
	}

	free(this->Data[index]);
	this->Data[index] = NULL;

	for (int i = index + 1; i < this->Length; i++)
	{
		this->Data[i - 1] = this->Data[i];
	}

	this->Length--;
	return 0;
}

int ArrayListString_RemoveLast(ArrayListString* this)
{
	if (this == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	ArrayList_RemoveAt(this, this->Length - 1);

	return 0;
}

int ArrayListString_RemoveFirst(ArrayListString* this)
{
	return ArrayList_RemoveAt(this, 0);
}

int ArrayListString_Clear(ArrayListString* this)
{
	if (this == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	for (int i = 0; i < this->Length; i++)
	{
		free(this->Data[i]);
		this->Data[i] = NULL;
	}

	return 0;
}