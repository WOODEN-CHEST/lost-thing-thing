#include "IDCodePointHashMap.h"
#include <stddef.h>
#include "Memory.h"
#include "LTTChar.h"
#include "LTTMath.h"
#include <stdbool.h>


// Macros.
#define HASHMAP_CAPACITY 256

#define BUCKET_CAPACITY 4
#define BUCKET_GROWTH 2

#define CODEPOINT_COUNT_LIST_DEFAULT_CAPACTY 16
#define CODEPOINT_COUNT_LIST_GROWTH 2

#define ID_LIST_CAPACITY 8
#define ID_LIST_GROWTH 2

#define MAX_TRACKED_CODEPOINT_COUNT 16

#define ID_HASHSET_CAPACITY 128

// Types.
/* HashMap */
typedef struct CodepointIDListStruct
{
	unsigned long long* IDs;
	size_t IDCount;
	size_t _capacity;
} CodepointIDList;

typedef struct CodepointAmountsEntryStruct
{
	int Codepoint;
	CodepointIDList* IDLists;
} CodepointAmountsEntry;

typedef struct CodepointAmountsBucketStruct
{
	CodepointAmountsEntry* Entries;
	size_t Count;
	size_t _capacity;
} CodepointAmountsBucket;

/* String codepoint count. */
typedef struct StringCodepointCountStruct
{
	int Codepoint;
	size_t Count;
} StringCodepointCount;

typedef struct StringCodepointCountListStruct
{
	StringCodepointCount* Elements;
	size_t ElementCount;
	size_t _capacity;
} StringCodepointCountList;

/* ID HashSet */
typedef struct IDHashSetStruct
{
	CodepointIDList* Lists;
	size_t Count;
} IDHashSet;


// Static functions.
/* Codepoint ID list. */
static void CodepointIDListConstruct(CodepointIDList* list)
{
	list->IDs = NULL;
	list->_capacity = 0;
	list->IDCount = 0;
}

static void CodepointIDListEnsureCapacity(CodepointIDList* list, size_t capacity)
{
	if (list->IDs == NULL)
	{
		list->_capacity = ID_LIST_CAPACITY;
		list->IDs = (unsigned long long*)Memory_SafeMalloc(sizeof(unsigned long long) * list->_capacity);
	}

	if (list->_capacity >= capacity)
	{
		return;
	}

	while (list->_capacity < capacity)
	{
		list->_capacity *= ID_LIST_GROWTH;
	}
	list->IDs = (unsigned long long*)Memory_SafeRealloc(list->IDs, sizeof(unsigned long long) * list->_capacity);
}

static void CodepointIDListAddID(CodepointIDList* list, unsigned long long id)
{
	CodepointIDListEnsureCapacity(list, list->IDCount + 1);
	list->IDs[list->IDCount] = id;
	list->IDCount += 1;
}

static void CodepointIDListRemoveID(CodepointIDList* list, unsigned long long id)
{
	size_t Index = 0;
	for (; Index < list->IDCount; Index++)
	{
		if (list->IDs[Index] == id)
		{
			list->IDCount -= 1;
			break;
		}
	}

	Index += 1;
	for (; Index <= list->IDCount; Index++)
	{
		list->IDs[Index - 1] = list->IDs[Index];
	}
}

static bool CodepointIDListContains(CodepointIDList* list, unsigned long long id)
{
	for (size_t i = 0; i < list->IDCount; i++)
	{
		if (list->IDs[i] == id)
		{
			return true;
		}
	}

	return false;
}

static void CodepointIDListDeconstruct(CodepointIDList* list)
{
	Memory_Free(list->IDs);
}

/* ID HashSet */
static void IDHashsetConstruct(IDHashSet* self)
{
	self->Lists = (CodepointIDList*)Memory_SafeMalloc(sizeof(CodepointIDList) * ID_HASHSET_CAPACITY);

	for (int i = 0; i < ID_HASHSET_CAPACITY; i++)
	{
		CodepointIDListConstruct(&self->Lists[i]);
	}

	self->Count = 0;
}

static void IDHashsetAddID(IDHashSet* self, unsigned long long id)
{
	CodepointIDList* List = &self->Lists[id % ID_HASHSET_CAPACITY];

	for (size_t i = 0; i < List->IDCount; i++)
	{
		if (List->IDs[i] == id)
		{
			return;
		}
	}

	CodepointIDListAddID(List, id);
	self->Count += 1;
}

