#include "parser.h"
#include "error.h"
#include "scanner.h"
#include "string.h"
#include "symtable.h"
#include "tokens.h"
#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG_PARSER
#ifdef DEBUG_PARSER
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

// Forward declarations of helper functions
static void expect_token(TokenType expected_type, Scanner *scanner);
static ASTNode *parse_block(Scanner *scanner, char *function_name);
static ASTNode *parse_variable_declaration(Scanner *scanner, char *function_name);
static ASTNode *parse_variable_assigning(Scanner *scanner, char *function_name);
static ASTNode *parse_if_statement(Scanner *scanner, char *function_name);
static ASTNode *parse_while_statement(Scanner *scanner, char *function_name);
static ASTNode *parse_return_statement(Scanner *scanner, char *function_name);
static ASTNode *parse_import(Scanner *scanner);
static ASTNode *parse_primary_expression(Scanner *scanner, char *function_name);
static ASTNode *parse_binary_operation(Scanner *scanner, ASTNode *left_node, char *function_name);
static ASTNode *check_and_convert_expression(ASTNode *node, DataType expected_type, const char *variable_name);
DataType parse_type(Scanner *scanner);        // Объявляем функцию заранее
DataType parse_return_type(Scanner *scanner); // Объявляем функцию заранее
void check_return_types(ASTNode *function_node, DataType return_type, int *block_layer);
bool type_convertion(ASTNode *main_node);
bool is_subtype_nullable(DataType type_nullable, DataType type);
bool is_nullable(DataType type_nullable);
DataType detach_nullable(DataType type_nullable);
void parse_functions_declaration(Scanner *scanner, ASTNode *program_node);
void scope_check_identifiers_in_tree(ASTNode *root);
bool scope_check(ASTNode *node_decl, ASTNode *node_identifier);
int get_builtin_function_index(const char *function_name);

// Global token storage
static Token current_token;
static SymTable symtable; // Global symbol table for the program

BuiltinFunctionInfo builtin_functions[] = {
    {"readstr", TYPE_U8_NULLABLE, {}, 0},
    {"readi32", TYPE_INT_NULLABLE, {}, 0},
    {"readf64", TYPE_FLOAT_NULLABLE, {}, 0},
    {"write", TYPE_VOID, {TYPE_ALL}, 1},
    {"i2f", TYPE_FLOAT, {TYPE_INT}, 1},
    {"f2i", TYPE_INT, {TYPE_FLOAT}, 1},
    {"string", TYPE_U8, {TYPE_U8}, 1},
    {"length", TYPE_INT, {TYPE_U8}, 1},
    {"concat", TYPE_U8, {TYPE_U8, TYPE_U8}, 2},
    {"substring", TYPE_U8_NULLABLE, {TYPE_U8, TYPE_INT, TYPE_INT}, 3},
    {"strcmp", TYPE_INT, {TYPE_U8, TYPE_U8}, 2},
    {"ord", TYPE_INT, {TYPE_U8, TYPE_INT}, 2},
    {"chr", TYPE_U8, {TYPE_INT}, 1}
    // Добавьте другие функции по необходимости
};
size_t get_num_builtin_functions()
{
    return sizeof(builtin_functions) / sizeof(builtin_functions[0]);
}

bool is_builtin_function(const char *identifier, Scanner *scanner)
{
    if (strcmp(identifier, "ifj") != 0)
    {
        return false;
    }
    LOG("!!!!DEBUG_PARSER: Recognized 'ifj' keyword\n");
    current_token = get_next_token(scanner);
    expect_token(TOKEN_DOT, scanner);
    identifier = current_token.lexeme;
    LOG("!!!!DEBUG_PARSER: Recognized identifier: %s\n", identifier);
    for (size_t i = 0; i < sizeof(builtin_functions) / sizeof(builtin_functions[0]); i++)
    {
        if (strcmp(identifier, builtin_functions[i].name) == 0)
        {
            LOG("!!!!DEBUG_PARSER: Recognized builtin function: %s\n", identifier);
            return true;
        }
    }
    return false;
}
// Определение типа данных встроенных функций с использованием словаря
DataType get_builtin_function_type(const char *function_name)
{
    size_t num_functions = sizeof(builtin_functions) / sizeof(builtin_functions[0]);
    for (size_t i = 0; i < num_functions; i++)
    {
        if (strcmp(function_name, builtin_functions[i].name) == 0)
        {
            LOG("DEBUG_PARSER: Function return type: %d\n", builtin_functions[i].return_type);
            return builtin_functions[i].return_type;
        }
    }
    return TYPE_UNKNOWN; // Неизвестная функция
}

int get_builtin_function_index(const char *function_name)
{
    size_t num_functions = sizeof(builtin_functions) / sizeof(builtin_functions[0]);
    for (size_t i = 0; i < num_functions; i++)
    {
        if (strcmp(function_name, builtin_functions[i].name) == 0)
        {
            LOG("DEBUG_PARSER: Function return index: %d\n", builtin_functions[i].return_type);
            return i;
        }
    }
    return -1; // Неизвестная функция
}

void parser_init(Scanner *scanner)
{
    LOG("DEBUG_PARSER: Initializing parser...\n");
    // Initialize the symbol table
    symtable_init(&symtable);
    // Get the first token to start parsing
    current_token = get_next_token(scanner);
}

ASTNode *parse_program(Scanner *scanner)
{
    LOG("DEBUG_PARSER: Parsing program\n");

    // Создаем узел программы
    ASTNode *program_node = create_program_node();
    ASTNode current_function_copy; // Указатель на текущую функцию
    ASTNode *current_function_carette = NULL;
    // Парсим обязательное выражение импорта и добавляем его в дерево
    ASTNode *import_node = parse_import(scanner);
    program_node->next = import_node;
    load_builtin_functions(&symtable, import_node);

    // Парсим только функции для их декларации
    parse_functions_declaration(scanner, program_node);

    // Продолжаем парсить остальную часть программы

    current_function_copy = *program_node->body;
    current_function_carette = program_node->body;

    while (current_token.type != TOKEN_EOF)
    {
        if ((current_token.type == TOKEN_PUB) || (current_token.type == TOKEN_FN))
        {
            *current_function_carette = *(parse_function(scanner, true));

            current_function_carette->next = current_function_copy.next;
            current_function_carette = current_function_copy.next;
            if (current_function_copy.next != NULL)
                current_function_copy = *current_function_copy.next;
            // Обновляем указатель на текущую функцию
        }
        else
        {
            error_exit(ERR_SYNTAX, "Expected function definition. Line: %d, Column: %d", current_token.line, current_token.column);
        }
    }

    if (!is_symtable_all_used(&symtable))
    {
        error_exit(ERR_SEMANTIC_UNUSED, "Unused variable");
    }

    // Рекурсивно проверяем если все переменные не выходят за зону декларации своих блоков
    scope_check_identifiers_in_tree(program_node);

    return program_node;
}

