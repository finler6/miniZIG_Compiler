/**
 * @file scanner.c
 *
 * Implementation of the scanner module.
 * The scanner reads the input file character by character and performs lexical analysis.
 *
 * IFJ Project 2024, Team 'xstepa77'
 *
 * @author <xlitvi02> Gleb Litvinchuk
 * @author <xstepa77> Pavel Stepanov
 * @author <xkovin00> Viktoriia Kovina
 * @author <xshmon00> Gleb Shmonin
 */

#include "scanner.h"
#include "error.h"
#include "tokens.h"
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LEXEME_LENGTH 256

// Function prototypes
static void skip_whitespace_and_comments(Scanner *scanner);
static Token scan_identifier_or_keyword(Scanner *scanner);
static Token scan_number_literal(Scanner *scanner);
static Token scan_string_literal(Scanner *scanner);
static Token scan_operator_or_delimiter(Scanner *scanner);
static Token get_next_token_internal(Scanner *scanner);

/**
 * Ensure the buffer does not exceed its maximum length
 */
static void check_buffer_length(int index)
{
    if (index >= MAX_LEXEME_LENGTH - 1)
    {
        error_exit(ERR_LEXICAL, "Literal too long.");
    }
}

/**
 * Helper function to skip whitespace and comments
 */
static void skip_whitespace_and_comments(Scanner *scanner)
{
    bool skipping = true;
    while (skipping)
    {
        // Skip whitespace characters
        while (isspace(scanner->current_char))
        {
            if (scanner->current_char == '\n')
            {
                scanner->line++;
                scanner->column = 0;
            }
            else
            {
                scanner->column++;
            }
            scanner->current_char = fgetc(scanner->input);
        }

        // Check for comments
        if (scanner->current_char == '/')
        {
            int next_char = fgetc(scanner->input);
            if (next_char == '/')
            {
                // Single-line comment, skip until end of line
                while (scanner->current_char != '\n' && scanner->current_char != EOF)
                {
                    scanner->current_char = fgetc(scanner->input);
                    scanner->column++;
                }
                continue; // Continue skipping whitespace and comments
            }
            else
            {
                // Not a comment, return the character
                ungetc(next_char, scanner->input);
                skipping = false;
            }
        }
        else
        {
            skipping = false;
        }
    }
}

/**
 * Recognize keywords or identifiers
 */
static Token recognize_keyword_or_identifier(const char *lexeme, Scanner *scanner)
{
    Token token;
    token.lexeme = string_duplicate(lexeme);
    token.line = scanner->line;
    token.column = scanner->column - strlen(lexeme);

    if (strcmp(lexeme, "const") == 0)
        token.type = TOKEN_CONST;
    else if (strcmp(lexeme, "var") == 0)
        token.type = TOKEN_VAR;
    else if (strcmp(lexeme, "if") == 0)
        token.type = TOKEN_IF;
    else if (strcmp(lexeme, "else") == 0)
        token.type = TOKEN_ELSE;
    else if (strcmp(lexeme, "while") == 0)
        token.type = TOKEN_WHILE;
    else if (strcmp(lexeme, "return") == 0)
        token.type = TOKEN_RETURN;
    else if (strcmp(lexeme, "fn") == 0)
        token.type = TOKEN_FN;
    else if (strcmp(lexeme, "pub") == 0)
        token.type = TOKEN_PUB;
    else if (strcmp(lexeme, "void") == 0)
        token.type = TOKEN_VOID;
    else if (strcmp(lexeme, "null") == 0)
        token.type = TOKEN_NULL;
    else if (strcmp(lexeme, "i32") == 0)
        token.type = TOKEN_I32;
    else if (strcmp(lexeme, "f64") == 0)
        token.type = TOKEN_F64;
    else if (strcmp(lexeme, "[]u8") == 0)
        token.type = TOKEN_U8;
    else if (strcmp(lexeme, "@import") == 0)
        token.type = TOKEN_IMPORT;
    else
    {
        if (strchr(lexeme, '@') != NULL)
        {
            error_exit(ERR_LEXICAL, "Invalid identifier: '@' symbol is not allowed.");
        }
        token.type = TOKEN_IDENTIFIER;
    }
    return token;
}