static void IDHashSetAddRange(IDHashSet* self, unsigned long long* idArray, size_t idArrayLength)
{
	for (size_t i = 0; i < idArrayLength; i++)
	{
		IDHashsetAddID(self, idArray[i]);
	}
}

static void IDHashsetRemoveID(IDHashSet* self, unsigned long long id)
{
	CodepointIDList* List = &self->Lists[id % ID_HASHSET_CAPACITY];

	size_t OldLength = List->IDCount;
	CodepointIDListRemoveID(List, id);
	self->Count -= OldLength - List->IDCount;
}

static bool IDHashSetContains(IDHashSet* self, unsigned long long id)
{
	for (int ListIndex = 0; ListIndex < ID_HASHSET_CAPACITY; ListIndex++)
	{
		CodepointIDList* List = &self->Lists[ListIndex];

		if ((List->IDCount > 0) && (CodepointIDListContains(List, id)))
		{
			return true;
		}
	}

	return false;
}

static unsigned long long* IDHashSetToArray(IDHashSet* self, size_t* arraySize)
{
	CodepointIDList CombinedList;
	CodepointIDListConstruct(&CombinedList);

	for (int ListIndex = 0; ListIndex < ID_HASHSET_CAPACITY; ListIndex++)
	{
		CodepointIDList* IDListInHashSet = &self->Lists[ListIndex];
		for (size_t IDIndex = 0; IDIndex < IDListInHashSet->IDCount; IDIndex++)
		{
			CodepointIDListAddID(&CombinedList, IDListInHashSet->IDs[IDIndex]);
		}
	}

	*arraySize = CombinedList.IDCount;
	return CombinedList.IDs;
}

static void IDHashSetIntersectWithHashset(IDHashSet* resultHashSet, IDHashSet* hashSetToIntersect)
{
	size_t IDCount;
	unsigned long long* IDs = IDHashSetToArray(resultHashSet, &IDCount);

	for (size_t i = 0; i < IDCount; i++)
	{
		if (!IDHashSetContains(hashSetToIntersect, IDs[i]))
		{
			IDHashsetRemoveID(resultHashSet, IDs[i]);
		}
	}

	Memory_Free(IDs);
}

void IDHashSetClear(IDHashSet* self)
{
	self->Count = 0;
	for (int i = 0; i < ID_HASHSET_CAPACITY; i++)
	{
		self->Lists[i].IDCount = 0;
	}
}

static void IDHashsetDeconstruct(IDHashSet* self)
{
	for (int i = 0; i < ID_HASHSET_CAPACITY; i++)
	{
		CodepointIDListDeconstruct(&self->Lists[i]);
	}
	Memory_Free(self->Lists);
}


/* Codepoint amounts. */
static void ConstructCodepointAmountsEntry(CodepointAmountsEntry* amounts, int codepoint)
{
	amounts->Codepoint = codepoint;
	amounts->IDLists = (CodepointIDList*)Memory_SafeMalloc(sizeof(CodepointIDList) * MAX_TRACKED_CODEPOINT_COUNT);

	for (int i = 0; i < MAX_TRACKED_CODEPOINT_COUNT; i++)
	{
		CodepointIDListConstruct(&amounts->IDLists[i]);
	}
}

static void ClearCodepointEntry(CodepointAmountsEntry* entry)
{
	for (int i = 0; i < MAX_TRACKED_CODEPOINT_COUNT; i++)
	{
		entry->IDLists[i].IDCount = 0;
	}
}

static void DeconstructCodepointEntry(CodepointAmountsEntry* entry)
{
	for (int i = 0; i < MAX_TRACKED_CODEPOINT_COUNT; i++)
	{
		CodepointIDListDeconstruct(&entry->IDLists[i]);
	}

	Memory_Free(entry->IDLists);
}

