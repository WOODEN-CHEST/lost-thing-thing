#pragma once
#include <stdbool.h>

typedef struct DictionaryStruct Dictionary;

int Dictionary_Construct(Dictionary* hashSet, int capacity);

Dictionary* Dictionary_Construct2(int capacity);

int Dictionary_Add(Dictionary* this, void* element);

int Dictionary_Remove(Dictionary* this, void* element);

bool Dictionary_Contains(Dictionary* this, void* element);

int Dictionary_Clear(Dictionary* this);