void scope_check_identifiers_in_tree(ASTNode *root)
{
    if (!root)
    {
        return;
    }

    // Если узел является идентификатором, выполняем проверку области видимости
    if ((root->type == NODE_IDENTIFIER || root->type == NODE_ASSIGNMENT) && strcmp(root->name, "_") != 0)
    {
        // Ищем символ
        Symbol *symbol = symtable_search(&symtable, root->name);
        ASTNode *declaration_node;

        // Если символ является параметром функции
        if (symbol->symbol_type == SYMBOL_PARAMETER)
        {
            // Берем значение узла функции и из симтейбла и рекурсивно ищем наш узел
            Symbol *parent_function = symtable_search(&symtable, symbol->parent_function);
            declaration_node = parent_function->declaration_node;
            bool found = scope_check(declaration_node, root);
            if (!found)
            {
                error_exit(ERR_SEMANTIC_UNDEF, "Variable is not defined in this scope");
            }
        }
        else // если же является обычным идентификатором, то ищем узел декларации переменной
        {    // и с его помощью рекурсивно ищем узел
            declaration_node = symbol->declaration_node;
            bool found = scope_check(declaration_node, root);
            if (!found)
            {
                error_exit(ERR_SEMANTIC_UNDEF, "Variable is not defined in this scope");
            }
        }
    }

    // Рекурсивно обходим дерево
    scope_check_identifiers_in_tree(root->left);
    scope_check_identifiers_in_tree(root->right);
    scope_check_identifiers_in_tree(root->body);
    scope_check_identifiers_in_tree(root->next);
    scope_check_identifiers_in_tree(root->condition);
}

bool scope_check(ASTNode *node_decl, ASTNode *node_identifier)
{
    if (!node_decl || !node_identifier)
    {
        return false;
    }

    // Сравниваем текущий узел с идентификатором
    if (node_decl == node_identifier)
    {
        return true; // Совпадение найдено
    }

    // Рекурсивный вызов для дочерних узлов
    if (scope_check(node_decl->left, node_identifier))
    {
        return true;
    }
    if (scope_check(node_decl->right, node_identifier))
    {
        return true;
    }
    if (scope_check(node_decl->body, node_identifier))
    {
        return true;
    }
    if (scope_check(node_decl->next, node_identifier))
    {
        return true;
    }
    if (scope_check(node_decl->condition, node_identifier))
    {
        return true;
    }

    return false; // Совпадение не найдено
}

void parse_functions_declaration(Scanner *scanner, ASTNode *program_node)
{
    // Сохраняем состояние сканера для дальнейшего возврата после декларации функций
    Scanner saved_scanner_state = *scanner;
    FILE saved_input = *scanner->input;
    Token saved_token = current_token;

    ASTNode *current_function = NULL;
    // Парсим функции
    while (current_token.type != TOKEN_EOF)
    {
        if ((current_token.type == TOKEN_PUB) || (current_token.type == TOKEN_FN))
        {
            ASTNode *function_node = parse_function(scanner, false);
            if (program_node->body == NULL)
            {
                program_node->body = function_node; // Устанавливаем первую функцию
            }
            else
            {
                current_function->next = function_node; // Присоединяем к текущему
            }
            current_function = function_node; // Обновляем указатель на текущую функцию
        }
        else
        {
            error_exit(ERR_SYNTAX, "Expected function definition. Line: %d, Column: %d", current_token.line, current_token.column);
        }
    }

    // Восстанавливаем состояние сканера
    *scanner = saved_scanner_state;
    *scanner->input = saved_input;
    current_token = saved_token;

    return;
}

ASTNode *parse_function(Scanner *scanner, bool is_definition)
{
    LOG("DEBUG_PARSER: Parsing function\n");
    expect_token(TOKEN_PUB, scanner); // Ожидаем ключевое слово 'pub'
    expect_token(TOKEN_FN, scanner);  // Ожидаем ключевое слово 'fn'

    if (current_token.type != TOKEN_IDENTIFIER)
    {
        error_exit(ERR_SYNTAX, "Expected function name.");
    }

    char *function_name = string_duplicate(current_token.lexeme);
    if (function_name == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation failed for function name.");
    }
    current_token = get_next_token(scanner);

    expect_token(TOKEN_LEFT_PAREN, scanner); // '('

    // Парсим параметры функции
    ASTNode **parameters = NULL;
    int param_count = 0;
    // if (!is_definition)
    //{
    if (current_token.type != TOKEN_RIGHT_PAREN)
    {
        parameters = (ASTNode **)malloc(sizeof(ASTNode *));
        parameters[param_count++] = parse_parameter(scanner, function_name, is_definition);
        while (current_token.type == TOKEN_COMMA)
        {
            current_token = get_next_token(scanner);
            parameters = (ASTNode **)realloc(parameters, (param_count + 1) * sizeof(ASTNode *));
            parameters[param_count++] = parse_parameter(scanner, function_name, is_definition);
        }
    }
    //}
    /*else
    {
        while (current_token.type != TOKEN_RIGHT_PAREN)
            current_token = get_next_token(scanner);
    }**/
    expect_token(TOKEN_RIGHT_PAREN, scanner); // ')'

    // Ожидаем возвращаемый тип
    DataType return_type = parse_return_type(scanner);
    ASTNode *function_node = NULL;

    if (is_definition)
    {
        // Создаем узел функции
        ASTNode *body_node = parse_block(scanner, function_name);
        function_node = create_function_node(function_name, return_type, parameters, param_count, body_node);
        // Symbol *symbol = symtable_search(&symtable, function_name);

        int block_layer = 0;
        check_return_types(function_node->body->body, return_type, &block_layer);
    }
    else
    {
        if (current_token.type != TOKEN_LEFT_BRACE)
        {
            error_exit(ERR_SYNTAX, "Expected '{' at the start of function body.");
        }
        int brace_count = 1; // Начинаем с уровня вложенности 1 для первой открывающей скобки

        while (brace_count > 0)
        {
            current_token = get_next_token(scanner);

            if (current_token.type == TOKEN_LEFT_BRACE)
            {
                brace_count++; // Встретили новую `{`, увеличиваем счетчик
            }
            else if (current_token.type == TOKEN_RIGHT_BRACE)
            {
                brace_count--; // Встретили `}`, уменьшаем счетчик
            }
        }
        LOG("!!!DEBUG_PARSER: Brace count passed\n");
        // Добавляем функцию в таблицу символов
        function_node = create_function_node(function_name, return_type, parameters, param_count, NULL);

        char *function_name_symtable = string_duplicate(function_name);
        Symbol *function_symbol = symtable_search(&symtable, function_name_symtable);
        if (function_symbol != NULL)
        {
            error_exit(ERR_SEMANTIC_OTHER, "Function already defined.");
        }

        Symbol *new_function = (Symbol *)malloc(sizeof(Symbol));
        if (new_function == NULL)
        {
            error_exit(ERR_INTERNAL, "Memory allocation failed for new function symbol");
        }

        new_function->name = string_duplicate(function_name_symtable);
        if (new_function->name == NULL)
        {
            free(new_function);
            error_exit(ERR_INTERNAL, "Memory allocation failed for function name");
        }

        new_function->symbol_type = SYMBOL_FUNCTION;
        new_function->parent_function = string_duplicate(function_name);
        new_function->data_type = return_type;
        new_function->is_defined = true;
        new_function->declaration_node = function_node;
        new_function->is_used = strcmp(new_function->name, "main") == 0 ? true : false;
        new_function->next = NULL;

        symtable_insert(&symtable, function_name_symtable, new_function);
        current_token = get_next_token(scanner);
    }

    return function_node;
}

