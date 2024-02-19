#include "LTTHTML.h"
#include "Memory.h"
#include <stdbool.h>
#include "LTTString.h"


// Macros.
#define TAG_AREA "area"
#define TAG_BASE "base"
#define TAG_BR "br"
#define TAG_COL "col"
#define TAG_EMBED "embed"
#define TAG_HR "hr"
#define TAG_IMG "img"
#define TAG_INPUT "input"
#define TAG_LINK "link"
#define TAG_META "meta"
#define TAG_PARAM "param"
#define TAG_SOURCE "source"
#define TAG_TRACK "track"
#define TAG_WBR "wbr"

#define DOCTYPE "<!DOCTYPE html>"
#define TAG_SYMBOL_OPEN '<'
#define TAG_SYMBOL_CLOSE '>'
#define TAG_SYMBOL_SLASH '/'
#define ATTRIBUTE_SEPARATOR ' '
#define ATTRIBUTE_VALUE_ASSIGNMENT '='
#define ATTRIBUTE_VALUE_QUOTE '"'

#define ATTRIBUTE_CLASS "class"
#define ATTRIBUTE_ID "id"

#define NOT_FOUND_INDEX -1

#define CAPACITY_GROWTH 2


// Static functions.
static bool IsTagSelfClosing(const char* tag)
{
	return String_Equals(tag, TAG_AREA) || String_Equals(tag, TAG_BASE) || String_Equals(tag, TAG_BR) || String_Equals(tag, TAG_COL)
		|| String_Equals(tag, TAG_EMBED) || String_Equals(tag, TAG_HR) || String_Equals(tag, TAG_IMG) || String_Equals(tag, TAG_INPUT)
		|| String_Equals(tag, TAG_LINK) || String_Equals(tag, TAG_META) || String_Equals(tag, TAG_PARAM) || String_Equals(tag, TAG_SOURCE)
		|| String_Equals(tag, TAG_TRACK) || String_Equals(tag, TAG_WBR);
}

static void EnsureSubElementArrayCapacity(HTMLElement* element, size_t capacity)
{
	if (capacity <= element->_subElementArrayCapacity)
	{
		return;
	}

	if (element->_subElementArrayCapacity == 0)
	{
		element->_subElementArrayCapacity = 1;
		element->SubElementArray = (HTMLElement**)Memory_SafeMalloc(sizeof(HTMLElement*));
		if (capacity <= element->_subElementArrayCapacity)
		{
			return;
		}
	}

	while (capacity > element->_subElementArrayCapacity)
	{
		element->_subElementArrayCapacity *= CAPACITY_GROWTH;
	}

	element->SubElementArray = (HTMLElement**)Memory_SafeRealloc(element->SubElementArray, sizeof(HTMLElement*) * element->_subElementArrayCapacity);
}

static void EnsureAttributeArrayCapacity(HTMLElement* element, size_t capacity)
{
	if (capacity <= element->_attributeArrayCapacity)
	{
		return;
	}

	if (element->_attributeArrayCapacity == 0)
	{
		element->_attributeArrayCapacity = 1;
		element->AttributeArray = (HTMLAttribute*)Memory_SafeMalloc(sizeof(HTMLAttribute));
		if (capacity <= element->_attributeArrayCapacity)
		{
			return;
		}
	}

	while (capacity > element->_attributeArrayCapacity)
	{
		element->_attributeArrayCapacity *= CAPACITY_GROWTH;
	}

	element->AttributeArray = (HTMLAttribute*)Memory_SafeRealloc(element->AttributeArray, sizeof(HTMLAttribute) * element->_attributeArrayCapacity);
}

