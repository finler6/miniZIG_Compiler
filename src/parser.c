#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "tokens.h"
#include "scanner.h"
#include "symtable.h"
#include "error.h"

// Forward declarations of helper functions
static void expect_token(TokenType expected_type, Scanner *scanner);
static void parse_block(Scanner *scanner);
static void parse_variable_declaration(Scanner *scanner);
static void parse_if_statement(Scanner *scanner);
static void parse_while_statement(Scanner *scanner);
static void parse_return_statement(Scanner *scanner);
static DataType parse_primary_expression(Scanner *scanner);
static DataType parse_binary_operation(Scanner *scanner);
DataType parse_type(Scanner *scanner);             // Объявляем функцию заранее
DataType parse_return_type(Scanner *scanner);      // Объявляем функцию заранее

// Global token storage
static Token current_token;
static SymTable symtable;  // Global symbol table for the program

void parser_init(Scanner *scanner) {
    printf("Initializing parser...\n");
    // Initialize the symbol table
    symtable_init(&symtable);
    // Get the first token to start parsing
    current_token = get_next_token(scanner);
}

// Function to start parsing the program
void parse_program(Scanner *scanner) {
    while (current_token.type != TOKEN_EOF) {
        //print_token(current_token); // Debugging token output
        if (current_token.type == TOKEN_PUB) {
            parse_function(scanner);
        } else {
            error_exit(ERR_SYNTAX, "Expected function definition.");
        }
    }
}

// Function to parse a function definition
// Function to parse a function definition
int parse_function(Scanner *scanner) {
    expect_token(TOKEN_PUB, scanner);  // Ожидаем ключевое слово 'pub'
    expect_token(TOKEN_FN, scanner);   // Ожидаем ключевое слово 'fn'

    if (current_token.type != TOKEN_IDENTIFIER) {
        error_exit(ERR_SYNTAX, "Expected function name.");
    }

    if (current_token.lexeme == NULL) {
        error_exit(ERR_INTERNAL, "Lexeme is NULL before strdup.");
    }
    char *function_name = string_duplicate(current_token.lexeme);
    if (function_name == NULL) {
        error_exit(ERR_INTERNAL, "Memory allocation failed for function name.");
    }
    current_token = get_next_token(scanner);


    expect_token(TOKEN_LEFT_PAREN, scanner);  // '('

    // Parse function parameters
    if (current_token.type != TOKEN_RIGHT_PAREN) {
        parse_parameter(scanner);  // Разбираем параметры функции
        while (current_token.type == TOKEN_COMMA) {
            current_token = get_next_token(scanner);
            parse_parameter(scanner);
        }
    }
    expect_token(TOKEN_RIGHT_PAREN, scanner);  // ')'

    // Ожидаем возвращаемый тип
    DataType return_type = parse_return_type(scanner);  // Функция, разбирающая возвращаемый тип
    
    // Добавляем функцию в таблицу символов
    /*Symbol *function_symbol = symtable_search(&symtable, function_name);
    if (function_symbol != NULL) {
        error_exit(ERR_SEMANTIC, "Function already defined.");
    }
    Symbol new_function = {.name = function_name, .symbol_type = SYMBOL_FUNCTION, .data_type = return_type, .is_defined = true};
    symtable_insert(&symtable, function_name, &new_function);*/

    // Parse function body (block)
    parse_block(scanner);
    return 0;
}

void parse_parameter(Scanner *scanner) {
    // Ожидаем имя параметра
    if (current_token.type != TOKEN_IDENTIFIER) {
        error_exit(ERR_SYNTAX, "Expected parameter name.");
    }


    printf("current_token.lexeme before param: %s\n", current_token.lexeme);
    char *param_name = strdup(current_token.lexeme);
    if (param_name == NULL) {
        error_exit(ERR_INTERNAL, "Memory allocation failed for parameter name.");
    }

    current_token = get_next_token(scanner);

    // Ожидаем двоеточие ':'
    expect_token(TOKEN_COLON, scanner);

    // Ожидаем тип параметра
    DataType param_type = parse_type(scanner);

    // Проверяем существование параметра в таблице символов
    /*printf("param_name before symtable_search: %s\n", param_name);
    Symbol *param_symbol = symtable_search(&symtable, param_name);
    if (param_symbol != NULL) {
        free(param_name);
        error_exit(ERR_SEMANTIC, "Parameter already defined.");
    }

    // Создаем новый символ и добавляем его в таблицу символов
    Symbol *new_param = (Symbol *)malloc(sizeof(Symbol));
    if (new_param == NULL) {
        free(param_name);
        error_exit(ERR_INTERNAL, "Memory allocation failed for parameter symbol.");
    }

    new_param->name = param_name;
    new_param->symbol_type = SYMBOL_PARAMETER;
    new_param->data_type = param_type;
    new_param->is_defined = true;
    new_param->next = NULL;

    symtable_insert(&symtable, param_name, new_param);*/

    // Не обрабатываем запятую и не вызываем parse_parameter рекурсивно
}


