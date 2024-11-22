#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h" 
#include <stdio.h>

typedef struct FunctionInfo {
    char *name;
    int param_count;
    char **parameters;
} FunctionInfo;

//static FunctionInfo *functions = NULL;
//static int function_count = 0;

void codegen_init(const char *filename);

void codegen_finalize();

void codegen_generate_program(ASTNode *program_node);

void codegen_generate_function(ASTNode *function_node);

void codegen_generate_block(FILE *output, ASTNode *block_node, const char* current_function);
void codegen_generate_statement(FILE *output, ASTNode *statement_node, const char* current_function);
void codegen_generate_expression(FILE *output, ASTNode *node, const char* current_function);
void codegen_generate_function_call(FILE *output, ASTNode *node, const char* current_function);
void codegen_generate_variable_declaration(FILE *output, ASTNode *declaration_node);
void codegen_generate_assignment(FILE *output, ASTNode *assignment_node);
void codegen_generate_return(FILE *output, ASTNode *return_node);

void codegen_generate_if(FILE *output, ASTNode *if_node);

void codegen_generate_while(FILE *output, ASTNode *while_node);

int generate_unique_label();

const char* get_function_name_from_variable(const char* var_name);

void collect_functions(ASTNode *program_node);

#endif // CODEGEN_H
