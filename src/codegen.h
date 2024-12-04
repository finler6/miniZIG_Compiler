/**
 * @file codegen.h
 *
 * Header file for the code generation module.
 *
 * IFJ Project 2024, Team 'xstepa77'
 *
 * @author
 *   <xlitvi02> Gleb Litvinchuk
 *   <xstepa77> Pavel Stepanov
 *   <xkovin00> Viktoriia Kovina
 *   <xshmon00> Gleb Shmonin
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include <stdio.h>

/** Structure to track usage of built-in functions */
typedef struct {
    bool uses_substring;
    bool uses_strcmp;
    bool uses_string;
} BuiltinFunctionUsage;

/** Entry for mapping temporary variables to AST nodes */
typedef struct TempVarMapEntry {
    ASTNode *node;
    char *key;      // Key identifier
    char *var_name; // Generated variable name
    struct TempVarMapEntry *next;
} TempVarMapEntry;

/** Structure to keep track of declared variables */
typedef struct DeclaredVar {
    char *var_name;
    struct DeclaredVar *next;
} DeclaredVar;

/** Structure to keep track of temporary variables */
typedef struct TempVar {
    char *name;
    struct TempVar *next;
} TempVar;

void codegen_init(const char *filename);
void codegen_finalize();
void codegen_generate_program(ASTNode *program_node);
void codegen_generate_function(ASTNode *function_node);
void codegen_generate_block(FILE *output, ASTNode *block_node, const char *current_function);
void codegen_generate_statement(FILE *output, ASTNode *statement_node, const char *current_function);
void codegen_generate_expression(FILE *output, ASTNode *node, const char *current_function);
void codegen_generate_function_call(FILE *output, ASTNode *node, const char *current_function);
void codegen_generate_variable_declaration(FILE *output, ASTNode *declaration_node);
void codegen_generate_assignment(FILE *output, ASTNode *assignment_node);
void codegen_generate_return(FILE *output, ASTNode *return_node, const char *current_function);
void codegen_generate_if(FILE *output, ASTNode *if_node);
void codegen_generate_while(FILE *output, ASTNode *while_node);
void codegen_generate_builtin_functions();
void codegen_declare_variables_in_statement(FILE *output, ASTNode *node);
void codegen_declare_variables_in_block(FILE *output, ASTNode *block_node);
void collect_variables_in_statement(ASTNode *node);
void collect_variables_in_block(ASTNode *node);
void collect_variables_in_function_call(ASTNode *node);
void collect_variables_in_expression(ASTNode *node);
int generate_unique_label();
const char *get_function_name_from_variable(const char *var_name);
bool is_function_parameter(ASTNode *function, const char *var_name);

#endif // CODEGEN_H