ASTNode *parse_parameter(Scanner *scanner, char *function_name, bool is_definition)
{
    LOG("DEBUG_PARSER: Parsing parameter\n");

    // Ожидаем имя параметра
    if (current_token.type != TOKEN_IDENTIFIER)
    {
        error_exit(ERR_SYNTAX, "Expected parameter name.");
    }

    LOG("DEBUG_PARSER: current_token.lexeme before param: %s\n", current_token.lexeme);
    char *param_name = construct_variable_name(current_token.lexeme, function_name);
    if (param_name == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation failed for parameter name.");
    }

    current_token = get_next_token(scanner);

    // Ожидаем двоеточие ':'
    expect_token(TOKEN_COLON, scanner);

    // Ожидаем тип параметра
    DataType param_type = parse_type(scanner);

    // Проверяем существование параметра в таблице символов
    LOG("DEBUG_PARSER: param_name before symtable_search: %s\n", param_name);
    Symbol *param_symbol = symtable_search(&symtable, param_name);
    if (param_symbol != NULL && is_definition)
    {
        free(param_name);
        error_exit(ERR_SEMANTIC_OTHER, "Parameter already defined.");
    }

    // Создаем новый символ и добавляем его в таблицу символов
    ASTNode *param_node = create_variable_declaration_node(param_name, param_type, NULL);
    if (is_definition)
    {
        Symbol *new_param = (Symbol *)malloc(sizeof(Symbol));
        if (new_param == NULL)
        {
            free(param_name);
            error_exit(ERR_INTERNAL, "Memory allocation failed for parameter symbol.");
        }

        new_param->name = param_name;
        new_param->symbol_type = SYMBOL_PARAMETER;
        new_param->parent_function = string_duplicate(function_name);
        new_param->data_type = param_type;
        new_param->is_defined = true;
        new_param->next = NULL;
        new_param->declaration_node = param_node;

        symtable_insert(&symtable, param_name, new_param);
    }
    // Создаем узел параметра AST

    return param_node;
}

DataType parse_type(Scanner *scanner)
{
    if (current_token.type == TOKEN_I32)
    {
        current_token = get_next_token(scanner); // Обновляем токен после разбора типа
        return TYPE_INT;
    }
    else if (current_token.type == TOKEN_F64)
    {
        current_token = get_next_token(scanner); // Обновляем токен после разбора типа
        return TYPE_FLOAT;
    }
    else if (current_token.type == TOKEN_U8)
    {
        current_token = get_next_token(scanner); // Обновляем токен после разбора типа
        return TYPE_U8;
    }
    else if (current_token.type == TOKEN_VOID)
    {
        current_token = get_next_token(scanner); // Обновляем токен после разбора типа
        return TYPE_VOID;
    }
    else if (current_token.type == TOKEN_ASSIGN)
    {
        current_token = get_next_token(scanner);
        return TYPE_UNKNOWN;
    }
    else if (current_token.type == TOKEN_QUESTION)
    {
        current_token = get_next_token(scanner);
        if (current_token.type == TOKEN_I32)
        {
            current_token = get_next_token(scanner); // Обновляем токен после разбора типа
            return TYPE_INT_NULLABLE;
        }
        else if (current_token.type == TOKEN_F64)
        {
            current_token = get_next_token(scanner); // Обновляем токен после разбора типа
            return TYPE_FLOAT_NULLABLE;
        }
        else if (current_token.type == TOKEN_U8)
        {
            current_token = get_next_token(scanner); // Обновляем токен после разбора типа
            return TYPE_U8_NULLABLE;
        }
    }

    error_exit(ERR_SYNTAX, "Expected type.");
    return TYPE_UNKNOWN;
}
// Function to parse the return type (same as parameter type parsing)
DataType parse_return_type(Scanner *scanner)
{
    if (current_token.type == TOKEN_I32)
    {
        current_token = get_next_token(scanner); // Переходим к следующему токену
        return TYPE_INT;
    }
    else if (current_token.type == TOKEN_F64)
    {
        current_token = get_next_token(scanner);
        return TYPE_FLOAT;
    }
    else if (current_token.type == TOKEN_U8)
    {
        current_token = get_next_token(scanner);
        return TYPE_U8;
    }
    else if (current_token.type == TOKEN_VOID)
    {
        current_token = get_next_token(scanner);
        return TYPE_VOID;
    }
    else if (current_token.type == TOKEN_QUESTION)
    {
        current_token = get_next_token(scanner);
        if (current_token.type == TOKEN_I32)
        {
            current_token = get_next_token(scanner); // Обновляем токен после разбора типа
            return TYPE_INT_NULLABLE;
        }
        else if (current_token.type == TOKEN_F64)
        {
            current_token = get_next_token(scanner); // Обновляем токен после разбора типа
            return TYPE_FLOAT_NULLABLE;
        }
        else if (current_token.type == TOKEN_U8)
        {
            current_token = get_next_token(scanner); // Обновляем токен после разбора типа
            return TYPE_U8_NULLABLE;
        }
    }

    error_exit(ERR_SYNTAX, "Expected return type.");
    return TYPE_UNKNOWN;
}

// Parses a block of statements enclosed in {}
ASTNode *parse_block(Scanner *scanner, char *function_name)
{
    LOG("DEBUG_PARSER: Parsing block\n");
    expect_token(TOKEN_LEFT_BRACE, scanner); // Ожидаем '{'

    // Создаем узел блока
    ASTNode *block_node = create_block_node(NULL, TYPE_NULL);
    ASTNode *current_statement = NULL;

    // Парсим операторы в теле функции до тех пор, пока не встретим '}'
    while (current_token.type != TOKEN_RIGHT_BRACE)
    {
        ASTNode *statement_node = parse_statement(scanner, function_name);

        if (!block_node->body)
        {
            block_node->body = statement_node;
        }
        else
        {
            current_statement->next = statement_node;
        }
        current_statement = statement_node;
    }

    // Ожидаем закрывающую скобку '}'
    expect_token(TOKEN_RIGHT_BRACE, scanner);

    return block_node;
}

ASTNode *parse_statement(Scanner *scanner, char *function_name)
{
    if (current_token.type == TOKEN_VAR || current_token.type == TOKEN_CONST)
    {
        return parse_variable_declaration(scanner, function_name);
    }
    else if (current_token.type == TOKEN_IF)
    {
        return parse_if_statement(scanner, function_name);
    }
    else if (current_token.type == TOKEN_WHILE)
    {
        return parse_while_statement(scanner, function_name);
    }
    else if (current_token.type == TOKEN_RETURN)
    {
        return parse_return_statement(scanner, function_name); // Обрабатываем return и возвращаем узел
    }
    else if (current_token.type == TOKEN_IDENTIFIER)
    {
        // Парсим как выражение присваивания и возвращаем узел
        return parse_variable_assigning(scanner, function_name);
    }
    else
    {
        error_exit(ERR_SYNTAX, "Invalid statement.");
        return NULL; // На случай ошибки, хотя сюда выполнение не дойдет
    }
}

