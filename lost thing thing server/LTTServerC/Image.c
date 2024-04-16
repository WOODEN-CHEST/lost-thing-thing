#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "LTTMath.h"
#include "Memory.h"

#include "Image.h"

void Image_ScaleImageToFit(Image* image, int maxSizeOnAxis)
{
	float Scale = (float)maxSizeOnAxis / Math_Max(image->SizeX, image->SizeY);
	int TargetXSize = (int)(image->SizeX * Scale);
	int TargetYSize = (int)(image->SizeY * Scale);

	const char* ResizedImageData = Memory_SafeMalloc(TargetXSize * TargetYSize * STBI_rgb);
	stbir_resize(image->Data, image->SizeX, image->SizeY, 0, (void*)ResizedImageData, TargetXSize, TargetYSize,
		0, STBIR_RGB, STBIR_TYPE_UINT8, STBIR_EDGE_CLAMP, STBIR_FILTER_DEFAULT);
	Memory_Free((char*)image->Data);
	image->Data = (const unsigned char*)ResizedImageData;
	image->SizeX = TargetXSize;
	image->SizeY = TargetYSize;
	image->ColorChannels = STBI_rgb;
}