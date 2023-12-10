#include "ErrorCodes.h"
#include <stdlib.h>

void AbortProgramIfError(int code)
{
	if (code == 0)
	{
		return;
	}


	AbortProgram(code);
}

void AbortProgram(int code)
{

}