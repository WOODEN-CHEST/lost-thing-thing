#include "LttString.h"
#include "Math.h"
#include <stdbool.h>
#include <string.h>
#include "File.h"
#include "HttpListener.h"

int main()
{
	char Path[] = "C:\\Users\\KČerņavskis\\Desktop\\tests\\file_to_delete.txt";

	File_Delete(Path);

	return 0;
}