ASTNode *parse_variable_assigning(Scanner *scanner, char *function_name)
{
    LOG("DEBUG_PARSER: Parsing variable assigning\n");
    char *name;
    Symbol *symbol;
    if (strcmp(current_token.lexeme, "ifj") == 0 || strcmp(current_token.lexeme, "_") == 0)
    {
        bool is_builtin = is_builtin_function(current_token.lexeme, scanner);
        if (is_builtin)
        {
            name = construct_builtin_name("ifj", current_token.lexeme);
            symbol = symtable_search(&symtable, name);
            if (symbol == NULL)
            {
                error_exit(ERR_SEMANTIC_UNDEF, "Undefined builtin function");
            }
            char *builtin_function_name = string_duplicate(current_token.lexeme);

            current_token = get_next_token(scanner);
            expect_token(TOKEN_LEFT_PAREN, scanner);
            ASTNode **arguments;
            int arg_count = 0;
            int builtin_index = get_builtin_function_index(builtin_function_name);
            int params_count = builtin_functions[builtin_index].param_count;

            if (current_token.type != TOKEN_RIGHT_PAREN)
            {
                arguments = (ASTNode **)malloc(sizeof(ASTNode *));
                arguments[arg_count++] = parse_expression(scanner, function_name);
                // Проверка если тип данных соответствует
                if ((arguments[arg_count - 1]->data_type != builtin_functions[builtin_index].param_types[arg_count - 1]) && builtin_functions[builtin_index].param_types[arg_count - 1] != TYPE_ALL)
                {
                    error_exit(ERR_SEMANTIC_PARAMS, "Invalid type of arguments");
                }

                while (current_token.type == TOKEN_COMMA)
                {
                    if (arg_count >= params_count)
                    {
                        error_exit(ERR_SEMANTIC_PARAMS, "Invalid number of arguments");
                    }
                    current_token = get_next_token(scanner);
                    arguments = (ASTNode **)realloc(arguments, (arg_count + 1) * sizeof(ASTNode *));
                    arguments[arg_count++] = parse_expression(scanner, function_name);
                    if ((arguments[arg_count - 1]->data_type != builtin_functions[builtin_index].param_types[arg_count - 1]) && builtin_functions[builtin_index].param_types[arg_count - 1] != TYPE_ALL)
                    {
                        error_exit(ERR_SEMANTIC_PARAMS, "Invalid type of arguments");
                    }
                }
            }
            expect_token(TOKEN_RIGHT_PAREN, scanner); // Ожидаем ')'

            // Проверка если количество аргументов соответствует
            if (arg_count != builtin_functions[builtin_index].param_count)
            {
                error_exit(ERR_SEMANTIC_PARAMS, "Invalid number of params");
            }

            // Создаем узел вызова функции
            ASTNode *func_call_node = create_function_call_node(name, arguments, arg_count);
            func_call_node->data_type = builtin_functions[builtin_index].return_type; // Устанавливаем тип данных на основе встроенной функции или символа

            expect_token(TOKEN_SEMICOLON, scanner);
            return func_call_node;
        }
        else
        {
            name = string_duplicate(current_token.lexeme);
            symbol = symtable_search(&symtable, name);

            current_token = get_next_token(scanner);
            // Логика для присваивания
            expect_token(TOKEN_ASSIGN, scanner);

            // Парсим выражение для присваивания
            ASTNode *value_node = parse_expression(scanner, function_name);

            // Если переменная ожидает float, а выражение int, выполняем неявную конверсию
            if (symbol->data_type != TYPE_ALL)
            {
                if (is_nullable(symbol->data_type) && value_node->data_type == TYPE_NULL)
                {
                    // Допускаем присваивание null, если переменная поддерживает nullable-тип.
                }
                else if (!is_subtype_nullable(symbol->data_type, value_node->data_type))
                {
                    error_exit(ERR_SEMANTIC_TYPE, "Cannot assign null to non-nullable variable %s.", name);
                }
            }

            // Проверка и выполнение неявной конверсии
            if (symbol->data_type == TYPE_FLOAT && value_node->data_type == TYPE_INT)
            {
                value_node = convert_to_float_node(value_node);
            }

            // Ожидаем точку с запятой в конце оператора
            // expect_token(TOKEN_SEMICOLON, scanner);

            // Создаем узел присваивания и возвращаем его
            expect_token(TOKEN_SEMICOLON, scanner);
            return create_assignment_node(name, value_node);
        }
    }
    else
    {
        name = construct_variable_name(current_token.lexeme, function_name);
        symbol = symtable_search(&symtable, name);
        if (symbol == NULL)
        {
            error_exit(ERR_SEMANTIC_UNDEF, "Variable or function %s is not defined.", current_token.lexeme);
        }
        if (symbol->is_constant)
        {
            error_exit(ERR_SEMANTIC_OTHER, "Constant variable can't be modified");
        }
        current_token = get_next_token(scanner);

        // Логика для присваивания
        expect_token(TOKEN_ASSIGN, scanner);

        // Парсим выражение для присваивания
        ASTNode *value_node = parse_expression(scanner, function_name);

        // Рекурсивная проверка и приведение типов
        value_node = check_and_convert_expression(value_node, symbol->data_type, name);

        // Ожидаем точку с запятой в конце оператора
        // expect_token(TOKEN_SEMICOLON, scanner);
        expect_token(TOKEN_SEMICOLON, scanner);
        // Создаем узел присваивания и возвращаем его
        return create_assignment_node(name, value_node);
    }
}
ASTNode *check_and_convert_expression(ASTNode *node, DataType expected_type, const char *variable_name)
{
    if (!node)
        return NULL;

    // Если узел — бинарная операция, рекурсивно проверяем подузлы
    if (node->type == NODE_BINARY_OPERATION)
    {
        node->left = check_and_convert_expression(node->left, expected_type, variable_name);
        node->right = check_and_convert_expression(node->right, expected_type, variable_name);

        // Обновляем тип результата операции
        if (node->left->data_type == TYPE_FLOAT || node->right->data_type == TYPE_FLOAT)
        {
            node->data_type = TYPE_FLOAT;
        }
        else if (node->left->data_type == TYPE_INT && node->right->data_type == TYPE_INT)
        {
            node->data_type = TYPE_INT;
        }
        else
        {
            error_exit(ERR_SEMANTIC_TYPE, "Type mismatch in binary operation for variable %s.", variable_name);
        }
    }
    else if (node->type == NODE_LITERAL || node->type == NODE_IDENTIFIER)
    {
        // Проверяем, требуется ли преобразование
        if (node->data_type == TYPE_INT && expected_type == TYPE_FLOAT)
        {
            LOG("DEBUG: Converting int to float for variable %s, name of value node: %s\n", variable_name, node->value);
            node = convert_to_float_node(node);
        }
        else if (node->data_type != expected_type)
        {
            error_exit(ERR_SEMANTIC_TYPE, "1 Type mismatch in assignment to variable %s.", variable_name);
        }
    }
    else
    {
        // Для остальных типов узлов (вызовы функций и т.д.)
        if (node->data_type == TYPE_INT && expected_type == TYPE_FLOAT)
        {
            LOG("DEBUG: Converting int to float for variable %s in function call\n", variable_name);
            node = convert_to_float_node(node);
        }
        else if (node->data_type != expected_type)
        {
            error_exit(ERR_SEMANTIC_TYPE, "2 Type mismatch in assignment to variable %s.", variable_name);
        }
    }

    return node;
}

