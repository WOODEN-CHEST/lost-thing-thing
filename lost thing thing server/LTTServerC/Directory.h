#pragma once

#include "LTTErrors.h"

#define Directory_IsPathSeparator(character) (character == '\\') || (character == '/')
#define PATH_SEPARATOR '/'

// Functions.
/// <summary>
/// Checks whether a given directory exists.
/// </summary>
/// <param name="path">A path to a directory.</param>
/// <returns>true if it exists, otherwise false.</returns>
_Bool Directory_Exists(const char* path);

/// <summary>
/// Creates a directory. All parent directories must already exist for the directory to be created.
/// </summary>
/// <param name="path">A path to the directory to be created.</param>
/// <returns>0 on success, error code on failure.</returns>
ErrorCode Directory_Create(const char* path);

/// <summary>
/// Creates all directories in a given path.
/// </summary>
/// <param name="path">A path to a directory.</param>
/// <returns>0 on success, error code on failure.</returns>
ErrorCode Directory_CreateAll(const char* path);

/// <summary>
/// Deletes a directory.
/// </summary>
/// <param name="path">The path of the directory to delete.</param>
/// <returns>0 on success, error code on failure.</returns>
ErrorCode Directory_Delete(const char* path);

/// <summary>
/// Returns a path to the parent directory of a directory or file.
/// </summary>
/// <param name="path">The path to find the parent directory of.</param>
/// <returns>The parent directory path (stored on the heap)</returns>
char* Directory_GetParentDirectory(const char* path);

/// <summary>
/// Combines the two strings with a '/' (path separator) character in-between. Essentially combines the paths.
/// </summary>/
/// <param name="path1">First path</param>
/// <param name="path2">Second path</param>
/// <returns>Combined path (stored on the heap)</returns>
char* Directory_CombinePaths(const char* path1, const char* path2);

/// <summary>
/// Gets the name of the final directory or file in a path.
/// </summary>
/// <param name="path">The full path of the file or directory.</param>
/// <returns>Name of the file or directory (stored on the heap)</returns>
char* Directory_GetName(const char* path);

/// <summary>
/// Changes the file extension in the given path.
/// </summary>
///  <param name="path">The path for which to change the extension</param>
///  <param name="newExtension">The new extension.</param>
/// <returns>Path with the extension changes (stored on the heap)</returns>
char* Directory_ChangePathExtension(const char* path, const char* newExtension);