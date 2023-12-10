#include <stdlib.h>
#include "Memory.h"
#include "ErrorCodes.h"

// Functions.
void* SafeMalloc(const int size)
{
	void* Pointer = malloc(size);

	if (Pointer == NULL)
	{
		AbortProgram(MEMORY_ALLOCATION_FAILED_ERRCODE);
	}

	return Pointer;
}

void* SafeRealloc(void* ptr, const int size)
{
	if (ptr == NULL)
	{
		return NULL;
	}

	void* Pointer = realloc(ptr, size);

	if (Pointer == NULL)
	{
		AbortProgram(MEMORY_ALLOCATION_FAILED_ERRCODE);
	}

	return Pointer;
}