ASTNode *parse_variable_declaration(Scanner *scanner, char *function_name)
{
    LOG("DEBUG_PARSER: Parsing variable declaration\n");
    TokenType var_type = current_token.type; // 'var' или 'const'
    current_token = get_next_token(scanner);

    if (current_token.type != TOKEN_IDENTIFIER)
    {
        error_exit(ERR_SYNTAX, "Expected variable name.");
    }
    if (current_token.lexeme == NULL)
    {
        error_exit(ERR_INTERNAL, "Lexeme is NULL before strdup.");
    }
    char *variable_name = construct_variable_name(current_token.lexeme, function_name);
    if (variable_name == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation failed for variable name.");
    }
    current_token = get_next_token(scanner);

    DataType declaration_type = TYPE_UNKNOWN;

    // Проверяем наличие типа переменной
    if (current_token.type == TOKEN_COLON)
    {
        current_token = get_next_token(scanner);
        declaration_type = parse_type(scanner);
    }

    expect_token(TOKEN_ASSIGN, scanner); // Ожидаем '='

    // Парсим выражение для присваивания
    ASTNode *initializer_node = parse_expression(scanner, function_name);
    DataType expr_type = initializer_node->data_type;

    // Проверка на совпадение типов
    if (declaration_type != TYPE_UNKNOWN && expr_type != declaration_type && !is_subtype_nullable(declaration_type, expr_type))
    {
        error_exit(ERR_SEMANTIC_TYPE, "Declared type of variable does not match the assigned type. %d %d,\n Line and column: %d %d\n", declaration_type, expr_type, current_token.line, current_token.column);
    }
    else if (declaration_type == TYPE_UNKNOWN)
    {
        declaration_type = expr_type;
    }

    // Семантическая проверка типа
    if (var_type == TOKEN_VAR && expr_type == TYPE_UNKNOWN)
    {
        error_exit(ERR_SEMANTIC_TYPE, "Unknown data type assignment.");
    }

    expect_token(TOKEN_SEMICOLON, scanner); // Ожидаем ';' в конце оператора

    // Проверяем существование переменной в таблице символов
    Symbol *symbol = symtable_search(&symtable, variable_name);
    if (symbol != NULL)
    {
        error_exit(ERR_SEMANTIC_OTHER, "Variable already defined.");
    }

    ASTNode *variable_declaration_node = create_variable_declaration_node(variable_name, declaration_type, initializer_node);
    // Создаем новый символ и добавляем его в таблицу символов
    Symbol *new_var = (Symbol *)malloc(sizeof(Symbol));
    if (new_var == NULL)
    {
        free(variable_name);
        error_exit(ERR_INTERNAL, "Memory allocation failed for parameter symbol.");
    }
    new_var->name = variable_name;
    new_var->symbol_type = SYMBOL_VARIABLE;
    new_var->parent_function = string_duplicate(function_name);
    new_var->data_type = declaration_type;
    new_var->is_defined = true;
    new_var->is_constant = (var_type == TOKEN_CONST) ? true : false;
    new_var->declaration_node = variable_declaration_node;
    new_var->next = NULL;

    symtable_insert(&symtable, variable_name, new_var);

    // Создаем узел объявления переменной и возвращаем его
    return variable_declaration_node;
}

ASTNode *parse_if_statement(Scanner *scanner, char *function_name)
{
    LOG("DEBUG_PARSER: Parsing if statement\n");
    expect_token(TOKEN_IF, scanner);         // 'if'
    expect_token(TOKEN_LEFT_PAREN, scanner); // '('

    // Парсим условие
    ASTNode *condition_node = parse_expression(scanner, function_name);
    ASTNode *variable_declaration_node;

    bool is_pipe = false;

    // Семантическая проверка на тип bool для условия
    if (condition_node->data_type != TYPE_BOOL && !is_nullable(condition_node->data_type))
    {
        error_exit(ERR_SEMANTIC_TYPE, "Condition in if statement must be boolean.");
    }

    expect_token(TOKEN_RIGHT_PAREN, scanner); // ')'

    if (current_token.type == TOKEN_PIPE)
    {
        current_token = get_next_token(scanner);
        if (current_token.type != TOKEN_IDENTIFIER)
        {
            error_exit(ERR_SEMANTIC, "Expected identifier |id|");
        }
        char *variable_name = construct_variable_name(current_token.lexeme, function_name);
        Symbol *symbol = symtable_search(&symtable, current_token.lexeme);
        if (symbol != NULL)
        {
            error_exit(ERR_SEMANTIC_OTHER, "Variable is already defined");
        }
        variable_declaration_node = create_variable_declaration_node(variable_name, detach_nullable(condition_node->data_type), condition_node->parameters); // Unsure about condition_node->parameters
        variable_declaration_node = create_variable_declaration_node(variable_name, detach_nullable(condition_node->data_type), condition_node->parameters); // Unsure about condition_node->parameters
        // Создаем новый символ и добавляем его в таблицу символов
        Symbol *new_var = (Symbol *)malloc(sizeof(Symbol));
        if (new_var == NULL)
        {
            free(variable_name);
            error_exit(ERR_INTERNAL, "Memory allocation failed for parameter symbol.");
        }
        new_var->name = variable_name;
        new_var->symbol_type = SYMBOL_VARIABLE;
        new_var->parent_function = string_duplicate(function_name);
        new_var->data_type = detach_nullable(condition_node->data_type);
        new_var->is_defined = true;
        new_var->is_constant = true;
        new_var->declaration_node = variable_declaration_node;
        new_var->next = NULL;

        symtable_insert(&symtable, variable_name, new_var);

        current_token = get_next_token(scanner);
        expect_token(TOKEN_PIPE, scanner);

        is_pipe = true;
    }

    // Парсим тело 'if'
    ASTNode *true_block = parse_block(scanner, function_name);

    if (is_pipe)
    {
        ASTNode *tmp = true_block->body;
        true_block->body = variable_declaration_node;
        variable_declaration_node->next = tmp;
    }
    // Парсим необязательный блок 'else', если он есть
    ASTNode *false_block = NULL;
    if (current_token.type == TOKEN_ELSE)
    {
        current_token = get_next_token(scanner);
        false_block = parse_block(scanner, function_name);
    }

    if (false_block != NULL)
    {
        if (true_block->data_type != false_block->data_type)
        {
            error_exit(ERR_SEMANTIC_TYPE, "Incompabile type of return expression");
        }
    }
    // Создаем узел 'if' и возвращаем его
    return create_if_node(condition_node, true_block, false_block, variable_declaration_node);
}