/**
 * Scan identifiers or keywords
 */
static Token scan_identifier_or_keyword(Scanner *scanner)
{
    char lexeme_buffer[MAX_LEXEME_LENGTH];
    int index = 0;
    while (isalnum(scanner->current_char) || scanner->current_char == '_' || scanner->current_char == '@' || scanner->current_char == '[' || scanner->current_char == ']')
    {
        check_buffer_length(index);
        lexeme_buffer[index++] = scanner->current_char;
        scanner->current_char = fgetc(scanner->input);
        scanner->column++;
    }
    if (index == 0)
    {
        error_exit(ERR_LEXICAL, "Lexeme buffer is empty.");
    }

    lexeme_buffer[index] = '\0';

    return recognize_keyword_or_identifier(lexeme_buffer, scanner);
}

/**
 * Read a sequence of digits into the buffer
 */
static void read_digits(Scanner *scanner, char *buffer, int *index)
{
    while (isdigit(scanner->current_char))
    {
        check_buffer_length(*index);
        buffer[(*index)++] = scanner->current_char;
        scanner->current_char = fgetc(scanner->input);
        scanner->column++;
    }
}

/**
 * Handle the exponent part of a float literal
 */
static void handle_exponent(Scanner *scanner, char *buffer, int *index)
{
    buffer[(*index)++] = scanner->current_char;
    scanner->current_char = fgetc(scanner->input);
    scanner->column++;

    // Optional '+' or '-'
    if (scanner->current_char == '+' || scanner->current_char == '-')
    {
        check_buffer_length(*index);
        buffer[(*index)++] = scanner->current_char;
        scanner->current_char = fgetc(scanner->input);
        scanner->column++;
    }

    // At least one digit required in exponent
    if (!isdigit(scanner->current_char))
    {
        error_exit(ERR_LEXICAL, "Invalid float literal exponent.");
    }

    read_digits(scanner, buffer, index);
}

/**
 * Main function to scan numeric literals (int and float)
 */
static Token scan_number_literal(Scanner *scanner)
{
    char number_buffer[MAX_LEXEME_LENGTH];
    int index = 0;
    int is_float = 0;

    // Read integer part
    read_digits(scanner, number_buffer, &index);

    // Handle decimal point for float literals
    if (scanner->current_char == '.')
    {
        is_float = 1;
        check_buffer_length(index);
        number_buffer[index++] = scanner->current_char;
        scanner->current_char = fgetc(scanner->input);
        scanner->column++;

        // At least one digit required after the decimal point
        if (!isdigit(scanner->current_char))
        {
            error_exit(ERR_LEXICAL, "Invalid float literal.");
        }

        read_digits(scanner, number_buffer, &index);
    }

    // Handle exponent part for float literals
    if (scanner->current_char == 'e' || scanner->current_char == 'E')
    {
        is_float = 1;
        handle_exponent(scanner, number_buffer, &index);
    }

    number_buffer[index] = '\0';

    // Validate integer literals: non-zero numbers should not start with '0'
    if (!is_float && strlen(number_buffer) > 1 && number_buffer[0] == '0')
    {
        error_exit(ERR_LEXICAL, "Invalid integer literal with leading zero.");
    }

    // Create the token
    Token token;
    token.lexeme = string_duplicate(number_buffer);
    token.line = scanner->line;
    token.column = scanner->column - strlen(number_buffer);

    token.type = is_float ? TOKEN_FLOAT_LITERAL : TOKEN_INT_LITERAL;
    return token;
}

/**
 * Handle escape sequences in a string literal
 */
