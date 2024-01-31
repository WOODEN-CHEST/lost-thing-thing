#pragma once
#include <stdbool.h>

// Macros.
#define Math_Max(a, b) a > b ? a : b

#define Math_Min(a, b) a < b ? a : b

#define Math_Clamp(value, minValue, maxValue) Math_Max(minValue, Math_Min(value, maxValue))


// Functions.
int Math_RandomInt();

int Math_RandomIntInRange(int min, int maxExclusive);

float Math_RandomFloat();

#define Math_TryWithChance(chance) chance <= Math_RandomFloat()