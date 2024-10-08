// parser.h
#ifndef PARSER_H
#define PARSER_H

#include "tokens.h"
#include "symtable.h"
#include "scanner.h"

// Initializes the parser
void parser_init(Scanner *scanner);

// Starts parsing the input program
void parse_program();

// Parses a single function
int parse_function();

// Parses a statement (expression, if, while, return, etc.)
void parse_statement();

void parse_parameter(Scanner *scanner);

DataType parse_expression();

#endif // PARSER_H