static void AppendHTMLElementAsString(StringBuilder* builder, HTMLElement* element)
{
	StringBuilder_AppendChar(builder, TAG_SYMBOL_OPEN);
	StringBuilder_Append(builder, element->Name);

	bool HadAttribute = false;
	for (size_t i = 0; i < element->AttributeCount; i++)
	{
		StringBuilder_AppendChar(builder, ATTRIBUTE_SEPARATOR);
		StringBuilder_Append(builder, element->AttributeArray[i].Name);

		if (element->AttributeArray[i].Value == NULL)
		{
			continue;
		}

		StringBuilder_AppendChar(builder, ATTRIBUTE_VALUE_ASSIGNMENT);
		StringBuilder_AppendChar(builder, ATTRIBUTE_VALUE_QUOTE);
		StringBuilder_Append(builder, element->AttributeArray[i].Value);
		StringBuilder_AppendChar(builder, ATTRIBUTE_VALUE_QUOTE);
	}

	StringBuilder_AppendChar(builder, TAG_SYMBOL_CLOSE);

	if (IsTagSelfClosing(element->Name))
	{
		return;
	}

	if (element->Contents)
	{
		StringBuilder_Append(builder, element->Contents);
	}

	for (size_t i = 0; i < element->SubElementCount; i++)
	{
		AppendHTMLElementAsString(builder, element->SubElementArray[i]);
	}


	StringBuilder_AppendChar(builder, TAG_SYMBOL_OPEN);
	StringBuilder_AppendChar(builder, TAG_SYMBOL_SLASH);
	StringBuilder_Append(builder, element->Name);
	StringBuilder_AppendChar(builder, TAG_SYMBOL_CLOSE);
}


// Functions.
/* Document. */
void HTMLDocument_Construct(HTMLDocument* document)
{
	document->Base = (HTMLElement*)Memory_SafeMalloc(sizeof(HTMLElement));
	HTMLElement_Construct(document->Base, "html");

	document->Head = HTMLElement_AddElement(document->Base, "head");
	document->Body = HTMLElement_AddElement(document->Base, "body");
}

HTMLElement* HTMLDocument_GetElementByID(HTMLDocument* document, const char* id)
{
	return HTMLElement_GetElementByID(document->Base, id);
}

HTMLElement* HTMLDocument_GetElementByClass(HTMLDocument* document, const char* className)
{
	return HTMLElement_GetElementByClass(document->Base, className);
}

HTMLElement* HTMLDocument_GetElementByAttribute(HTMLDocument* document, const char* name, const char* value)
{
	return HTMLElement_GetElementByAttribute(document->Base, name, value);
}

char* HTMLDocument_ToString(HTMLDocument* document)
{
	StringBuilder Builder;
	StringBuilder_Construct(&Builder, DEFAULT_STRING_BUILDER_CAPACITY);

	StringBuilder_Append(&Builder, DOCTYPE);
	
	AppendHTMLElementAsString(&Builder, document->Base);

	return Builder.Data;
}

void HTMLDocument_Deconstruct(HTMLDocument* document)
{
	HTMLElement_Deconstruct(document->Base);
}


/* Element. */
ErrorCode HTMLElement_Construct(HTMLElement* element, const char* name)
{
	if (String_LengthBytes(name) > sizeof(element->Name))
	{
		return Error_SetError(ErrorCode_InvalidArgument, "HTMLElement_Construct: name is longer than allowed.");
	}

	String_CopyTo(name, element->Name);
	
	element->AttributeArray = NULL;
	element->AttributeCount = 0;
	element->_attributeArrayCapacity = 0;
	
	element->SubElementArray = NULL;
	element->SubElementCount = 0;
	element->_subElementArrayCapacity = 0;

	element->Contents = NULL;
}

ErrorCode HTMLElement_SetAttribute(HTMLElement* element, const char* name, const char* value)
{
	for (size_t i = 0; i < element->AttributeCount; i++)
	{
		HTMLAttribute* Attribute = &element->AttributeArray[i];

		if (!String_Equals(name, Attribute->Name))
		{
			continue;
		}

		Memory_Free(Attribute->Value);
		Attribute->Value = String_CreateCopy(value);
		return;
	}

	EnsureAttributeArrayCapacity(element, element->AttributeCount + 1);

	if (String_LengthBytes(name) > sizeof(element->AttributeArray[0].Name))
	{
		return Error_SetError(ErrorCode_InvalidArgument, "HTMLElement_SetAttribute: Attribute name is too long.");
	}

	String_CopyTo(name, &element->AttributeArray[element->AttributeCount].Name);
	element->AttributeArray[element->AttributeCount].Value = String_CreateCopy(value);
	element->AttributeCount += 1;
}

HTMLAttribute* HTMLElement_GetAttribute(HTMLElement* element, const char* name)
{
	for (size_t i = 0; i < element->AttributeCount; i++)
	{
		if (String_Equals(name, element->AttributeArray[i].Name))
		{
			return &element->AttributeArray[i];
		}
	}

	return NULL;
}