// Function to parse a while statement
ASTNode *parse_while_statement(Scanner *scanner, char *function_name)
{
    LOG("DEBUG_PARSER: Parsing while statement\n");
    expect_token(TOKEN_WHILE, scanner);      // 'while'
    expect_token(TOKEN_LEFT_PAREN, scanner); // '('

    // Парсим условие
    ASTNode *condition_node = parse_expression(scanner, function_name);
    ASTNode *variable_declaration_node;
    bool is_pipe = false;
    // Семантическая проверка на тип bool для условия
    if (condition_node->data_type != TYPE_BOOL && !is_nullable(condition_node->data_type))
    {
        error_exit(ERR_SEMANTIC_TYPE, "Condition in while statement must be boolean.");
    }

    expect_token(TOKEN_RIGHT_PAREN, scanner); // ')'

    if (current_token.type == TOKEN_PIPE)
    {
        current_token = get_next_token(scanner);
        if (current_token.type != TOKEN_IDENTIFIER)
        {
            error_exit(ERR_SEMANTIC, "Expected identifier |id|");
        }
        char *variable_name = construct_variable_name(current_token.lexeme, function_name);
        Symbol *symbol = symtable_search(&symtable, current_token.lexeme);
        if (symbol != NULL)
        {
            error_exit(ERR_SEMANTIC_OTHER, "Variable is already defined");
        }
        variable_declaration_node = create_variable_declaration_node(variable_name, detach_nullable(condition_node->data_type), condition_node->parameters); // Unsure about condition_node->parameters
        variable_declaration_node = create_variable_declaration_node(variable_name, detach_nullable(condition_node->data_type), condition_node->parameters); // Unsure about condition_node->parameters
        // Создаем новый символ и добавляем его в таблицу символов
        Symbol *new_var = (Symbol *)malloc(sizeof(Symbol));
        if (new_var == NULL)
        {
            free(variable_name);
            error_exit(ERR_INTERNAL, "Memory allocation failed for parameter symbol.");
        }
        new_var->name = variable_name;
        new_var->symbol_type = SYMBOL_VARIABLE;
        new_var->parent_function = string_duplicate(function_name);
        new_var->data_type = detach_nullable(condition_node->data_type);
        new_var->is_defined = true;
        new_var->is_constant = true;
        new_var->declaration_node = variable_declaration_node;
        new_var->next = NULL;

        symtable_insert(&symtable, variable_name, new_var);

        current_token = get_next_token(scanner);
        expect_token(TOKEN_PIPE, scanner);
        is_pipe = true;
    }

    // Парсим тело 'while'
    ASTNode *body_node = parse_block(scanner, function_name);

    if (is_pipe)
    {
        ASTNode *tmp = body_node->body;
        body_node->body = variable_declaration_node;
        variable_declaration_node->next = tmp;
    }
    // Создаем узел 'while' и возвращаем его
    return create_while_node(condition_node, body_node);
}

ASTNode *parse_return_statement(Scanner *scanner, char *function_name)
{
    LOG("DEBUG_PARSER: Parsing return statement\n");
    expect_token(TOKEN_RETURN, scanner); // Ожидаем ключевое слово 'return'

    ASTNode *return_value_node = NULL;

    // Проверяем, есть ли выражение после 'return'
    if (current_token.type != TOKEN_SEMICOLON)
    {
        return_value_node = parse_expression(scanner, function_name);
    }

    expect_token(TOKEN_SEMICOLON, scanner); // Ожидаем ';' после оператора return
    LOG("DEBUG_PARSER: Finished parsing return statement\n");

    // Создаем узел 'return' и возвращаем его
    return create_return_node(return_value_node);
}

ASTNode *convert_to_float_node(ASTNode *node)
{
    if (node->data_type != TYPE_INT)
    {
        error_exit(ERR_SEMANTIC_TYPE, "Attempted to convert non-integer node to float.");
    }

    // Создаем новую строку с добавлением ".0"
    char *new_value = string_duplicate(node->value);
    if (!new_value)
    {
        error_exit(ERR_INTERNAL, "Failed to allocate memory for float conversion.");
    }
    add_decimal(new_value); // Модифицируем копию строки

    // Создаем новый узел с новой строкой
    ASTNode *conversion_node = create_literal_node(TYPE_FLOAT, new_value);
    free(new_value); // Освобождаем временную строку
    return conversion_node;
}

ASTNode *parse_expression(Scanner *scanner, char *function_name)
{
    LOG("DEBUG_PARSER: Parsing expression. Current token: %d\n. Line and column: %d %d\n", current_token.type, current_token.line, current_token.column);

    // Разбор первой части выражения и создание узла AST
    ASTNode *left_node = parse_primary_expression(scanner, function_name);
    bool is_boolean_expression = false;

    // Проверка наличия бинарного оператора и создание узлов для бинарных операций
    while (current_token.type == TOKEN_PLUS || current_token.type == TOKEN_MINUS ||
           current_token.type == TOKEN_MULTIPLY || current_token.type == TOKEN_DIVIDE ||
           current_token.type == TOKEN_LESS || current_token.type == TOKEN_LESS_EQUAL ||
           current_token.type == TOKEN_GREATER || current_token.type == TOKEN_GREATER_EQUAL ||
           current_token.type == TOKEN_EQUAL || current_token.type == TOKEN_NOT_EQUAL)
    {

        NodeType op_type;

        // Определяем тип узла операции на основе токена
        switch (current_token.type)
        {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_MULTIPLY:
        case TOKEN_DIVIDE:
            op_type = NODE_BINARY_OPERATION;
            break;
        case TOKEN_LESS:
        case TOKEN_LESS_EQUAL:
        case TOKEN_GREATER:
        case TOKEN_GREATER_EQUAL:
        case TOKEN_EQUAL:
        case TOKEN_NOT_EQUAL:
            op_type = NODE_BINARY_OPERATION;
            is_boolean_expression = true;
            break;
        default:
            error_exit(ERR_SYNTAX, "Unknown operator type.");
        }

        const char *operator_name = current_token.lexeme;

        current_token = get_next_token(scanner); // Пропускаем оператор

        ASTNode *right_node = parse_primary_expression(scanner, function_name);

        // Проверка типов и выполнение конверсии
        if (left_node->data_type != right_node->data_type)
        {
            if (left_node->type == NODE_IDENTIFIER && right_node->type == NODE_IDENTIFIER)
            {
                // Оба операнда — переменные, ошибка
                error_exit(ERR_SEMANTIC, "Implicit conversion between variables is not allowed.");
            }

            if (left_node->data_type == TYPE_INT && right_node->data_type == TYPE_FLOAT)
            {
                if (left_node->type == NODE_LITERAL)
                {
                    // Левый операнд — литерал i32, преобразуем в f64
                    left_node = convert_to_float_node(left_node);
                }
                else
                {
                    error_exit(ERR_SEMANTIC, "Implicit conversion requires a literal operand.");
                }
            }
            else if (left_node->data_type == TYPE_FLOAT && right_node->data_type == TYPE_INT)
            {
                if (right_node->type == NODE_LITERAL)
                {
                    // Правый операнд — литерал i32, преобразуем в f64
                    right_node = convert_to_float_node(right_node);
                }
                else
                {
                    error_exit(ERR_SEMANTIC, "Implicit conversion requires a literal operand.");
                }
            }
            else
            {
                error_exit(ERR_SEMANTIC, "Type mismatch in binary operation: %s %s %s.", left_node->value, operator_name, right_node->name);
            }
        }

        /* // Проверка на null для nullable-типов
        if ((left_node->data_type == TYPE_INT_NULLABLE || left_node->data_type == TYPE_FLOAT_NULLABLE) &&
             left_node->value == NULL)
         {
             error_exit(ERR_SEMANTIC, "Nullable variable is null.");
         }
         if ((right_node->data_type == TYPE_INT_NULLABLE || right_node->data_type == TYPE_FLOAT_NULLABLE) &&
             right_node->value == NULL)
         {
             error_exit(ERR_SEMANTIC, "Nullable variable is null.");
         }*/

        // Создаём узел для бинарной операции
        left_node = create_binary_operation_node(operator_name, left_node, right_node);

        // Обновляем тип выражения
        if (is_boolean_expression)
        {
            left_node->data_type = TYPE_BOOL; // Логические операции возвращают bool
        }
        else
        {
            left_node->data_type = left_node->left->data_type; // Обновляем тип данных выражения
        }
    }

    // Возвращаем узел AST итогового выражения
    return left_node;
}

