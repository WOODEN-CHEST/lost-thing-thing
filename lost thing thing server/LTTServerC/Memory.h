#pragma once

// Functions.
void* Memory_SafeMalloc(const int size);

void* Memory_SafeRealloc(void* ptr, const int size);

void Memory_Free(void* ptr);