static char handle_escape_sequence(Scanner *scanner)
{
    scanner->current_char = fgetc(scanner->input);
    scanner->column++;

    switch (scanner->current_char)
    {
    case 'n': return '\n';
    case 't': return '\t';
    case 'r': return '\r';
    case '"': return '"';
    case '\\': return '\\';
    case 'x':
    {
        char hex_digits[3] = {0};
        for (int i = 0; i < 2; i++)
        {
            scanner->current_char = fgetc(scanner->input);
            scanner->column++;
            if (!isxdigit(scanner->current_char))
            {
                error_exit(ERR_LEXICAL, "Invalid escape sequence in string literal.");
            }
            hex_digits[i] = scanner->current_char;
        }
        return (char)strtol(hex_digits, NULL, 16);
    }
    default:
        error_exit(ERR_LEXICAL, "Invalid escape sequence in string literal.");
    }
    return '\0'; // Unreachable
}

/**
 * Main function to scan string literals
 */
static Token scan_string_literal(Scanner *scanner)
{
    char string_buffer[MAX_LEXEME_LENGTH];
    int index = 0;

    scanner->current_char = fgetc(scanner->input); // Skip the opening quote
    scanner->column++;

    while (scanner->current_char != '"' && scanner->current_char != EOF)
    {
        if (scanner->current_char == '\\')
        {
            check_buffer_length(index);
            string_buffer[index++] = handle_escape_sequence(scanner);
        }
        else if (scanner->current_char == '\n')
        {
            error_exit(ERR_LEXICAL, "Unterminated string literal."); // Strings cannot contain newlines
        }
        else if (scanner->current_char < 32 || scanner->current_char == 35 || scanner->current_char == 92)
        {
            error_exit(ERR_LEXICAL, "Invalid character in string literal."); // ASCII > 32, not '#', not '\\'
        }
        else
        {
            check_buffer_length(index);
            string_buffer[index++] = scanner->current_char;
        }

        scanner->current_char = fgetc(scanner->input);
        scanner->column++;
    }

    if (scanner->current_char != '"')
    {
        error_exit(ERR_LEXICAL, "Unterminated string literal.");
    }

    scanner->current_char = fgetc(scanner->input); // Skip the closing quote
    scanner->column++;

    string_buffer[index] = '\0';

    Token token;
    token.type = TOKEN_STRING_LITERAL;
    token.lexeme = string_duplicate(string_buffer);
    token.line = scanner->line;
    token.column = scanner->column - strlen(string_buffer) - 2; // Approximation

    return token;
}

/**
 * Create a simple token for single-character symbols
 */
static Token create_simple_token(Scanner *scanner, TokenType type)
{
    Token token;
    token.type = type;
    token.line = scanner->line;
    token.column = scanner->column;

    token.lexeme = safe_malloc(2);
    token.lexeme[0] = scanner->current_char;
    token.lexeme[1] = '\0';

    scanner->current_char = fgetc(scanner->input);
    scanner->column++;

    return token;
}

/**
 * Scan operators and delimiters
 */