// Parses a primary expression (literal, identifier, or parenthesized expression)
ASTNode *parse_primary_expression(Scanner *scanner, char *function_name)
{
    LOG("DEBUG_PARSER: Parsing primary expression. Current token: %d\n Line and column: %d %d\n", current_token.type, current_token.line, current_token.column);

    if (current_token.type == TOKEN_INT_LITERAL)
    {
        // Создаем узел для целочисленного литерала
        char *value = string_duplicate(current_token.lexeme);
        ASTNode *literal_node = create_literal_node(TYPE_INT, value);
        current_token = get_next_token(scanner);
        return literal_node;
    }
    else if (current_token.type == TOKEN_FLOAT_LITERAL)
    {
        // Создаем узел для литерала с плавающей точкой
        char *value = string_duplicate(current_token.lexeme);
        ASTNode *literal_node = create_literal_node(TYPE_FLOAT, value);
        current_token = get_next_token(scanner);
        return literal_node;
    }
    else if (current_token.type == TOKEN_STRING_LITERAL)
    {
        // Создаем узел для строкового литерала
        char *value = string_duplicate(current_token.lexeme);
        ASTNode *literal_node = create_literal_node(TYPE_U8, value);
        current_token = get_next_token(scanner);
        return literal_node;
    }
    else if (current_token.type == TOKEN_IDENTIFIER)
    {
        // Проверяем, существует ли идентификатор в таблице символов
        DataType data_type;
        char *identifier_name;
        Symbol *symbol;
        bool is_builtin = is_builtin_function(current_token.lexeme, scanner);
        if (is_builtin)
        {
            identifier_name = construct_builtin_name("ifj", current_token.lexeme);
            symbol = symtable_search(&symtable, identifier_name);
            if (symbol == NULL)
            {
                error_exit(ERR_SEMANTIC_UNDEF, "Undefined builtin function");
            }
            char *builtin_function_name = string_duplicate(current_token.lexeme);

            current_token = get_next_token(scanner);
            expect_token(TOKEN_LEFT_PAREN, scanner);
            ASTNode **arguments = NULL;
            int arg_count = 0;
            int builtin_index = get_builtin_function_index(builtin_function_name);
            int params_count = builtin_functions[builtin_index].param_count;

            if (current_token.type != TOKEN_RIGHT_PAREN)
            {
                arguments = (ASTNode **)malloc(sizeof(ASTNode *));
                arguments[arg_count++] = parse_expression(scanner, function_name);
                // Проверка если тип данных соответствует
                if ((arguments[arg_count - 1]->data_type != builtin_functions[builtin_index].param_types[arg_count - 1]) && builtin_functions[builtin_index].param_types[arg_count - 1] != TYPE_ALL)
                {
                    error_exit(ERR_SEMANTIC_PARAMS, "Invalid type of arguments");
                }

                while (current_token.type == TOKEN_COMMA)
                {
                    if (arg_count >= params_count)
                    {
                        error_exit(ERR_SEMANTIC_PARAMS, "Invalid number of arguments");
                    }
                    current_token = get_next_token(scanner);
                    arguments = (ASTNode **)realloc(arguments, (arg_count + 1) * sizeof(ASTNode *));
                    arguments[arg_count++] = parse_expression(scanner, function_name);
                    if ((arguments[arg_count - 1]->data_type != builtin_functions[builtin_index].param_types[arg_count - 1]) && builtin_functions[builtin_index].param_types[arg_count - 1] != TYPE_ALL)
                    {
                        error_exit(ERR_SEMANTIC_PARAMS, "Invalid type of arguments");
                    }
                }
            }
            expect_token(TOKEN_RIGHT_PAREN, scanner); // Ожидаем ')'

            // Проверка если количество аргументов соответствует
            if (arg_count != builtin_functions[builtin_index].param_count)
            {
                error_exit(ERR_SEMANTIC_PARAMS, "Invalid number of params");
            }

            // Создаем узел вызова функции
            ASTNode *func_call_node = create_function_call_node(identifier_name, arguments, arg_count);
            func_call_node->data_type = builtin_functions[builtin_index].return_type; // Устанавливаем тип данных на основе встроенной функции или символа
            // expect_token(TOKEN_SEMICOLON, scanner);
            return func_call_node;
        }
        else
        {
            // Проверяем елси найденый символ является функцией
            symbol = symtable_search(&symtable, current_token.lexeme);

            if (symbol != NULL && symbol->symbol_type == SYMBOL_FUNCTION)
            {
                identifier_name = current_token.lexeme;
                current_token = get_next_token(scanner);
                // Если это вызов функции

                expect_token(TOKEN_LEFT_PAREN, scanner); // Пропускаем '('

                // Получаем значение списка параметров через найденый символ, в котором есть ссылка на инициализируемый узел функции в котором есть ссылка на поле параметров
                ASTNode **arguments = symbol->declaration_node->parameters;
                int arg_count = 0;

                if (current_token.type != TOKEN_RIGHT_PAREN)
                {
                    arguments = (ASTNode **)malloc(sizeof(ASTNode *));
                    arguments[arg_count++] = parse_expression(scanner, function_name);
                    // Проверка если тип данных соответствует
                    if (arguments[arg_count - 1]->data_type != symbol->declaration_node->parameters[arg_count - 1]->data_type)
                    {
                        error_exit(ERR_SEMANTIC_PARAMS, "Invalid type of arguments");
                    }

                    while (current_token.type == TOKEN_COMMA)
                    {
                        current_token = get_next_token(scanner);
                        arguments = (ASTNode **)realloc(arguments, (arg_count + 1) * sizeof(ASTNode *));
                        arguments[arg_count++] = parse_expression(scanner, function_name);
                        if (arguments[arg_count - 1]->data_type != symbol->declaration_node->parameters[arg_count - 1]->data_type)
                        {
                            error_exit(ERR_SEMANTIC_PARAMS, "Invalid type of arguments");
                        }
                    }
                }
                expect_token(TOKEN_RIGHT_PAREN, scanner); // Ожидаем ')'

                // Проверка если количество аргументов соответствует
                if (arg_count != symbol->declaration_node->param_count)
                {
                    error_exit(ERR_SEMANTIC_PARAMS, "Invalid number of params");
                }

                // Создаем узел вызова функции
                ASTNode *func_call_node = create_function_call_node(identifier_name, arguments, arg_count);
                func_call_node->data_type = symbol->data_type; // Устанавливаем тип данных на основе встроенной функции или символа
                // expect_token(TOKEN_SEMICOLON, scanner);
                return func_call_node;
            }
            else // если не является, то повторяем поиск, с расчетом на то что это переменная
            {
                identifier_name = construct_variable_name(current_token.lexeme, function_name);
                symbol = symtable_search(&symtable, identifier_name);
                if (symbol == NULL)
                {
                    error_exit(ERR_SEMANTIC_UNDEF, "Undefined variable or function. Got lexeme: %s", current_token.lexeme);
                }
                ASTNode *identifier_node = create_identifier_node(identifier_name);
                identifier_node->data_type = symbol->data_type; // Устанавливаем тип данных для идентификатора

                // expect_token(TOKEN_SEMICOLON, scanner);
                current_token = get_next_token(scanner);
                return identifier_node;
            }
            LOG("DEBUG_PARSER: BEFORE Primary parsing got token type: %d\n", current_token.type);
            current_token = get_next_token(scanner);
            LOG("DEBUG_PARSER: Primary parsing got token type: %d\n", current_token.type);

            // Создаем узел идентификатора
        }
    }
    else if (current_token.type == TOKEN_LEFT_PAREN)
    {
        // Скобочное выражение
        current_token = get_next_token(scanner);                       // Пропускаем '('
        ASTNode *expr_node = parse_expression(scanner, function_name); // Парсим выражение в скобках
        expect_token(TOKEN_RIGHT_PAREN, scanner);                      // Ожидаем ')'
        return expr_node;
    }
    else if (current_token.type == TOKEN_NULL)
    {
        // Создаем узел для целочисленного литерала
        char *value = string_duplicate(current_token.lexeme);
        ASTNode *literal_node = create_literal_node(TYPE_NULL, value);
        current_token = get_next_token(scanner);
        return literal_node;
    }
    else
    {
        error_exit(ERR_SYNTAX, "Expected literal, identifier, or '(' for expression.");
        return NULL; // На случай ошибки, хотя сюда выполнение не дойдет
    }
}

