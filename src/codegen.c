#include <stdio.h>
#include <stdlib.h>
#include "codegen.h"
#include "symtable.h"
#include "error.h"

static FILE *output = NULL;

void codegen_init(FILE *output_file) {
    output = output_file;
}

void codegen_finalize() {
    output = NULL;
}

void codegen_program_start() {
    fprintf(output, ".IFJcode24\n");
}

void codegen_program_end() {
    fprintf(output, "EXIT int@0\n");  // End program
}

void codegen_function_start(char *function_name) {
    fprintf(output, "LABEL %s\n", function_name);
    fprintf(output, "PUSHFRAME\n");
}

void codegen_function_end() {
    fprintf(output, "POPFRAME\n");
    fprintf(output, "RETURN\n");
}

void codegen_declare_variable(Symbol *variable) {
    fprintf(output, "DEFVAR LF@%s\n", variable->name);
}

void codegen_assignment(Symbol *variable, Symbol *expression) {
    // Assuming expression has been evaluated and result stored
    fprintf(output, "MOVE LF@%s LF@%s\n", variable->name, expression->name);
}

// Generate code for binary expressions like addition, subtraction, etc.
void codegen_expression(Symbol *result, Symbol *lhs, char operator, Symbol *rhs) {
    switch (operator) {
        case '+':
            fprintf(output, "ADD LF@%s LF@%s LF@%s\n", result->name, lhs->name, rhs->name);
            break;
        case '-':
            fprintf(output, "SUB LF@%s LF@%s LF@%s\n", result->name, lhs->name, rhs->name);
            break;
        case '*':
            fprintf(output, "MUL LF@%s LF@%s LF@%s\n", result->name, lhs->name, rhs->name);
            break;
        case '/':
            fprintf(output, "DIV LF@%s LF@%s LF@%s\n", result->name, lhs->name, rhs->name);
            break;
        default:
            error_exit(ERR_INTERNAL, "Unsupported operator in expression.");
    }
}

void codegen_if_start(int label_number) {
    fprintf(output, "LABEL $if%d\n", label_number);
    fprintf(output, "JUMPIFEQ $else%d GF@cond bool@false\n", label_number);
}

void codegen_if_else(int label_number) {
    fprintf(output, "JUMP $endif%d\n", label_number);
    fprintf(output, "LABEL $else%d\n", label_number);
}

void codegen_if_end(int label_number) {
    fprintf(output, "LABEL $endif%d\n", label_number);
}

void codegen_while_start(int label_number) {
    fprintf(output, "LABEL $while%d\n", label_number);
}

void codegen_while_condition(int label_number) {
    fprintf(output, "JUMPIFEQ $endwhile%d GF@cond bool@false\n", label_number);
}

void codegen_while_end(int label_number) {
    fprintf(output, "JUMP $while%d\n", label_number);
    fprintf(output, "LABEL $endwhile%d\n", label_number);
}

void codegen_return(Symbol *return_value) {
    if (return_value != NULL) {
        fprintf(output, "MOVE LF@retval LF@%s\n", return_value->name);  // Move return value
    }
    fprintf(output, "POPFRAME\n");
    fprintf(output, "RETURN\n");
}
