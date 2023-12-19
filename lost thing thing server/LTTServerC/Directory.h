#pragma once

#include  <stdio.h>
#include <stdbool.h>
#include "ArrayList.h"

#define IsPathSeparator(character) (character == '\\') || (character == '/')
#define PathSeparator '/'

// Functions.
/// <summary>
/// Checks whether a given directory exists.
/// </summary>
/// <param name="path">A path to a directory.</param>
/// <returns>true if it exists, otherwise false.</returns>
bool Directory_Exists(char* path);

/// <summary>
/// Creates a directory. All parent directories must already exist for the directory to be created.
/// </summary>
/// <param name="path">A path to the directory to be created.</param>
/// <returns>0 on success, error code on failure.</returns>
int Directory_Create(char* path);

/// <summary>
/// Creates all directories in a given path.
/// </summary>
/// <param name="path">A path to a directory.</param>
/// <returns>0 on success, error code on failure.</returns>
int Directory_CreateAll(char* path);

/// <summary>
/// Deletes a directory.
/// </summary>
/// <param name="path">The path of the directory to delete.</param>
/// <returns>0 on success, error code on failure.</returns>
int Directory_Delete(char* path);

/// <summary>
/// Returns a path to the parent directory of a directory or file.
/// </summary>
/// <param name="path">The path to find the parent directory of.</param>
/// <returns>The parent directory path (stored on the heap)</returns>
char* Directory_GetParentDirectory(char* path);

/// <summary>
/// Combines the two strings with a '/' (path separator) character in-between. Essentially combines the paths.
/// </summary>/
/// <param name="path1">First path</param>
/// <param name="path2">Second path</param>
/// <returns>Combined path (stored on the heap)</returns>
char* Directory_Combine(char* path1, char* path2);