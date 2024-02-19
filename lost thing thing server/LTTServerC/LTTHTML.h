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
	char* Contents; // May be null.

	struct HTMLElementStruct** SubElementArray; // May be null.
	size_t SubElementCount;

	HTMLAttribute* AttributeArray; // May be null.
	size_t AttributeCount;

	size_t _attributeArrayCapacity;
	size_t _subElementArrayCapacity;
} HTMLElement;



typedef struct HTMLDocumentStruct
{
	HTMLElement* Base;
	HTMLElement* Head;
	HTMLElement* Body;
} HTMLDocument;

void HTMLDocument_Construct(HTMLDocument* document);

HTMLElement* HTMLDocument_GetElementByID(HTMLDocument* document, const char* id);

HTMLElement* HTMLDocument_GetElementByClass(HTMLDocument* document, const char* className);

HTMLElement* HTMLDocument_GetElementByAttribute(HTMLDocument* document, const char* name, const char* value);

char* HTMLDocument_ToString(HTMLDocument* document);


ErrorCode HTMLElement_Construct(HTMLElement* element, const char* name);

ErrorCode HTMLElement_SetAttribute(HTMLElement* element, const char* name, const char* value);

HTMLAttribute* HTMLElement_GetAttribute(HTMLElement* element, const char* name);

void HTMLElement_RemoveAttribute(HTMLElement* element, const char* name);

void HTMLElement_ClearAttributes(HTMLElement* element);

HTMLElement* HTMLElement_AddElement(HTMLElement* baseElement, const char* name);

HTMLElement* HTMLElement_AddElement2(HTMLElement* baseElement, HTMLElement* elementToAdd, const char* name);

HTMLElement_GetElementByTag(HTMLElement* element, const char* tag);

HTMLElement* HTMLElement_GetElementByID(HTMLElement* element, const char* id);

HTMLElement* HTMLElement_GetElementByClass(HTMLElement* element, const char* className);

HTMLElement* HTMLElement_GetElementByAttribute(HTMLElement* element, const char* name, const char* value);

char* HTMLElement_ToString(HTMLElement* element);