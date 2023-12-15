#pragma once

#include  <stdio.h>
#include <stdbool.h>
#include "ArrayList.h"

#define IsPathSeparator(character) (character == '\\') || (character == '/')

// Functions.
bool Directory_Exists(char* path);

int Directory_Create(char* path);

int Directory_CreateAll(char* path);

int Directory_Delete(char* path);