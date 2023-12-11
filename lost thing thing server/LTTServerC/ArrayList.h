#pragma once

// Structures.
struct ArrayListStruct
{
	void** Data;
	int Length;
	int _capacity;
};

typedef struct ArrayListStruct ArrayList;

// Functions.
int ArrayList_Construct(ArrayList* arrayList);

ArrayList* ArrayList_Construct2();

int ArrayList_Deconstruct(ArrayList* this);

int ArrayList_Add(ArrayList* this, void* objectPointer);

int ArrayList_Insert(ArrayList* this, void* objectPointer, int index);

int ArrayList_Remove(ArrayList* this, void* objectPointer);

int ArrayList_RemoveAt(ArrayList* this, int index);

int ArrayList_RemoveLast(ArrayList* this);

int ArrayList_RemoveFirst(ArrayList* this);

int ArrayList_Clear(ArrayList* this);