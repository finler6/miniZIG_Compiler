/**
 * @file scanner.h
 *
 * Header file for the scanner module.
 * This module is responsible for reading the input file and returning tokens.
 *
 * IFJ Project 2024, Team 'xstepa77'
 *
 * @author <xlitvi02> Gleb Litvinchuk
 * @author <xstepa77> Pavel Stepanov
 * @author <xkovin00> Viktoriia Kovina
 * @author <xshmon00> Gleb Shmonin
 */
#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>
#include "tokens.h"

/**
 * Scanner structure.
 * Contains the input file, current line and column, and the current character.
 */
typedef struct {
    FILE *input;        
    int line;            
    int column;          
    int current_char;   
} Scanner;

// Scanner initialization function
void scanner_init(FILE *input_file, Scanner *scanner);

// Public function to get the next token
Token get_next_token();

#endif // SCANNER_H