/* Bucket. */
static void EnsureBucketCapacity(CodepointAmountsBucket* bucket, size_t capacity)
{
	if (bucket->_capacity == 0)
	{
		bucket->_capacity = BUCKET_CAPACITY;
		bucket->Entries = (CodepointAmountsEntry*)Memory_SafeMalloc(sizeof(CodepointAmountsEntry) * bucket->_capacity);
	}

	if (bucket->_capacity >= capacity)
	{
		return;
	}

	while (bucket->_capacity < capacity)
	{
		bucket->_capacity *= BUCKET_GROWTH;
	}
	bucket->Entries = (CodepointAmountsEntry*)Memory_SafeRealloc(bucket->Entries, sizeof(CodepointAmountsEntry) * bucket->_capacity);
}

static CodepointAmountsEntry* GetCodepointAmountsEntry(CodepointAmountsBucket* bucket, int codepoint)
{
	for (size_t i = 0; i < bucket->Count; i++)
	{
		if (bucket->Entries[i].Codepoint == codepoint)
		{
			return &(bucket->Entries[i]);
		}
	}

	EnsureBucketCapacity(bucket, bucket->Count + 1);
	ConstructCodepointAmountsEntry(&(bucket->Entries[bucket->Count]), codepoint);
	bucket->Count += 1;

	return &(bucket->Entries[bucket->Count - 1]);
}

static void ClearBucket(CodepointAmountsBucket* bucket)
{
	for (size_t i = 0; i < bucket->Count; i++)
	{
		ClearCodepointEntry(&(bucket->Entries[i]));
	}
}

static void DeconstructBucket(CodepointAmountsBucket* bucket)
{
	for (size_t i = 0; i < bucket->Count; i++)
	{
		DeconstructCodepointEntry(&bucket->Entries[i]);
	}

	Memory_Free(bucket->Entries);
}


/* Hashmap. */
static CodepointIDList* GetCodepointIDListFromHashMap(IDCodepointHashMap* self, int codepoint, size_t count)
{
	CodepointAmountsBucket* Bucket = &(self->CodepointBuckets[codepoint % HASHMAP_CAPACITY]);
	CodepointAmountsEntry* Entry = GetCodepointAmountsEntry(Bucket, codepoint);
	return &(Entry->IDLists[Math_Clamp(count - 1, 0, MAX_TRACKED_CODEPOINT_COUNT)]);
}


/* Counting codepoints. */
static void EnsureCapacityStringCodepointCountList(StringCodepointCountList* list, size_t capacity)
{
	if (list->_capacity >= capacity)
	{
		return;
	}

	while (list->_capacity < capacity)
	{
		list->_capacity *= CODEPOINT_COUNT_LIST_GROWTH;
	}
	list->Elements = (StringCodepointCount*)Memory_SafeRealloc(list->Elements, sizeof(StringCodepointCount) * list->_capacity);
}

static void IncrementCountForCodepoint(int codepoint, StringCodepointCountList* list)
{
	for (size_t i = 0; i < list->ElementCount; i++)
	{
		if (list->Elements[i].Codepoint == codepoint)
		{
			list->Elements[i].Count += 1;
			return;
		}
	}

	EnsureCapacityStringCodepointCountList(list, list->ElementCount + 1);
	list->Elements[list->ElementCount].Codepoint = codepoint;
	list->Elements[list->ElementCount].Count = 1;

	list->ElementCount += 1;
}

static void CountCodepoints(const char* string, StringCodepointCountList* list, bool ignoreWhitespace)
{
	list->ElementCount = 0;
	list->_capacity = CODEPOINT_COUNT_LIST_DEFAULT_CAPACTY;
	list->Elements = (StringCodepointCount*)Memory_SafeMalloc(sizeof(StringCodepointCount) * list->_capacity);

	size_t Index = 0;
	while (string[Index] != '\0')
	{
		char Character[MAX_UTF8_CODEPOINT_SIZE];
		Char_CopyTo(string + Index, Character);
		Char_ToLower(Character);

		int Codepoint = Char_GetCodepoint(Character);

		if (ignoreWhitespace && Char_IsWhitespace(Character))
		{
			Index += Char_GetByteCountCodepoint(Codepoint);
			continue;
		}

		IncrementCountForCodepoint(Codepoint, list);
		Index += Char_GetByteCountCodepoint(Codepoint);
	}
}


