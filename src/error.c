/*
 * Project Name: Implementation of the IFJ24 Compiler
 * Authors: 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "error.h"

void error_exit(int error_code, const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
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
