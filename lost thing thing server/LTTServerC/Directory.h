#pragma once

#include  <stdio.h>
#include <stdbool.h>

// Functions.
bool Directory_Exists(char* path);

int Directory_Create(char* path);

int Directory_Delete(char* path);