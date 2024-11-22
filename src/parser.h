// parser.h
#ifndef PARSER_H
#define PARSER_H

#include "tokens.h"
#include "symtable.h"
#include "ast.h"
#include "scanner.h"
#include "string.h"

// Initializes the parser
void parser_init(Scanner *scanner);

// Starts parsing the input program
ASTNode* parse_program(Scanner *scanner);

// Parses a single function
ASTNode* parse_function(Scanner *scanner, bool is_definition);

// Parses a statement (expression, if, while, return, etc.)
ASTNode* parse_statement(Scanner *scanner, char *function_name);

ASTNode* parse_parameter(Scanner *scanner, char *function_name, bool is_definition);

ASTNode* parse_expression(Scanner *scanner, char *function_name);

typedef struct {
    const char *name;             
    DataType return_type;         
    DataType param_types[3];      
    int param_count;           
} BuiltinFunctionInfo;


extern BuiltinFunctionInfo builtin_functions[];

bool is_builtin_function(const char *identifier, Scanner *scanner);
ASTNode *convert_to_float_node(ASTNode *node);

size_t get_num_builtin_functions();

#endif // PARSER_H
