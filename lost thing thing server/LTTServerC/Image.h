#pragma once

#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_resize2.h"
#define FILE_EXTENSION_PNG ".png"

typedef struct ImageStruct
{
	int SizeX;
	int SizeY;
	int ColorChannels;
	const unsigned char* Data;
} Image;

void Image_ScaleImageToFit(Image* image, int maxSizeOnAxis);