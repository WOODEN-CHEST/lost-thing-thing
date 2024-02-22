#pragma once

// Functions.
void* Memory_SafeMalloc(size_t size);

void* Memory_SafeRealloc(void* ptr, size_t size);

void Memory_Free(void* ptr);