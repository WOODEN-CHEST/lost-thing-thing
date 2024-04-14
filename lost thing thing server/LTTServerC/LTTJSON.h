#pragma once
#include <stdbool.h>

typedef enum JSONTypeEnum
{
	JSONType_Object,
	JSONType_Integer,
	JSONType_Float,
	JSONType_String,
	JSONType_Boolean,
	JSONType_Null,
} JSONType;

typedef struct JSONObjectStruct
{

} JSONObject;

typedef union JSONValueUnion
{

	long long Integer;
	double Float;
	char* String;
	bool Boolean;
	void* Null;
} JSONValue;

typedef struct JSONEntryStruct
{
	JSONType Type;
	JSONValue Value;
};

