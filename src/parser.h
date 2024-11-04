// parser.h
#ifndef PARSER_H
#define PARSER_H

#include "tokens.h"
#include "symtable.h"
#include "ast.h"
#include "scanner.h"

// Initializes the parser
void parser_init(Scanner *scanner);

// Starts parsing the input program
ASTNode* parse_program(Scanner *scanner);

// Parses a single function
ASTNode* parse_function(Scanner *scanner);

// Parses a statement (expression, if, while, return, etc.)
ASTNode* parse_statement(Scanner *scanner);

ASTNode* parse_parameter(Scanner *scanner);

ASTNode* parse_expression(Scanner *scanner);

#endif // PARSER_H
