#include "Dictionary.h"
#include "ErrorCodes.h"
#include "Memory.h"
#include "ArrayList.h"
#include <stdlib.h>

#define DEFAULT_DICTIONARY_CAPACITY 16
#define DICTIONARY_CAPACITY_GROWTH 2

// Structures.
struct DictionaryStruct
{
	ArrayList* _data;
	size_t _capacity;
};


// Functions.
int Dictionary_Construct(Dictionary* dictionary, int capacity)
{
	if (dictionary == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}
	if (capacity < 1)
	{
		capacity = DEFAULT_DICTIONARY_CAPACITY;
	}

	dictionary->_capacity = capacity;
	dictionary->_data = SafeMalloc(sizeof(Dictionary) * capacity);

	return 0;
}

Dictionary* Dictionary_Construct2(int capacity)
{
	Dictionary* Dict = SafeMalloc(sizeof(Dictionary));
	Dictionary_Construct(&Dict, capacity);
	return Dict;
}

static void Dictionary_EnsureCapacity(Dictionary* this, size_t capacity)
{
	if (this->_capacity < capacity)
	{
		this->_capacity *= DICTIONARY_CAPACITY_GROWTH;
		this->_data = SafeRealloc(this->_data, this->_capacity);
	}
}

static void Dictionary_AddToList(ArrayList* list)
{

}

int Dictionary_Add(Dictionary* this, void* element)
{
	if (this == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}



	return 0;
}

int Dictionary_Remove(Dictionary* this, void* element)
{

}

bool Dictionary_Contains(Dictionary* this, void* element)
{

}

int Dictionary_Clear(Dictionary* this)
{
	if (this == NULL)
	{
		return NULL_REFERENCE_ERRCODE;
	}

	for (int i = 0; i < this->_capacity; i++)
	{
		if ((this->_data + i) == NULL)
		{
			continue;
		}

		ArrayList_Clear(this->_data + i);
	}
}