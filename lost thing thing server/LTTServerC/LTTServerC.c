#include "LttString.h"
#include "Math.h"
#include <stdbool.h>
#include <string.h>
#include "File.h"
#include "HttpListener.h"
#include "Directory.h"


int main()
{
	char Path[] = "/////";
	Directory_GetDirectoriesInPath(Path);
	//Directory_Create(Path);

	return 0;
}