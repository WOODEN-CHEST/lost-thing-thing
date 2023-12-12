#include "Math.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Fields.
static bool IsRandomInitialized = false;


// Functions.


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

int Math_RandomInt()
{
	Math_InitRandom();
	int Value = rand() | (rand() << 15) | (rand() << 30);
	return Value;
}

int Math_RandomIntInRange(int min, int maxExclusive)
{
	Math_InitRandom();

	if (min < 0)
	{
		min = 0;
	}
	if (maxExclusive < min)
	{
		maxExclusive = min;
	}

	int Value = Math_RandomInt() & INT32_MAX;
	Value %= maxExclusive - min;
	Value += min;

	return Value;
}

float Math_RandomFloat()
{
	int IntValue = Math_RandomIntInRange(0, INT32_MAX);
	float Value = (float)IntValue / (float)INT32_MAX;
	return Value;
}