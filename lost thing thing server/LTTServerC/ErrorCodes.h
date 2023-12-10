#pragma once

#define SUCCESS_CODE
#define NULL_REFERENCE_ERRCODE -1
#define INVALID_ARGUMENT_ERRCODE -2
#define ARGUMENT_OUT_OF_RANGE_ERRCODE -3
#define ILLEGAL_OPERATION_ERRCODE -4
#define MEMORY_ALLOCATION_FAILED_ERRCODE -5
#define IO_ERROR_ERRCODE -6


// Functions.
void AbortProgramIfError(int code);

void AbortProgram(int code);