DataType parse_type(Scanner *scanner) {
    if (current_token.type == TOKEN_I32) {
        current_token = get_next_token(scanner);  // Обновляем токен после разбора типа
        return TYPE_INT;
    } else if (current_token.type == TOKEN_F64) {
        current_token = get_next_token(scanner);  // Обновляем токен после разбора типа
        return TYPE_FLOAT;
    } else if (current_token.type == TOKEN_VOID) {
        current_token = get_next_token(scanner);  // Обновляем токен после разбора типа
        return TYPE_VOID;
    } else {
        error_exit(ERR_SYNTAX, "Expected type.");
        return TYPE_UNKNOWN;
    }
}
// Function to parse the return type (same as parameter type parsing)
DataType parse_return_type(Scanner *scanner) {
    if (current_token.type == TOKEN_I32) {
        current_token = get_next_token(scanner);  // Переходим к следующему токену
        return TYPE_INT;
    } else if (current_token.type == TOKEN_F64) {
        current_token = get_next_token(scanner);
        return TYPE_FLOAT;
    } else if (current_token.type == TOKEN_VOID) {
        current_token = get_next_token(scanner);
        return TYPE_VOID;
    } else {
        error_exit(ERR_SYNTAX, "Expected return type.");
        return TYPE_UNKNOWN;
    }
}

// Parses a block of statements enclosed in {}
static void parse_block(Scanner *scanner) {
    expect_token(TOKEN_LEFT_BRACE, scanner);  // Ожидаем '{'

    // Парсим операторы в теле функции до тех пор, пока не встретим '}'
    while (current_token.type != TOKEN_RIGHT_BRACE) {
        parse_statement(scanner);
    }

    // После завершения тела функции ожидаем закрывающую скобку '}'
    expect_token(TOKEN_RIGHT_BRACE, scanner);
}

// Function to parse a statement
void parse_statement(Scanner *scanner) {
    if (current_token.type == TOKEN_VAR || current_token.type == TOKEN_CONST) {
        parse_variable_declaration(scanner);
    } else if (current_token.type == TOKEN_IF) {
        parse_if_statement(scanner);
    } else if (current_token.type == TOKEN_WHILE) {
        parse_while_statement(scanner);
    } else if (current_token.type == TOKEN_RETURN) {
        parse_return_statement(scanner);  // Обрабатываем return
    } else {
        // Parse as an expression statement
        parse_expression(scanner);
        expect_token(TOKEN_SEMICOLON, scanner);  // Expressions end with a semicolon
    }
}

// Function to parse a variable declaration
void parse_variable_declaration(Scanner *scanner) {
    TokenType var_type = current_token.type;  // 'var' or 'const'
    current_token = get_next_token(scanner);

    if (current_token.type != TOKEN_IDENTIFIER) {
        error_exit(ERR_SYNTAX, "Expected variable name.");
    }
    if (current_token.lexeme == NULL) {
        error_exit(ERR_INTERNAL, "Lexeme is NULL before strdup.");
    }
    char *variable_name = string_duplicate(current_token.lexeme);
    if (variable_name == NULL) {
        error_exit(ERR_INTERNAL, "Memory allocation failed for variable name.");
    }
    current_token = get_next_token(scanner);


    expect_token(TOKEN_ASSIGN, scanner);  // '='

    // Parse the expression being assigned
    DataType expr_type = parse_expression(scanner);

    // Semantically check if the types are compatible
    if (var_type == TOKEN_VAR && expr_type == TYPE_UNKNOWN) {
        error_exit(ERR_SEMANTIC, "Unknown data type assignment.");
    }

    expect_token(TOKEN_SEMICOLON, scanner);  // Variable declaration ends with a semicolon

    /*// Add the variable to the symbol table
    Symbol *symbol = symtable_search(&symtable, variable_name);
    if (symbol != NULL) {
        error_exit(ERR_SEMANTIC, "Variable already defined.");
    }

    Symbol new_var = {.name = variable_name, .symbol_type = SYMBOL_VARIABLE, .data_type = expr_type, .is_defined = true};
    symtable_insert(&symtable, variable_name, &new_var);*/
}

// Function to parse an if statement
void parse_if_statement(Scanner *scanner) {
    expect_token(TOKEN_IF, scanner);  // 'if'
    expect_token(TOKEN_LEFT_PAREN, scanner);  // '('

    DataType cond_type = parse_expression(scanner);  // Condition expression

    // Semantic check for boolean condition
    if (cond_type != TYPE_BOOL) {
        error_exit(ERR_SEMANTIC, "Condition in if statement must be boolean.");
    }

    expect_token(TOKEN_RIGHT_PAREN, scanner);  // ')'

    parse_block(scanner);  // 'if' body

    // Optional 'else' part
    if (current_token.type == TOKEN_ELSE) {
        current_token = get_next_token(scanner);
        parse_block(scanner);  // 'else' body
    }
}

