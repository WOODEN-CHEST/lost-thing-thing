#pragma once
#include <stddef.h>

// Functions.
void* Memory_SafeMalloc(size_t size);

void* Memory_SafeRealloc(void* ptr, size_t size);

void Memory_Free(void* ptr);

void Memory_Copy(const char* source, char* destination, size_t size);

void Memory_Set(unsigned char* memoryToSet, size_t memorySize, unsigned char value);