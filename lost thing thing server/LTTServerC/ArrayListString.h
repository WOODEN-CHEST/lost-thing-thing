#pragma once
#include "LttErrors.h"

// Macros.
#define ARRAYLIST_STRING_DEFAULT_CAPACITY 16

// Structures.
struct ArrayListStringStruct
{
	char** Data;
	size_t Length;
	size_t _capacity;
};

typedef struct ArrayListStringStruct ArrayListString;

// Functions.
void ArrayListString_Construct(ArrayListString* arrayList, size_t capacity);

ArrayListString* ArrayListString_Construct2(size_t capacity);

void ArrayLisStringt_Deconstruct(ArrayListString* self);

void ArrayListString_Add(ArrayListString* self, char* string);

ErrorCode ArrayListString_Insert(ArrayListString* self, char* string, size_t index);

void ArrayListString_Remove(ArrayListString* self, char* string);

ErrorCode ArrayListString_RemoveAt(ArrayListString* self, size_t index);

void ArrayListString_RemoveLast(ArrayListString* self);

void ArrayListString_RemoveFirst(ArrayListString* self);

int ArrayListString_Clear(ArrayListString* self);