#include <stdbool.h>
#include "LttCommon.h"
#include <stdio.h>

int main()
{
	StringBuilder Builder;
	StringBuilder_Construct(&Builder, DEFAULT_STRING_BUILDER_CAPACITY);

	StringBuilder_AppendChar(&Builder, 'h');

	return 0;
}