void HTMLElement_RemoveAttribute(HTMLElement* element, const char* name)
{
	int RemovalIndex = NOT_FOUND_INDEX;

	for (int i = 0; i < element->AttributeCount; i++)
	{
		if (String_Equals(name, element->AttributeArray[i].Name))
		{
			RemovalIndex = i;
			break;
		}
	}

	if (RemovalIndex == NOT_FOUND_INDEX)
	{
		return;
	}

	for (int i = RemovalIndex + 1; i < element->AttributeCount; i++)
	{
		element->AttributeArray[i - 1] = element->AttributeArray[i];
	}

	element->AttributeCount -= 1;
}

void HTMLElement_ClearAttributes(HTMLElement* element)
{
	element->AttributeCount = 0;
}

HTMLElement* HTMLElement_AddElement(HTMLElement* baseElement, const char* name)
{
	EnsureSubElementArrayCapacity(baseElement, baseElement->SubElementCount + 1);

	HTMLElement* SubElement = (HTMLElement*)Memory_SafeMalloc(sizeof(HTMLElement));
	HTMLElement_Construct(SubElement, name);
	baseElement->SubElementArray[baseElement->SubElementCount] = SubElement;
	baseElement->SubElementCount += 1;

	return SubElement;
}

HTMLElement* HTMLElement_AddElement2(HTMLElement* baseElement, HTMLElement* elementToAdd)
{
	EnsureSubElementArrayCapacity(baseElement, baseElement->SubElementCount + 1);
	baseElement->SubElementArray[baseElement->SubElementCount] = elementToAdd;
	baseElement->SubElementCount += 1;
}

HTMLElement* HTMLElement_GetElementByID(HTMLElement* element, const char* id)
{
	return HTMLElement_GetElementByAttribute(element, ATTRIBUTE_ID, id);
}

HTMLElement* HTMLElement_GetElementByClass(HTMLElement* element, const char* className)
{
	return HTMLElement_GetElementByAttribute(element, ATTRIBUTE_CLASS, className);
}

HTMLElement* HTMLElement_GetElementByAttribute(HTMLElement* element, const char* name, const char* value)
{
	HTMLAttribute* Attribute = HTMLElement_GetAttribute(element, name);
	if (Attribute && String_Equals(value, Attribute->Value))
	{
		return element;
	}

	for (size_t i = 0; i < element->SubElementCount; i++)
	{
		HTMLElement* Element = HTMLElement_GetElementByAttribute(element->SubElementArray[i], name, value);
		if (Element)
		{
			return Element;
		}
	}

	return NULL;
}

HTMLElement* HTMLElement_GetElementByName(HTMLElement* element, const char* name)
{
	if (String_Equals(name, element->Name))
	{
		return element;
	}

	for (size_t i = 0; i < element->SubElementCount; i++)
	{
		HTMLElement* Element = HTMLElement_GetElementByName(element->SubElementArray[i], name);
		if (Element)
		{
			return Element;
		}
	}

	return NULL;
}

void HTMLElement_RemoveElement(HTMLElement* element, HTMLElement* elementToRemove)
{
	int RemovalIndex = NOT_FOUND_INDEX;

	for (int i = 0; i < element->SubElementCount; i++)
	{
		if (element->SubElementArray[i] == elementToRemove)
		{
			RemovalIndex = i;
			break;
		}
	}

	if (RemovalIndex == NOT_FOUND_INDEX)
	{
		return;
	}

	HTMLElement_Deconstruct(element->SubElementArray[RemovalIndex]);

	for (int i = RemovalIndex + 1; i < element->SubElementCount; i++)
	{
		element->SubElementArray[i - 1] = element->SubElementArray[i];
	}

	element->SubElementArray -= 1;
}

void HTMLElement_ClearElements(HTMLElement* element)
{
	for (size_t i = 0; i < element->SubElementCount; i++)
	{
		HTMLElement_Deconstruct(element->SubElementArray[i]);
	}

	element->SubElementCount = 0;
}

char* HTMLElement_ToString(HTMLElement* element)
{
	StringBuilder Builder;
	StringBuilder_Construct(&Builder, DEFAULT_STRING_BUILDER_CAPACITY);
	AppendHTMLElementAsString(&Builder, element);
	return Builder.Data;
}

void HTMLElement_Deconstruct(HTMLElement* element)
{
	Memory_Free(element->AttributeArray);
	Memory_Free(element->Contents);

	for (size_t i = 0; i < element->SubElementCount; i++)
	{
		HTMLElement_Deconstruct(element->SubElementArray[i]);
	}

	Memory_Free(element->SubElementArray);
	Memory_Free(element);
}