// Functions.
void IDCodepointHashMap_Construct(IDCodepointHashMap* self)
{
	self->CodepointBuckets = (CodepointAmountsBucket*)Memory_SafeMalloc(sizeof(CodepointAmountsBucket) * HASHMAP_CAPACITY);

	for (int i = 0; i < HASHMAP_CAPACITY; i++)
	{
		self->CodepointBuckets[i].Entries = NULL;
		self->CodepointBuckets[i].Count = 0;
		self->CodepointBuckets[i]._capacity = 0;
	}
}

void IDCodepointHashMap_AddID(IDCodepointHashMap* self, const char* string, unsigned long long id)
{
	StringCodepointCountList CodepointCountList;
	CountCodepoints(string, &CodepointCountList, true);

	for (size_t i = 0; i < CodepointCountList.ElementCount; i++)
	{
		int Codepoint = CodepointCountList.Elements[i].Codepoint;
		size_t Count = CodepointCountList.Elements[i].Count;
		CodepointIDListAddID(GetCodepointIDListFromHashMap(self, Codepoint, Count), id);
	}

	Memory_Free(CodepointCountList.Elements);
}

void IDCodepointHashMap_RemoveID(IDCodepointHashMap* self, const char* string, unsigned long long id)
{
	StringCodepointCountList CodepointCountList;
	CountCodepoints(string, &CodepointCountList, true);

	for (size_t i = 0; i < CodepointCountList.ElementCount; i++)
	{
		int Codepoint = CodepointCountList.Elements[i].Codepoint;
		size_t Count = CodepointCountList.Elements[i].Count;
		CodepointIDListRemoveID(GetCodepointIDListFromHashMap(self, Codepoint, Count), id);
	}

	Memory_Free(CodepointCountList.Elements);
}

void IDCodepointHashMap_Clear(IDCodepointHashMap* self)
{
	for (int i = 0; i < HASHMAP_CAPACITY; i++)
	{
		ClearBucket(&(self->CodepointBuckets[i]));
	}
}

unsigned long long* IDCodepointHashMap_FindByString(IDCodepointHashMap* self, const char* string, bool ignoreWhitespace, size_t* arraySize)
{
	// Unoptimized garbage-ass algorithm.
	StringCodepointCountList CodepointCountList;
	CountCodepoints(string, &CodepointCountList, ignoreWhitespace);

	if (CodepointCountList.ElementCount == 0)
	{
		Memory_Free(CodepointCountList.Elements);
		return NULL;
	}

	IDHashSet FinalIDHashSet;
	IDHashSet CodepointIDHashSet;
	IDHashsetConstruct(&FinalIDHashSet);
	IDHashsetConstruct(&CodepointIDHashSet);

	for (size_t i = CodepointCountList.Elements[0].Count; i < MAX_TRACKED_CODEPOINT_COUNT; i++)
	{
		CodepointIDList* IDList = GetCodepointIDListFromHashMap(self,
			CodepointCountList.Elements[0].Codepoint, i);
		IDHashSetAddRange(&FinalIDHashSet, IDList->IDs, IDList->IDCount);
	}

	for (size_t CodepointIndex = 1; CodepointIndex < CodepointCountList.ElementCount; CodepointIndex++)
	{
		for (size_t CountIndex = CodepointCountList.Elements[CodepointIndex].Count;
			CountIndex < MAX_TRACKED_CODEPOINT_COUNT; CountIndex++)
		{
			CodepointIDList* IDList = GetCodepointIDListFromHashMap(self, CodepointCountList.Elements[CodepointIndex].Codepoint, CountIndex);
			IDHashSetAddRange(&CodepointIDHashSet, IDList->IDs, IDList->IDCount);
		}

		IDHashSetIntersectWithHashset(&FinalIDHashSet, &CodepointIDHashSet);
		IDHashSetClear(&CodepointIDHashSet);

		if (FinalIDHashSet.Count == 0)
		{
			break;
		}
	}

	unsigned long long* IDArray = IDHashSetToArray(&FinalIDHashSet, arraySize);
	IDHashsetDeconstruct(&FinalIDHashSet);
	IDHashsetDeconstruct(&CodepointIDHashSet);
	Memory_Free(CodepointCountList.Elements);
	return IDArray;
}

void IDCodepointHashMap_Deconstruct(IDCodepointHashMap* self)
{
	for (int i = 0; i < HASHMAP_CAPACITY; i++)
	{
		DeconstructBucket(&self->CodepointBuckets[i]);
	}

	Memory_Free(self->CodepointBuckets);
}