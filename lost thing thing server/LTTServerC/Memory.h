#pragma once

// Functions.
void* SafeMalloc(const int size);

void* SafeRealloc(void* ptr, const int size);

void FreeMemory(void* ptr);