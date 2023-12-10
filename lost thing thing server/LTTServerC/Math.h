#pragma once

// Functions.

int Math_MinInt(int a, int b);

int Math_MaxInt(int a, int b);

int Math_MinFloat(float a, float b);

int Math_MaxFloat(float a, float b);

int Math_MinDouble (double a, double b);

int Math_MaxDouble(double a, double b);

int Math_ClampInt(int value, int min, int max);

float Math_ClampFloat(float value, float min, float max);

double Math_ClampDouble(double value, double min, double max);

int Math_RandomInt(int min, int maxExclusive);

float Math_RandomFloat();