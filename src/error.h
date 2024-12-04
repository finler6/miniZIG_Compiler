/**
 * @file error.h
 *
 * Error handling functions declarations and error codes.
 *
 * IFJ Project 2024, Team 'xstepa77'
 *
 * @author <xlitvi02> Gleb Litvinchuk
 * @author <xstepa77> Pavel Stepanov
 * @author <xkovin00> Viktoriia Kovina
 * @author <xshmon00> Gleb Shmonin
 */
#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "utils.h"

// Error codes as per specification
#define ERR_OK 0               // No error
#define ERR_LEXICAL 1          // Lexical error
#define ERR_SYNTACTICAL 2      // Syntactical error
#define ERR_SYNTAX 2  
#define ERR_SEMANTIC_UNDEF 3   // Semantic error - undefined function or variable
#define ERR_SEMANTIC_PARAMS 4  // Semantic error - incorrect function parameters
#define ERR_SEMANTIC_OTHER 5   // Semantic error - redefinition, assignment to const, etc.
#define ERR_SEMANTIC_RETURN 6  // Semantic error - missing or extra expression in return
#define ERR_SEMANTIC_TYPE 7    // Semantic error - type compatibility in expressions
#define ERR_SEMANTIC_INFER 8   // Semantic error - cannot infer variable type
#define ERR_SEMANTIC_UNUSED 9  // Semantic error - unused variable
#define ERR_SEMANTIC 10        // Semantic error - other
#define ERR_INTERNAL 99        // Internal compiler error

// Functions for error handling
void error_exit(int error_code, const char *format, ...);
void warning(const char *format, ...);

#endif // ERROR_H
