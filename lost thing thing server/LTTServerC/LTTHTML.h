#pragma once
#include <stddef.h>
#include "LTTErrors.h"

typedef struct HTMLAttributeStruct
{
	char Name[32];
	char* Value;
} HTMLAttribute;

typedef struct HTMLElementStruct
{
	char Name[32];
	char* Contents;
	HTMLAttribute* AttributeArray;
	size_t AttributeCount;
	size_t _attributeArrayCapacity;
} HTMLElement;

typedef struct HTMLDocumentStruct
{
	HTMLElement* Base;
	HTMLElement* Head;
	HTMLElement* Body;
} HTMLDocument;

void HTMLDocument_Construct(HTMLDocument* document);

HTMLElement* HTMLDocument_GetElementByID(HTMLDocument* document, const char* id);

HTMLElement* HTMLDocument_GetElementByClass(HTMLDocument* document, const char* class);

HTMLElement* HTMLDocument_GetElementByAttribute(HTMLDocument* document, const char* name, const char* value);

char* HTMLDocument_ToString(HTMLDocument* document);


ErrorCode HTMLElement_Construct(HTMLElement* element, const char* name);

ErrorCode HTMLElement_SetAttribute(HTMLElement* element, const char* name, const char* value);

ErrorCode HTMLElement_RemoveAttribute(HTMLElement* element, const char* name);

HTMLAttribute* HTMLElement_GetAttribute(HTMLElement* element, const char* name);

HTMLElement* HTMLElement_AddElement(HTMLElement* element, const char* name);

HTMLElement* HTMLElement_GetElementByID(HTMLElement* element, const char* id);

HTMLElement* HTMLElement_GetElementByClass(HTMLElement* element, const char* class);

HTMLElement* HTMLElement_GetElementByAttribute(HTMLElement* element, const char* name, const char* value);

char* HTMLElement_ToString(HTMLElement* element);