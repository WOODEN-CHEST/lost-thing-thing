#include "Math.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Fields.
static bool IsRandomInitialized = false;


// Functions.
int Math_MinInt(int a, int b)
{
	if (a < b)
	{
		return a;
	}
	return b;
}

int Math_MaxInt(int a, int b)
{
	if (a > b)
	{
		return a;
	}
	return b;
}

int Math_MinFloat(float a, float b)
{
	if (a < b)
	{
		return a;
	}
	return b;
}

int Math_MaxFloat(float a, float b)
{
	if (a > b)
	{
		return a;
	}
	return b;
}

int Math_MinDouble(double a, double b)
{
	if (a < b)
	{
		return a;
	}
	return b;
}

int Math_MaxDouble(double a, double b)
{
	if (a > b)
	{
		return a;
	}
	return b;
}

int Math_ClampInt(int value, int min, int max)
{
	if (value < min)
	{
		return min;
	}
	else if (value > max)
	{
		return max;
	}
	return value;
}

float Math_ClampFloat(float value, float min, float max)
{
	if (value < min)
	{
		return min;
	}
	else if (value > max)
	{
		return max;
	}
	return value;
}

double Math_ClampDouble(double value, double min, double max)
{
	if (value < min)
	{
		return min;
	}
	else if (value > max)
	{
		return max;
	}
	return value;
}

static void Math_InitRandom()
{
	if (IsRandomInitialized)
	{
		return;
	}

	time_t Time;
	srand(time(&Time));
	IsRandomInitialized = true;
}

int Math_RandomInt(int min, int maxExclusive)
{
	Math_InitRandom();

	if (maxExclusive < min)
	{
		maxExclusive = min;
	}

	int Value = rand() | (rand() << 16);
	Value %= maxExclusive - min;
	Value += min;

	return  Value;
}

float Math_RandomFloat()
{
	int IntValue = Math_RandomInt(0, INT32_MAX);
	float Value = (float)IntValue / (float)INT32_MAX;
	return Value;
}