static Token scan_operator_or_delimiter(Scanner *scanner)
{
    Token token;
    token.line = scanner->line;
    token.column = scanner->column;

    switch (scanner->current_char)
    {
    case '+':
        return create_simple_token(scanner, TOKEN_PLUS);
    case '-':
        return create_simple_token(scanner, TOKEN_MINUS);
    case '*':
        return create_simple_token(scanner, TOKEN_MULTIPLY);
    case '/':
        return create_simple_token(scanner, TOKEN_DIVIDE);
    case '=':
    {
        int next_char = fgetc(scanner->input);
        scanner->column++;
        if (next_char == '=')
        {
            token.type = TOKEN_EQUAL;
            token.lexeme = string_duplicate("==");
            scanner->current_char = fgetc(scanner->input);
            scanner->column++;
        }
        else
        {
            ungetc(next_char, scanner->input);
            token.lexeme = string_duplicate("=");
            token.type = TOKEN_ASSIGN;
            scanner->current_char = fgetc(scanner->input);
            scanner->column++;
        }
        return token;
    }
    case '(':
        return create_simple_token(scanner, TOKEN_LEFT_PAREN);
    case ')':
        return create_simple_token(scanner, TOKEN_RIGHT_PAREN);
    case '{':
        return create_simple_token(scanner, TOKEN_LEFT_BRACE);
    case '}':
        return create_simple_token(scanner, TOKEN_RIGHT_BRACE);
    case '|':
        return create_simple_token(scanner, TOKEN_PIPE);
    case ':':
        return create_simple_token(scanner, TOKEN_COLON);
    case ';':
        return create_simple_token(scanner, TOKEN_SEMICOLON);
    case '<':
    {
        int next_char = fgetc(scanner->input);
        scanner->column++;
        if (next_char == '=')
        {
            token.type = TOKEN_LESS_EQUAL;
            token.lexeme = string_duplicate("<=");
            scanner->current_char = fgetc(scanner->input);
            scanner->column++;
        }
        else
        {
            ungetc(next_char, scanner->input);
            token.lexeme = string_duplicate("<");
            token.type = TOKEN_LESS;
            scanner->current_char = fgetc(scanner->input);
            scanner->column++;
        }
        return token;
    }
    case '>':
    {
        int next_char = fgetc(scanner->input);
        scanner->column++;
        if (next_char == '=')
        {
            token.type = TOKEN_GREATER_EQUAL;
            token.lexeme = string_duplicate(">=");
            scanner->current_char = fgetc(scanner->input);
            scanner->column++;
        }
        else
        {
            ungetc(next_char, scanner->input);
            token.lexeme = string_duplicate(">");
            token.type = TOKEN_GREATER;
            scanner->current_char = fgetc(scanner->input);
            scanner->column++;
        }
        return token;
    }
    case ',':
        return create_simple_token(scanner, TOKEN_COMMA);
    case '.':
        return create_simple_token(scanner, TOKEN_DOT);
    case '?':
        return create_simple_token(scanner, TOKEN_QUESTION);
    case '!':
    {
        int next_char = fgetc(scanner->input);
        scanner->column++;
        if (next_char == '=')
        {
            token.type = TOKEN_NOT_EQUAL;
            token.lexeme = string_duplicate("!=");
            scanner->current_char = fgetc(scanner->input);
            scanner->column++;
        }
        else
        {
            error_exit(ERR_LEXICAL, "Unknown operator '!' detected.");
        }
        return token;
    }
    default:
        error_exit(ERR_LEXICAL, "Unknown character: '%c'", scanner->current_char);
    }

    // In case of an unexpected situation
    error_exit(ERR_LEXICAL, "Unknown character: '%c'", scanner->current_char);
    return token; // Never reached
}

/**
 * Get the next token
 */
static Token get_next_token_internal(Scanner *scanner)
{
    skip_whitespace_and_comments(scanner);

    if (scanner->current_char == EOF)
    {
        Token token;
        token.type = TOKEN_EOF;
        token.lexeme = string_duplicate("EOF");
        token.line = scanner->line;
        token.column = scanner->column;
        return token;
    }

    if (isalpha(scanner->current_char) || scanner->current_char == '_' || scanner->current_char == '@' || scanner->current_char == '[' || scanner->current_char == ']') 
    {
        return scan_identifier_or_keyword(scanner);
    }
    else if (isdigit(scanner->current_char))
    {
        return scan_number_literal(scanner);
    }
    else if (scanner->current_char == '"')
    {
        return scan_string_literal(scanner);
    }
    else
    {
        return scan_operator_or_delimiter(scanner);
    }
}

/**
 * Public function to get the next token
 */
Token get_next_token(Scanner *scanner)
{
    return get_next_token_internal(scanner);
}

/**
 * Initialize the scanner
 */
void scanner_init(FILE *input_file, Scanner *scanner)
{
    scanner->input = input_file;
    scanner->current_char = fgetc(scanner->input);
    scanner->column = 1;
    scanner->line = 1;
}

/**
 * Free the memory occupied by a token
 */
void free_token(Token *token)
{
    if (token->lexeme != NULL)
    {
        safe_free(token->lexeme);
        token->lexeme = NULL;
    }
}
