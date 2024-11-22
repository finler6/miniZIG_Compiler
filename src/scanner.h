#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>
#include "tokens.h"


typedef struct {
    FILE *input;        
    int line;            
    int column;          
    int current_char;   
} Scanner;

void scanner_init(FILE *input_file, Scanner *scanner);

void scanner_free(Scanner *scanner);

Token get_next_token();

int scanner_is_keyword(const char *lexeme, Scanner *scanner);
int scanner_is_operator(int c, Scanner *scanner);
int scanner_is_delimiter(int c, Scanner *scanner);

#endif // SCANNER_H
