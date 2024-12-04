/**
 * @file parser.h
 *
 * Header file for the parser module. Contains function declarations for parsing the input program.
 *
 * IFJ Project 2024, Team 'xstepa77'
 *
 * @author <xlitvi02> Gleb Litvinchuk
 * @author <xstepa77> Pavel Stepanov
 * @author <xkovin00> Viktoriia Kovina
 * @author <xshmon00> Gleb Shmonin
 */
// parser.h
#ifndef PARSER_H
#define PARSER_H

#include "utils.h"
#include "tokens.h"
#include "symtable.h"
#include "ast.h"
#include "scanner.h"
#include "string.h"
#include "error.h"
#include "scanner.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Initializes the parser
void parser_init(Scanner *scanner);

// Starts parsing the input program
ASTNode* parse_program(Scanner *scanner);


// Data type parse functions
DataType parse_type(Scanner *scanner);
DataType parse_return_type(Scanner *scanner);

// Return types check
void check_return_types(ASTNode *function_node, DataType return_type, int *block_layer);
bool check_return_types_recursive(ASTNode *function_node, DataType return_type);
bool check_all_return_types(ASTNode *function_node, DataType return_type);

// Scopre check funtions
void scope_check_identifiers_in_tree(ASTNode *root);
bool scope_check(ASTNode *node_decl, ASTNode *node_identifier);

void parse_functions_declaration(Scanner *scanner, ASTNode *program_node);
bool type_convertion(ASTNode *main_node);
bool can_assign_type(DataType expected_type, DataType actual_type);
DataType detach_nullable(DataType type_nullable);
int get_builtin_function_index(const char *function_name);

bool is_builtin_function(const char *identifier, Scanner *scanner);
ASTNode *convert_to_float_node(ASTNode *node);
bool is_nullable(DataType type_nullable);

size_t get_num_builtin_functions();

/**
 * Structure containing information about a built-in function (ifj functions)
 */
typedef struct {
    const char *name;             
    DataType return_type;         
    DataType param_types[3];      
    int param_count;           
} BuiltinFunctionInfo;

// Built-in functions dictionary
extern BuiltinFunctionInfo builtin_functions[];

// Functions to manage scopes
int current_scope_id();
void enter_scope();
void exit_scope();

#endif // PARSER_H
