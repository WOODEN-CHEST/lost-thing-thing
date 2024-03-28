#pragma once
#include <stdbool.h>
#include <math.h>

// Macros.
#define Math_Max(a, b) (a > b ? a : b)

#define Math_Min(a, b) (a < b ? a : b)

#define Math_Clamp(value, minValue, maxValue) (value < minValue ? minValue : (value > maxValue ? maxValue : value))


// Functions.
int Math_RandomInt();

int Math_RandomIntInRange(int min, int maxExclusive);

float Math_RandomFloat();

#define Math_TryWithChance(chance) chance <= Math_RandomFloat()