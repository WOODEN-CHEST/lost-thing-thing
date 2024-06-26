#pragma once
#include <stddef.h>
#include <stdbool.h>


// Structures.
typedef struct IDCodepointHashMapStruct
{
	struct CodepointAmountsBucketStruct* CodepointBuckets;
} IDCodepointHashMap;


// Functions.
void IDCodepointHashMap_Construct(IDCodepointHashMap* self);

void IDCodepointHashMap_AddID(IDCodepointHashMap* self, const char* string, unsigned long long id);

void IDCodepointHashMap_RemoveID(IDCodepointHashMap* self, const char* string, unsigned long long id);

void IDCodepointHashMap_Clear(IDCodepointHashMap* self);

unsigned long long* IDCodepointHashMap_FindByString(IDCodepointHashMap* self, const char* string, bool ignoreWhitespace, size_t* arraySize);

void IDCodepointHashMap_Deconstruct(IDCodepointHashMap* self);