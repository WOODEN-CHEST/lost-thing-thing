#include "IDCodePointHashMap.h"
#include <stddef.h>
#include "Memory.h"
#include "LTTChar.h"
#include "LTTMath.h"


/* This entire implementation would've been so simple in a higher level programming language.
Instead it is tedious. Not hard, just tedious and annoying. */

// Macros.
#define HASHMAP_CAPACITY 256

#define BUCKET_CAPACITY 4
#define BUCKET_GROWTH 2

#define CODEPOINT_COUNT_LIST_DEFAULT_CAPACTY 16
#define CODEPOINT_COUNT_LIST_GROWTH 2

#define ID_LIST_CAPACITY 8
#define ID_LIST_GROWTH 2

#define MAX_TRACKED_CODEPOINT_COUNT 30

#define ID_HASHSET_BUCKET_COUNT 100
#define ID_HASHSET_BUCKET_CAPACITY 8
#define ID_HASHSET_BUCKET_GROWTH 2

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
typedef struct IDHashSetBucketStruct
{
	unsigned long long* IDs;
	size_t Count;
	size_t _capacity;
} IDHashSetBucket;

typedef struct IDHashSetStruct
{
	IDHashSetBucket* Buckets;
} IDHashSet;


// Static functions.
/* ID HashSet */
static void IDHashsetBucketEnsureCapacity(IDHashSetBucket* bucket)
{

}

static void IDHashsetConstruct(IDHashSet* self)
{
	self->Buckets = (IDHashSetBucket*)Memory_SafeMalloc(sizeof(IDHashSetBucket) * ID_HASHSET_BUCKET_COUNT);

	for (int i = 0; i < ID_HASHSET_BUCKET_COUNT; i++)
	{
		self->Buckets[i].Count = 0;
		self->Buckets[i].IDs = NULL;
		self->Buckets[i]._capacity = 0;
	}
}


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
	CodepointIDListEnsureCapacity(list, list->_capacity + 1);
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
		Memory_Free(entry->IDLists[i].IDs);
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
static void AddIDToCodepointCountMap(IDCodepointHashMap* self, unsigned long long id, int codepoint, int count)
{
	CodepointAmountsBucket* Bucket = &(self->CodepointBuckets[codepoint % HASHMAP_CAPACITY]);
	CodepointAmountsEntry* Entry = GetCodepointAmountsEntry(Bucket, codepoint);
	CodepointIDListAddID(&(Entry->IDLists[Math_Clamp(count - 1, 0, MAX_TRACKED_CODEPOINT_COUNT)]), id);
}

RemoveIDFromCodepointCountMap(IDCodepointHashMap* self, unsigned long long id, int codepoint, int count)
{
	CodepointAmountsBucket* Bucket = &(self->CodepointBuckets[codepoint % HASHMAP_CAPACITY]);
	CodepointAmountsEntry* Entry = GetCodepointAmountsEntry(Bucket, codepoint);
	CodepointIDListRemoveID(&(Entry->IDLists[Math_Clamp(count - 1, 0, MAX_TRACKED_CODEPOINT_COUNT)]), id);
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

static void CountCodepoints(const char* string, StringCodepointCountList* list)
{
	list->ElementCount = 0;
	list->_capacity = CODEPOINT_COUNT_LIST_DEFAULT_CAPACTY;
	list->Elements = (StringCodepointCount*)Memory_SafeMalloc(sizeof(StringCodepointCount) * list->_capacity);

	size_t Index = 0;
	while (string[Index] != '\0')
	{
		int Codepoint = Char_GetCodepoint(string + Index);
		IncrementCountForCodepoint(Codepoint, list);
		Index += Char_GetByteCountCodepoint(Codepoint);
	}
}


/* Searching by string. */
static void MultiplyIDLists(CodepointIDList* resultList, CodepointIDList* secondList)
{

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
	CountCodepoints(string, &CodepointCountList);

	for (size_t i = 0; i < CodepointCountList.ElementCount; i++)
	{
		AddIDToCodepointCountMap(self, id, CodepointCountList.Elements[i].Codepoint, CodepointCountList.Elements[i].Count);
	}

	Memory_Free(CodepointCountList.Elements);
}

void IDCodepointHashMap_RemoveID(IDCodepointHashMap* self, const char* string, unsigned long long id)
{
	StringCodepointCountList CodepointCountList;
	CountCodepoints(string, &CodepointCountList);

	for (size_t i = 0; i < CodepointCountList.ElementCount; i++)
	{
		RemoveIDFromCodepointCountMap(self, id, CodepointCountList.Elements[i].Codepoint, CodepointCountList.Elements[i].Count);
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

unsigned long long* IDCodepointHashMap_FindByString(IDCodepointHashMap* self, const char* string, bool ignoreWhitespace)
{
	// Unoptimized garbage-ass algorithm, but works so who cares
	StringCodepointCountList CodepointCountList;
	CountCodepoints(string, &CodepointCountList);

	CodepointIDList CombinedIDList;
	CodepointIDListConstruct(&CombinedIDList);
	CodepointIDList SingleCodpointIDList;
	CodepointIDListConstruct(&SingleCodpointIDList);

	for (size_t i = 0; i < CodepointCountList.ElementCount; i++)
	{


		if (CombinedIDList.IDCount <= 1)
		{
			break;
		}
	}

	Memory_Free(SingleCodpointIDList.IDs);
	if (CombinedIDList.IDCount == 0)
	{
		Memory_Free(CombinedIDList.IDs);
		return NULL;
	}
	return CombinedIDList.IDs;
}

void IDCodepointHashMap_Deconstruct(IDCodepointHashMap* self)
{
	for (int i = 0; i < HASHMAP_CAPACITY; i++)
	{
		DeconstructBucket(&self->CodepointBuckets[i]);
	}

	Memory_Free(self->CodepointBuckets);
}