// Function to parse a while statement
void parse_while_statement(Scanner *scanner) {
    expect_token(TOKEN_WHILE, scanner);  // 'while'
    expect_token(TOKEN_LEFT_PAREN, scanner);  // '('

    DataType cond_type = parse_expression(scanner);  // Condition expression

    // Semantic check for boolean condition
    if (cond_type != TYPE_BOOL) {
        error_exit(ERR_SEMANTIC, "Condition in while statement must be boolean.");
    }

    expect_token(TOKEN_RIGHT_PAREN, scanner);  // ')'

    parse_block(scanner);  // 'while' body
}

// Function to parse a return statement
// Function to parse a return statement
void parse_return_statement(Scanner *scanner) {
    printf("Parsing return statement\n");  // Debugging output
    expect_token(TOKEN_RETURN, scanner);

    if (current_token.type != TOKEN_SEMICOLON, scanner) {
        parse_expression(scanner);
    }

    expect_token(TOKEN_SEMICOLON, scanner);
    printf("Finished parsing return statement\n");  // Debugging output
}

// Function to parse an expression and return its data type
DataType parse_expression(Scanner *scanner) {
    return parse_primary_expression(scanner);  // Parse the first operand or literal

    // If there is a binary operation, check its type
    if (current_token.type == TOKEN_PLUS || current_token.type == TOKEN_MINUS ||
        current_token.type == TOKEN_MULTIPLY || current_token.type == TOKEN_DIVIDE) {
        return parse_binary_operation(scanner);
    }
    return TYPE_UNKNOWN;  // Default case, if no specific type was identified
}


// Parses a primary expression (literal, identifier, or parenthesized expression)
static DataType parse_primary_expression(Scanner *scanner) {
    if (current_token.type == TOKEN_INT_LITERAL) {
        current_token = get_next_token(scanner);
        return TYPE_INT;
    } else if (current_token.type == TOKEN_FLOAT_LITERAL) {
        current_token = get_next_token(scanner);
        return TYPE_FLOAT;
    } else if (current_token.type == TOKEN_STRING_LITERAL) {
        current_token = get_next_token(scanner);
        return TYPE_STRING;
    } else if (current_token.type == TOKEN_IDENTIFIER) {
        // Check if the identifier is in the symbol table
        char *identifier_name = current_token.lexeme;
        /*Symbol *symbol = symtable_search(&symtable, identifier_name);
        if (symbol == NULL) {
            error_exit(ERR_SEMANTIC, "Undefined variable or function.");
        }*/

        current_token = get_next_token(scanner);

        // If it's a function call, handle it
        if (current_token.type == TOKEN_LEFT_PAREN) {
            current_token = get_next_token(scanner);  // Skip '('
            if (current_token.type != TOKEN_RIGHT_PAREN) {
                // Parse arguments if any
                parse_expression(scanner);
                while (current_token.type == TOKEN_COMMA) {
                    current_token = get_next_token(scanner);
                    parse_expression(scanner);
                }
            }
            expect_token(TOKEN_RIGHT_PAREN, scanner);  // Expect ')'
        }

        return symbol->data_type;  // Return the type of the variable or function
    } else if (current_token.type == TOKEN_LEFT_PAREN) {
        // Parenthesized expression
        current_token = get_next_token(scanner);  // Skip '('
        DataType expr_type = parse_expression(scanner);  // Parse the expression inside parentheses
        expect_token(TOKEN_RIGHT_PAREN, scanner);  // Expect ')'
        return expr_type;
    } else {
        error_exit(ERR_SYNTAX, "Expected literal, identifier, or '(' for expression.");
        return TYPE_UNKNOWN;
    }
}


// Parses a binary operation and returns the resulting type
static DataType parse_binary_operation(Scanner *scanner) {
    TokenType operator_type = current_token.type;
    current_token = get_next_token(scanner);  // Move to the next token after operator

    DataType rhs_type = parse_primary_expression(scanner);  // Parse the right-hand side of the operation

    // Check the compatibility of types for binary operations
    if (operator_type == TOKEN_PLUS || operator_type == TOKEN_MINUS ||
        operator_type == TOKEN_MULTIPLY || operator_type == TOKEN_DIVIDE) {
        // For now, assuming operations between INT and FLOAT are allowed
        return rhs_type;
    }

    return TYPE_UNKNOWN;
}


// Function to expect a specific token type
static void expect_token(TokenType expected_type, Scanner *scanner) {
    if (current_token.type != expected_type) {
        printf("Expected token: %d, got token: %d\n", expected_type, current_token.type);
        error_exit(ERR_SYNTAX, "Unexpected token.");
    }
    current_token = get_next_token(scanner);
}
