#include <stdlib.h>
#include "Memory.h"
#include "LttErrors.h"

// Functions.
void* Memory_SafeMalloc(size_t size)
{
	void* Pointer = malloc(size);

	if (!Pointer)
	{
		Error_AbortProgram("Failed to safely allocate memory.");
		return NULL;
	}

	return Pointer;
}

void* Memory_SafeRealloc(void* ptr, size_t size)
{
	void* Pointer = realloc(ptr, size);

	if (!Pointer)
	{
		Error_AbortProgram("Failed to safely reallocate memory.");
		return NULL;
	}

	return Pointer;
}

void Memory_Free(void* ptr)
{
	free(ptr);
}

void Memory_Copy(const char* source, char* destination, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		destination[i] = source[i];
	}
}

void Memory_Set(unsigned char* memoryToSet, size_t memorySize, unsigned char value)
{
	for (int i = 0; i < memorySize; i++)
	{
		memoryToSet[i] = value;
	}
}