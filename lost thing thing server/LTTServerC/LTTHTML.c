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


// Static functions.
static bool IsTagSelfClosing(const char* tag)
{
	return String_Equals(tag, TAG_AREA) || String_Equals(tag, TAG_BASE) || String_Equals(tag, TAG_BR) || String_Equals(tag, TAG_COL)
		|| String_Equals(tag, TAG_EMBED) || String_Equals(tag, TAG_HR) || String_Equals(tag, TAG_IMG) || String_Equals(tag, TAG_INPUT)
		|| String_Equals(tag, TAG_LINK) || String_Equals(tag, TAG_META) || String_Equals(tag, TAG_PARAM) || String_Equals(tag, TAG_SOURCE)
		|| String_Equals(tag, TAG_TRACK) || String_Equals(tag, TAG_WBR);
}

static void EnsureElementCapacity(HTMLElement* element, size_t capacity)
{

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




/* Element. */
ErrorCode HTMLElement_Construct(HTMLElement* element, const char* name)
{
	if (String_LengthBytes(name) > sizeof(element->Name))
	{
		return Error_SetError(ErrorCode_InvalidArgument, "HTMLElement_Construct: name is longer than allowed.");
	}

	String_CopyTo(name, element->Name);
	element->_attributeArrayCapacity = 0;
	element->Contents = NULL;
	element->AttributeArray = NULL;
	element->AttributeCount = 0;
}

HTMLElement* HTMLElement_AddElement(HTMLElement* element, const char* name)
{
	EnsureElementCapacity()
}