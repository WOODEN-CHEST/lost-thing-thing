#include <stdlib.h>
#include "Memory.h"
#include "LttErrors.h"

// Functions.
void* Memory_SafeMalloc(size_t size)
{
	void* Pointer = malloc(size);

	if (Pointer == NULL)
	{
		Error_AbortProgram("Failed to safely allocate memory.");
		return NULL;
	}

	return Pointer;
}

void* Memory_SafeRealloc(void* ptr, size_t size)
{
	void* Pointer = realloc(ptr, size);

	if (Pointer == NULL)
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