// Function to parse the import line at the beginning of the program
ASTNode *parse_import(Scanner *scanner)
{
    LOG("DEBUG_PARSER: Parsing import statement\n");

    // Ожидаем ключевое слово 'const'
    expect_token(TOKEN_CONST, scanner);

    // Ожидаем идентификатор 'ifj'
    if (current_token.type != TOKEN_IDENTIFIER || strcmp(current_token.lexeme, "ifj") != 0)
    {
        error_exit(ERR_SYNTAX, "Expected identifier 'ifj'.");
    }
    current_token = get_next_token(scanner);

    // Ожидаем '='
    expect_token(TOKEN_ASSIGN, scanner);

    // Ожидаем '@import'
    if (current_token.type != TOKEN_IMPORT)
    {
        error_exit(ERR_SYNTAX, "Expected '@import'.");
    }
    current_token = get_next_token(scanner);

    // Ожидаем '('
    expect_token(TOKEN_LEFT_PAREN, scanner);

    // Ожидаем строковый литерал "ifj24.zig"
    if (current_token.type != TOKEN_STRING_LITERAL || strcmp(current_token.lexeme, "ifj24.zig") != 0)
    {
        error_exit(ERR_SYNTAX, "Expected string literal \"ifj24.zig\". Got: %s", current_token.lexeme);
    }
    char *import_value = string_duplicate(current_token.lexeme);
    current_token = get_next_token(scanner);

    // Ожидаем ')'
    expect_token(TOKEN_RIGHT_PAREN, scanner);

    // Ожидаем ';'
    expect_token(TOKEN_SEMICOLON, scanner);

    LOG("DEBUG_PARSER: Finished parsing import statement\n");

    // Создаем узел AST для оператора импорта
    return create_literal_node(TYPE_U8, import_value);
}

/*
A function that recursively traverses an entire AST and:
1. In case of void function looks for any return statement and checks if its type is different from void
2. In case of a function of a type other than void, it checks if all return statements match the given type of the function,
 and looks for a return statement in the main block of the function.
*/
void check_return_types(ASTNode *function_node, DataType return_type, int *block_layer)
{
    if (return_type == TYPE_VOID)
    {
        if (function_node != NULL)
        {
            if (function_node->type == NODE_RETURN)
            {
                if (function_node->data_type != TYPE_VOID)
                {
                    error_exit(ERR_SEMANTIC_RETURN, "Function VOID expects return(void)");
                }
            }
        }
        if (function_node->body != NULL)
        {
            check_return_types(function_node->body, return_type, block_layer);
        }
        if (function_node->left != NULL)
        {
            check_return_types(function_node->left, return_type, block_layer);
        }
        if (function_node->next != NULL)
        {
            check_return_types(function_node->next, return_type, block_layer);
        }
        return;
    }
    else
    {
        if (function_node == NULL && *block_layer == 0)
        {
            error_exit(ERR_SEMANTIC_RETURN, "Missing return statement");
        }
        if (function_node->type == NODE_RETURN)
        {
            if (function_node->data_type != return_type && !is_subtype_nullable(return_type, function_node->data_type))
            {
                error_exit(ERR_SEMANTIC_PARAMS, "Incompatible types of return statement");
            }
            if (*block_layer == 0) // Здесь проверяем значение на 0
            {
                (*block_layer)--; // Корректно уменьшаем значение
            }
            return;
        }
        if (function_node->body != NULL)
        {
            (*block_layer)++;
            check_return_types(function_node->body, return_type, block_layer);
            (*block_layer)--;
        }
        if (function_node->left != NULL)
        {
            (*block_layer)++;
            check_return_types(function_node->left, return_type, block_layer);
            (*block_layer)--;
        }
        if (function_node->next != NULL)
        {
            check_return_types(function_node->next, return_type, block_layer);
        }
        if (*block_layer == 0) // Здесь используем разыменование указателя
        {
            error_exit(ERR_SEMANTIC_RETURN, "Missing return statement");
        }
    }
}

bool type_convertion(ASTNode *main_node)
{
    if (main_node->left->data_type == TYPE_FLOAT && main_node->right->data_type == TYPE_INT && main_node->right->type == NODE_LITERAL)
    {
        add_decimal(main_node->right->value);
        main_node->right->data_type = TYPE_FLOAT;
        main_node->data_type = TYPE_FLOAT;
        return true;
    }
    else if (main_node->left->data_type == TYPE_INT && main_node->right->data_type == TYPE_FLOAT && main_node->left->type == NODE_LITERAL)
    {
        add_decimal(main_node->left->value);
        main_node->left->data_type = TYPE_FLOAT;
        main_node->data_type = TYPE_FLOAT;
        return true;
    }

    return false;
    /*
    else if(ma)
    remove_decimal(main_node->right->value);
    remove_decimal(main_node->left->value);*/
}

bool is_subtype_nullable(DataType type_nullable, DataType type)
{
    return ((type_nullable == TYPE_INT_NULLABLE && (type == TYPE_INT || type == TYPE_NULL)) ||
            (type_nullable == TYPE_FLOAT_NULLABLE && (type == TYPE_FLOAT || type == TYPE_NULL)) ||
            (type_nullable == TYPE_U8_NULLABLE && (type == TYPE_U8 || type == TYPE_NULL)));
}

bool is_nullable(DataType type_nullable)
{
    return (type_nullable == TYPE_INT_NULLABLE || type_nullable == TYPE_FLOAT_NULLABLE || type_nullable == TYPE_U8_NULLABLE);
}

DataType detach_nullable(DataType type_nullable)
{
    if (type_nullable == TYPE_INT_NULLABLE)
        type_nullable = TYPE_INT;
    else if (type_nullable == TYPE_FLOAT_NULLABLE)
        type_nullable = TYPE_FLOAT;
    else if (type_nullable == TYPE_U8_NULLABLE)
        type_nullable = TYPE_U8;

    return type_nullable;
}

// Function to expect a specific token type
static void expect_token(TokenType expected_type, Scanner *scanner)
{
    LOG("DEBUG_PARSER: Expected token: %d, got token: %d\nLine and column: %d %d\n", expected_type, current_token.type, current_token.line, current_token.column);
    if (current_token.type != expected_type)
    {
        error_exit(ERR_SYNTAX, "Unexpected token. Expected: %d, got: %d\nLine and column: %d %d\n", expected_type, current_token.type, current_token.line, current_token.column);
    }
    current_token = get_next_token(scanner);
}