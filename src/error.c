/*
 * Project Name: Implementation of the IFJ24 Compiler
 * Authors: 
 */

#include "error.h"

void error_exit(int error_code, const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "ERROR %i: ", error_code);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    cleanup_pointers_storage();
    exit(error_code);
}

void warning(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "Warning: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}
