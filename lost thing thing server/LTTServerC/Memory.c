#include <stdlib.h>
#include "Memory.h"
#include "LttErrors.h"
#include <memory.h>

// Functions.
void* Memory_SafeMalloc(size_t size)
{
	void* Pointer = malloc(size);

	if (!Pointer)
	{
		Error_AbortProgram("Failed to safely allocate memory. Will now commit sewer slide.");
		return NULL;
	}

	return Pointer;
}

void* Memory_SafeRealloc(void* ptr, size_t size)
{
	void* Pointer = realloc(ptr, size);

	if (!Pointer)
	{
		Error_AbortProgram("Failed to safely reallocate memory. Will now self destruct.");
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
	memcpy(destination, source, size);
}

void Memory_Set(char* memoryToSet, size_t memorySize, char value)
{
	memset(memoryToSet, value, memorySize);
}