#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "symtable.h"

// Initialize code generator
void codegen_init(FILE *output_file);

// Finalize code generation
void codegen_finalize();

// Generate code for program start
void codegen_program_start();

// Generate code for program end
void codegen_program_end();

// Generate code for function definition
void codegen_function_start(char *function_name);

// Generate code for function end
void codegen_function_end();

// Generate code for variable declaration
void codegen_declare_variable(Symbol *variable);

// Generate code for assignment
void codegen_assignment(Symbol *variable, Symbol *expression);

// Generate code for expressions (e.g., addition, subtraction, etc.)
void codegen_expression(Symbol *result, Symbol *lhs, char operator, Symbol *rhs);

// Generate code for if statement
void codegen_if_start(int label_number);
void codegen_if_else(int label_number);
void codegen_if_end(int label_number);

// Generate code for while loop
void codegen_while_start(int label_number);
void codegen_while_condition(int label_number);
void codegen_while_end(int label_number);

// Generate code for return statement
void codegen_return(Symbol *return_value);

#endif // CODEGEN_H
