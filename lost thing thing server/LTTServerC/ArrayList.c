#include "ArrayList.h"
#include "Memory.h"
#include "ErrorCodes.h"
#include <stdlib.h>

#define DEFAULT_CAPACITY 16
#define CAPACITY_GROWTH 4

// Functions.
// All objects stored in ArrayList must be on the heap.
int ArrayList_Construct(ArrayList* arrayList)
{
	if (arrayList == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	void** Data = SafeMalloc(sizeof(void*) * DEFAULT_CAPACITY);

	arrayList->Data = Data;
	arrayList->Length = 0;
	arrayList->_capacity = DEFAULT_CAPACITY;

	return 0;
}

ArrayList* ArrayList_Construct2()
{
	ArrayList* List = SafeMalloc(sizeof(ArrayList));
	ArrayList_Construct(List);
	return List;
}

int ArrayList_Deconstruct(ArrayList* this)
{
	if (this == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	ArrayList_Clear(this);
	free(this->Data);
	return 0;
}

static void ArrayList_EnsureCapacity(ArrayList* this, int capacity)
{
	if (capacity <= this->_capacity)
	{
		return;
	}

	this->_capacity *= CAPACITY_GROWTH;
	this->Data = SafeRealloc(this->Data, sizeof(void*) * this->_capacity);
}

int ArrayList_Add(ArrayList* this, void* objectPointer)
{
	if (this == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	ArrayList_EnsureCapacity(this, this->Length + 1);
	this->Data[this->Length] = objectPointer;
	this->Length++;

	return 0;
}

int ArrayList_Insert(ArrayList* this, void* objectPointer, int index)
{
	if (this == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}
	if (index < 0)
	{
		return INDEX_OUT_OF_RANGE_ERRCODE;
	}
	if (index == this->Length)
	{
		return ArrayList_Add(this, objectPointer);
	}

	ArrayList_EnsureCapacity(this, this->Length + 1);

	for (int i = this->Length; i > index; i--)
	{
		this->Data[i] = this->Data[i - 1];
	}
	this->Data[index] = objectPointer;

	this->Length++;
	return 0;
}

int ArrayList_Remove(ArrayList* this, void* objectPointer)
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

int ArrayList_RemoveAt(ArrayList* this, int index)
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

int ArrayList_RemoveLast(ArrayList* this)
{
	if (this == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	ArrayList_RemoveAt(this, this->Length - 1);
}

int ArrayList_RemoveFirst(ArrayList* this)
{
	ArrayList_RemoveAt(this, 0);
}

int ArrayList_Clear(ArrayList* this)
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
}