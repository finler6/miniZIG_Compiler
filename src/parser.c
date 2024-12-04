/**
 * @file parser.c
 *
 * Implementation of the parser module.
 * The parser is responsible for parsing the input source code.
 *
 * IFJ Project 2024, Team 'xstepa77'
 *
 * @author <xlitvi02> Gleb Litvinchuk
 * @author <xstepa77> Pavel Stepanov
 * @author <xkovin00> Viktoriia Kovina
 * @author <xshmon00> Gleb Shmonin
 */
#include "parser.h"

#undef DEBUG_PARSER
#ifdef DEBUG_PARSER
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

// Forward declarations of helper functions

// Function to make sure that current token us expected_type
static void expect_token(TokenType expected_type, Scanner *scanner);

// Main functions for parser
static ASTNode *parse_import(Scanner *scanner);
static ASTNode *parse_function(Scanner *scanner, bool is_definition);
static ASTNode *parse_parameter(Scanner *scanner, char *function_name, bool is_definition);
static ASTNode *parse_block(Scanner *scanner, char *function_name);
static ASTNode *parse_statement(Scanner *scanner, char *function_name);
static ASTNode *parse_variable_declaration(Scanner *scanner, char *function_name);
static ASTNode *parse_variable_assigning(Scanner *scanner, char *function_name);
static ASTNode *parse_if_statement(Scanner *scanner, char *function_name);
static ASTNode *parse_while_statement(Scanner *scanner, char *function_name);
static ASTNode *parse_return_statement(Scanner *scanner, char *function_name);
static ASTNode *parse_expression(Scanner *scanner, char *function_name);
static ASTNode *parse_primary_expression(Scanner *scanner, char *function_name);
static ASTNode *parse_builtin_fucntion_call(Scanner *scanner, Symbol *symbol, char *identifier_name, char *function_name);
static ASTNode *parse_function_call(Scanner *scanner, Symbol *symbol, char *identifier_name, char *function_name);
static ASTNode *parse_idendifier(Scanner *scanner, Symbol *symbol, char *identifier_name, char *function_name);
static ASTNode *check_and_convert_expression(ASTNode *node, DataType expected_type, const char *variable_name);
static ASTNode **parse_arguments(Scanner *scanner, Symbol *symbol, ASTNode **arguments, int param_count, int *arg_count, char *function_name, char *builtin_function_name);

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

bool check_arguments_compability(Symbol *symbol, ASTNode **arguments, int *arg_count, char *builtin_function_name);

void parse_functions_declaration(Scanner *scanner, ASTNode *program_node);
bool type_convertion(ASTNode *main_node);
bool can_assign_type(DataType expected_type, DataType actual_type);
DataType detach_nullable(DataType type_nullable);
int get_builtin_function_index(const char *function_name);

// Global token storage
static Token current_token;

// Global symbol table for the program
static SymTable symtable;

BuiltinFunctionInfo builtin_functions[] = {
    {"readstr", TYPE_U8_NULLABLE, {TYPE_NULL}, 0},
    {"readi32", TYPE_INT_NULLABLE, {TYPE_NULL}, 0},
    {"readf64", TYPE_FLOAT_NULLABLE, {TYPE_NULL}, 0},
    {"write", TYPE_VOID, {TYPE_ALL}, 1},
    {"i2f", TYPE_FLOAT, {TYPE_INT}, 1},
    {"f2i", TYPE_INT, {TYPE_FLOAT}, 1},
    {"string", TYPE_U8, {TYPE_U8}, 1},
    {"length", TYPE_INT, {TYPE_U8}, 1},
    {"concat", TYPE_U8, {TYPE_U8, TYPE_U8}, 2},
    {"substring", TYPE_U8_NULLABLE, {TYPE_U8, TYPE_INT, TYPE_INT}, 3},
    {"strcmp", TYPE_INT, {TYPE_U8, TYPE_U8}, 2},
    {"ord", TYPE_INT, {TYPE_U8, TYPE_INT}, 2},
    {"chr", TYPE_U8, {TYPE_INT}, 1}};

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

    ASTNode *program_node = create_program_node();

    // These nodes were defined to make pre-run to declare functions and its parameters
    ASTNode program_node_pointer;
    ASTNode *current_function_pointer = NULL;

    ASTNode *import_node = parse_import(scanner);
    program_node->next = import_node;

    load_builtin_functions(&symtable, import_node);

    // Pre-run
    parse_functions_declaration(scanner, program_node);

    /* At this point program currently have functions ASTNode created
    now program need to prevent rewriting this nodes and continue.
    So after Pre-run AST has all function nodes, but no function node has deeper nodes in tree
    Later this tree will be called pre-run tree
    */
    program_node_pointer = *program_node->body;
    current_function_pointer = program_node->body;

    while (current_token.type != TOKEN_EOF)
    {
        if ((current_token.type == TOKEN_PUB) || (current_token.type == TOKEN_FN))
        {
            // Creating function node again with all deeper nodes
            *current_function_pointer = *(parse_function(scanner, true));

            // Pointing to next funtion node according with pre-run
            current_function_pointer->next = program_node_pointer.next;

            // Moving to the next function node from pre-run tree
            current_function_pointer = program_node_pointer.next;

            // Moving pre-run tree pointer to the next function node if exists
            if (program_node_pointer.next != NULL)
                program_node_pointer = *program_node_pointer.next;
        }
        else
        {
            error_exit(ERR_SYNTAX, "Expected function definition. Line: %d, Column: %d", current_token.line, current_token.column);
        }
    }

    // Semantics check

    is_main_correct(&symtable);
    is_symtable_all_used(&symtable);
    scope_check_identifiers_in_tree(program_node);

    return program_node;
}

ASTNode *parse_function(Scanner *scanner, bool is_definition)
{
    LOG("DEBUG_PARSER: Parsing function\n");
    expect_token(TOKEN_PUB, scanner);
    expect_token(TOKEN_FN, scanner);

    if (current_token.type != TOKEN_IDENTIFIER)
    {
        error_exit(ERR_SYNTAX, "Expected function name.");
    }

    char *function_name = string_duplicate(current_token.lexeme);
    current_token = get_next_token(scanner);

    expect_token(TOKEN_LEFT_PAREN, scanner); // '('
    ASTNode **parameters = NULL;
    int param_count = 0;

    if (current_token.type != TOKEN_RIGHT_PAREN)
    {
        parameters = (ASTNode **)safe_malloc(sizeof(ASTNode *));
        parameters[param_count++] = parse_parameter(scanner, function_name, is_definition);
        while (current_token.type == TOKEN_COMMA)
        {
            current_token = get_next_token(scanner);
            parameters = (ASTNode **)safe_realloc(parameters, (param_count + 1) * sizeof(ASTNode *));
            parameters[param_count++] = parse_parameter(scanner, function_name, is_definition);
        }
    }

    expect_token(TOKEN_RIGHT_PAREN, scanner); // ')'

    DataType return_type = parse_return_type(scanner);
    ASTNode *function_node = NULL;

    if (is_definition)
    {
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
        int brace_count = 1;

        while (brace_count > 0)
        {
            current_token = get_next_token(scanner);

            if (current_token.type == TOKEN_LEFT_BRACE)
            {
                brace_count++;
            }
            else if (current_token.type == TOKEN_RIGHT_BRACE)
            {
                brace_count--;
            }
        }
        LOG("!!!DEBUG_PARSER: Brace count passed\n");
        function_node = create_function_node(function_name, return_type, parameters, param_count, NULL);

        char *function_name_symtable = string_duplicate(function_name);
        Symbol *function_symbol = symtable_search(&symtable, function_name_symtable);
        if (function_symbol != NULL)
        {
            error_exit(ERR_SEMANTIC_OTHER, "Function already defined.");
        }

        Symbol *new_function = (Symbol *)safe_malloc(sizeof(Symbol));
        new_function->name = string_duplicate(function_name_symtable);
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

    if (current_token.type != TOKEN_IDENTIFIER)
    {
        error_exit(ERR_SYNTAX, "Expected parameter name.");
    }

    LOG("DEBUG_PARSER: current_token.lexeme before param: %s\n", current_token.lexeme);
    char *param_name = construct_variable_name(current_token.lexeme, function_name);

    current_token = get_next_token(scanner);

    expect_token(TOKEN_COLON, scanner);

    DataType param_type = parse_type(scanner);

    LOG("DEBUG_PARSER: param_name before symtable_search: %s\n", param_name);
    Symbol *param_symbol = symtable_search(&symtable, param_name);
    if (param_symbol != NULL && is_definition)
    {
        safe_free(param_name);
        error_exit(ERR_SEMANTIC_OTHER, "Parameter already defined.");
    }

    ASTNode *param_node = create_variable_declaration_node(param_name, param_type, NULL);
    if (is_definition)
    {
        Symbol *new_param = (Symbol *)safe_malloc(sizeof(Symbol));
        new_param->name = param_name;
        new_param->symbol_type = SYMBOL_PARAMETER;
        new_param->parent_function = string_duplicate(function_name);
        new_param->data_type = param_type;
        new_param->is_defined = true;
        new_param->next = NULL;
        new_param->declaration_node = param_node;

        symtable_insert(&symtable, param_name, new_param);
    }

    return param_node;
}

ASTNode *parse_block(Scanner *scanner, char *function_name)
{
    LOG("DEBUG_PARSER: Parsing block\n");
    expect_token(TOKEN_LEFT_BRACE, scanner);

    ASTNode *block_node = create_block_node(NULL, TYPE_NULL);
    ASTNode *current_statement = NULL;

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
        return parse_return_statement(scanner, function_name);
    }
    else if (current_token.type == TOKEN_IDENTIFIER)
    {
        return parse_variable_assigning(scanner, function_name);
    }
    else
    {
        error_exit(ERR_SYNTAX, "Invalid statement.");
        return NULL;
    }
}

ASTNode *parse_variable_assigning(Scanner *scanner, char *function_name)
{
    LOG("DEBUG_PARSER: Parsing variable assigning\n");
    char *name = NULL;
    Symbol *symbol = NULL;
    ASTNode *function_node;
    bool is_builtin = is_builtin_function(current_token.lexeme, scanner);
    bool is_function = false;
    bool is_underscore = false;
    symbol = symtable_search(&symtable, current_token.lexeme);
    if (symbol != NULL)
    {
        if (symbol->symbol_type == SYMBOL_FUNCTION)
        {
            is_function = true;
        }
        else if (strcmp(symbol->name, "_") == 0)
        {
            is_underscore = true;
        }
    }
    if (is_builtin)
    {
        function_node = parse_builtin_fucntion_call(scanner, symbol, name, function_name);
        expect_token(TOKEN_SEMICOLON, scanner);
        return function_node;
        // If is function
    }
    else if (is_function)
    {
        char *function_call_name = string_duplicate(current_token.lexeme);
        symbol = symtable_search(&symtable, function_call_name);
        if (symbol == NULL || symbol->symbol_type != SYMBOL_FUNCTION)
        {
            error_exit(ERR_SEMANTIC_UNDEF, "Undefined function %s.", function_call_name);
        }

        ASTNode *func_call_node = parse_function_call(scanner, symbol, function_call_name, function_name);
        func_call_node->data_type = symbol->data_type;

        if (func_call_node->data_type != TYPE_VOID)
        {
            error_exit(ERR_SEMANTIC_PARAMS, "Cannot ignore return value of function %s.", function_call_name);
        }
        expect_token(TOKEN_SEMICOLON, scanner);

        return func_call_node;
    }
    else if (is_underscore)
    {
        name = string_duplicate(current_token.lexeme);
        symbol = symtable_search(&symtable, name);

        current_token = get_next_token(scanner);

        expect_token(TOKEN_ASSIGN, scanner);

        ASTNode *value_node = parse_expression(scanner, function_name);

        if (symbol->data_type != TYPE_ALL)
        {
            if (is_nullable(symbol->data_type) && value_node->data_type == TYPE_NULL)
            {
            }
            else if (!can_assign_type(symbol->data_type, value_node->data_type))
            {
                error_exit(ERR_SEMANTIC_TYPE, "Cannot assign null to non-nullable variable %s.", name);
            }
        }

        if (symbol->data_type == TYPE_FLOAT && value_node->data_type == TYPE_INT)
        {
            value_node = convert_to_float_node(value_node);
        }

        expect_token(TOKEN_SEMICOLON, scanner);
        return create_assignment_node(name, value_node);
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

        expect_token(TOKEN_ASSIGN, scanner);

        ASTNode *value_node = parse_expression(scanner, function_name);

        if (value_node->type == NODE_LITERAL && value_node->data_type == TYPE_U8)
        {
            error_exit(ERR_SEMANTIC_TYPE, "Cannot assign STRING LITERAL to a variable without using a function \"ifj.write(\"\")\"");
        }

        if (!can_assign_type(symbol->data_type, value_node->data_type))
            value_node = check_and_convert_expression(value_node, symbol->data_type, name);

        expect_token(TOKEN_SEMICOLON, scanner);

        return create_assignment_node(name, value_node);
    }
}

ASTNode *check_and_convert_expression(ASTNode *node, DataType expected_type, const char *variable_name)
{
    if (!node)
        return NULL;

    if (!can_assign_type(expected_type, node->data_type))
    {
        error_exit(ERR_SEMANTIC_TYPE, "Type mismatch in assignment to variable %s.", variable_name);
    }
    if (expected_type == TYPE_FLOAT && node->data_type == TYPE_INT)
    {
        node = convert_to_float_node(node);
    }

    return node;
}

ASTNode *parse_variable_declaration(Scanner *scanner, char *function_name)
{
    LOG("DEBUG_PARSER: Parsing variable declaration\n");
    TokenType var_type = current_token.type;
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
    current_token = get_next_token(scanner);

    DataType declaration_type = TYPE_UNKNOWN;

    if (current_token.type == TOKEN_COLON)
    {
        current_token = get_next_token(scanner);
        declaration_type = parse_type(scanner);
    }

    expect_token(TOKEN_ASSIGN, scanner);

    ASTNode *initializer_node = parse_expression(scanner, function_name);
    DataType expr_type = initializer_node->data_type;

    if (declaration_type != TYPE_UNKNOWN && expr_type != declaration_type && !can_assign_type(declaration_type, expr_type))
    {
        error_exit(ERR_SEMANTIC_TYPE, "Declared type of variable does not match the assigned type. %d %d,\n Line and column: %d %d\n", declaration_type, expr_type, current_token.line, current_token.column);
    }
    else if (declaration_type == TYPE_UNKNOWN && (expr_type == TYPE_UNKNOWN || expr_type == TYPE_NULL))
    {
        error_exit(ERR_SEMANTIC_INFER, "Cannot resolve data type assigning");
    }
    else if (initializer_node->type == NODE_LITERAL && expr_type == TYPE_U8)
    {
        error_exit(ERR_SEMANTIC_INFER, "Cannot assign STRING LITERAL without function \"ifj.write(\"\")\"");
    }
    else if (declaration_type == TYPE_UNKNOWN)
    {
        declaration_type = expr_type;
    }

    if (var_type == TOKEN_VAR && expr_type == TYPE_UNKNOWN)
    {
        error_exit(ERR_SEMANTIC_TYPE, "Unknown data type assignment.");
    }

    expect_token(TOKEN_SEMICOLON, scanner);

    Symbol *symbol = symtable_search(&symtable, variable_name);
    if (symbol != NULL)
    {
        error_exit(ERR_SEMANTIC_OTHER, "Variable already defined.");
    }

    ASTNode *variable_declaration_node = create_variable_declaration_node(variable_name, declaration_type, initializer_node);
    Symbol *new_var = (Symbol *)safe_malloc(sizeof(Symbol));
    new_var->name = variable_name;
    new_var->symbol_type = SYMBOL_VARIABLE;
    new_var->parent_function = string_duplicate(function_name);
    new_var->data_type = declaration_type;
    new_var->is_defined = true;
    new_var->is_constant = (var_type == TOKEN_CONST) ? true : false;
    new_var->declaration_node = variable_declaration_node;
    new_var->next = NULL;

    symtable_insert(&symtable, variable_name, new_var);

    return variable_declaration_node;
}

ASTNode *parse_if_statement(Scanner *scanner, char *function_name)
{
    LOG("DEBUG_PARSER: Parsing if statement\n");
    expect_token(TOKEN_IF, scanner);         // 'if'
    expect_token(TOKEN_LEFT_PAREN, scanner); // '('

    ASTNode *condition_node = parse_expression(scanner, function_name);
    ASTNode *variable_declaration_node;

    bool is_pipe = false;

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
        variable_declaration_node = create_variable_declaration_node(variable_name, detach_nullable(condition_node->data_type), (ASTNode *)condition_node->parameters); // Unsure about condition_node->parameters
        // variable_declaration_node = create_variable_declaration_node(variable_name, detach_nullable(condition_node->data_type), condition_node->parameters); // Unsure about condition_node->parameters
        Symbol *new_var = (Symbol *)safe_malloc(sizeof(Symbol));
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
    ASTNode *true_block = parse_block(scanner, function_name);
    if (is_pipe)
    {
        ASTNode *tmp = true_block->body;
        true_block->body = variable_declaration_node;
        variable_declaration_node->next = tmp;
        variable_declaration_node->left = create_identifier_node(condition_node->name);
    }
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
    return create_if_node(condition_node, true_block, false_block, variable_declaration_node);
}

// Function to parse a while statement
ASTNode *parse_while_statement(Scanner *scanner, char *function_name)
{
    LOG("DEBUG_PARSER: Parsing while statement\n");
    expect_token(TOKEN_WHILE, scanner);      // 'while'
    expect_token(TOKEN_LEFT_PAREN, scanner); // '('
    ASTNode *condition_node = parse_expression(scanner, function_name);
    ASTNode *variable_declaration_node;
    bool is_pipe = false;
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
        variable_declaration_node = create_variable_declaration_node(variable_name, detach_nullable(condition_node->data_type), (ASTNode *)condition_node->parameters); // Unsure about condition_node->parameters
        // variable_declaration_node = create_variable_declaration_node(variable_name, detach_nullable(condition_node->data_type), condition_node->parameters); // Unsure about condition_node->parameters
        Symbol *new_var = (Symbol *)safe_malloc(sizeof(Symbol));
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
    ASTNode *body_node = parse_block(scanner, function_name);
    if (is_pipe)
    {
        ASTNode *tmp = body_node->body;
        body_node->body = variable_declaration_node;
        variable_declaration_node->next = tmp;
        variable_declaration_node->left = create_identifier_node(condition_node->name);
    }
    return create_while_node(condition_node, body_node);
}

ASTNode *parse_return_statement(Scanner *scanner, char *function_name)
{
    LOG("DEBUG_PARSER: Parsing return statement\n");
    expect_token(TOKEN_RETURN, scanner);

    ASTNode *return_value_node = NULL;

    if (current_token.type != TOKEN_SEMICOLON)
    {
        return_value_node = parse_expression(scanner, function_name);
    }

    expect_token(TOKEN_SEMICOLON, scanner);
    LOG("DEBUG_PARSER: Finished parsing return statement\n");
    return create_return_node(return_value_node);
}

ASTNode *convert_to_float_node(ASTNode *node)
{
    if (node->data_type != TYPE_INT)
    {
        error_exit(ERR_SEMANTIC_TYPE, "Attempted to convert non-integer node to float.");
    }
    char *new_value = string_duplicate(node->value);
    add_decimal(new_value);

    ASTNode *conversion_node = create_literal_node(TYPE_FLOAT, new_value);
    safe_free(new_value);
    return conversion_node;
}

ASTNode *perform_type_checking_and_create_node(const char *operator_name, ASTNode *left_node, ASTNode *right_node)
{
    DataType left_type = left_node->data_type;
    DataType right_type = right_node->data_type;

    // Determine if the operation is a boolean operation
    bool is_boolean = false;
    bool is_equality = false;
    if (strcmp(operator_name, ">=") == 0 || strcmp(operator_name, "<=") == 0 ||
        strcmp(operator_name, "<") == 0 || strcmp(operator_name, ">") == 0 ||
        strcmp(operator_name, "==") == 0 || strcmp(operator_name, "!=") == 0)
    {
        is_boolean = true;
        if (strcmp(operator_name, "==") == 0 || strcmp(operator_name, "!=") == 0)
        {
            is_equality = true;
        }
    }

    // If either operand is of type U8 or U8 nullable, error
    if (left_type == TYPE_U8 || left_type == TYPE_U8_NULLABLE ||
        right_type == TYPE_U8 || right_type == TYPE_U8_NULLABLE)
    {
        error_exit(ERR_SEMANTIC_TYPE, "Invalid operand types for operator '%s'", operator_name);
    }

    DataType left_base_type = is_nullable(left_type) ? detach_nullable(left_type) : left_type;
    DataType right_base_type = is_nullable(right_type) ? detach_nullable(right_type) : right_type;

    // Handle == and != operators
    if (is_equality)
    {
        // Comparing with null
        if (left_type == TYPE_NULL || right_type == TYPE_NULL)
        {
            // Only nullable types can be compared with null
            if ((is_nullable(left_type) && right_type == TYPE_NULL) ||
                (left_type == TYPE_NULL && is_nullable(right_type)))
            {
                // Create the node for comparison
                ASTNode *node = create_binary_operation_node(operator_name, left_node, right_node);
                node->data_type = TYPE_BOOL;
                return node;
            }
            else
            {
                error_exit(ERR_SEMANTIC_TYPE, "Cannot compare non-nullable type with null");
            }
        }
        else
        {
            // Handle implicit conversion between int and float if one operand is a literal
            if (left_base_type != right_base_type)
            {
                if ((left_base_type == TYPE_INT && right_base_type == TYPE_FLOAT && left_node->type == NODE_LITERAL))
                {
                    left_node = convert_to_float_node(left_node);
                    left_base_type = TYPE_FLOAT;
                }
                else if (left_base_type == TYPE_FLOAT && right_base_type == TYPE_INT && right_node->type == NODE_LITERAL)
                {
                    right_node = convert_to_float_node(right_node);
                    right_base_type = TYPE_FLOAT;
                }
            }

            // After conversions, check if base types match
            if (left_base_type != right_base_type)
            {
                error_exit(ERR_SEMANTIC_TYPE, "Cannot compare types '%d' and '%d' with operator '%s'", left_type, right_type, operator_name);
            }

            // Create the node for comparison
            ASTNode *node = create_binary_operation_node(operator_name, left_node, right_node);
            node->data_type = TYPE_BOOL;
            return node;
        }
    }
    else
    {
        // For other operators, operands must be non-nullable
        if (is_nullable(left_type) || is_nullable(right_type) || left_type == TYPE_NULL || right_type == TYPE_NULL)
        {
            error_exit(ERR_SEMANTIC_TYPE, "Cannot perform operator '%s' on nullable types or null", operator_name);
        }

        // Now both operands are non-nullable
        if (left_base_type == right_base_type)
        {
            if (left_base_type == TYPE_INT || left_base_type == TYPE_FLOAT)
            {
                ASTNode *node = create_binary_operation_node(operator_name, left_node, right_node);
                node->data_type = is_boolean ? TYPE_BOOL : left_base_type;
                return node;
            }
            else
            {
                error_exit(ERR_SEMANTIC_TYPE, "Invalid operand types for operator '%s'", operator_name);
            }
        }
        else if ((left_base_type == TYPE_INT && right_base_type == TYPE_FLOAT) ||
                 (left_base_type == TYPE_FLOAT && right_base_type == TYPE_INT))
        {
            // Implicit conversion allowed if int operand is a literal
            if (left_base_type == TYPE_INT && left_node->type == NODE_LITERAL)
            {
                left_node = convert_to_float_node(left_node);
                left_base_type = TYPE_FLOAT;
            }
            else if (right_base_type == TYPE_INT && right_node->type == NODE_LITERAL)
            {
                right_node = convert_to_float_node(right_node);
                right_base_type = TYPE_FLOAT;
            }
            else
            {
                error_exit(ERR_SEMANTIC_TYPE, "Cannot implicitly convert int variable to float");
            }

            // Now both operands are float
            ASTNode *node = create_binary_operation_node(operator_name, left_node, right_node);
            node->data_type = is_boolean ? TYPE_BOOL : TYPE_FLOAT;
            return node;
        }
        else
        {
            error_exit(ERR_SEMANTIC_TYPE, "Incompatible operand types for operator '%s'", operator_name);
        }
    }

    // Should not reach here
    error_exit(ERR_INTERNAL, "Unhandled type checking case");
    return NULL; // For compiler warnings
}

ASTNode *parse_multiplicative(Scanner *scanner, char *function_name)
{
    ASTNode *node = parse_primary_expression(scanner, function_name);

    while (current_token.type == TOKEN_MULTIPLY || current_token.type == TOKEN_DIVIDE)
    {
        const char *operator_name = current_token.lexeme;
        current_token = get_next_token(scanner);
        ASTNode *right_node = parse_primary_expression(scanner, function_name);

        // Выполняем проверку типов и устанавливаем data_type
        node = perform_type_checking_and_create_node(operator_name, node, right_node);
    }

    return node;
}

ASTNode *parse_additive(Scanner *scanner, char *function_name)
{
    ASTNode *node = parse_multiplicative(scanner, function_name);

    while (current_token.type == TOKEN_PLUS || current_token.type == TOKEN_MINUS)
    {
        const char *operator_name = current_token.lexeme;
        current_token = get_next_token(scanner);
        ASTNode *right_node = parse_multiplicative(scanner, function_name);

        // Выполняем проверку типов и устанавливаем data_type
        node = perform_type_checking_and_create_node(operator_name, node, right_node);
    }

    return node;
}

ASTNode *parse_relational(Scanner *scanner, char *function_name)
{
    ASTNode *node = parse_additive(scanner, function_name);

    while (current_token.type == TOKEN_LESS || current_token.type == TOKEN_LESS_EQUAL ||
           current_token.type == TOKEN_GREATER || current_token.type == TOKEN_GREATER_EQUAL)
    {
        const char *operator_name = current_token.lexeme;
        current_token = get_next_token(scanner);
        ASTNode *right_node = parse_additive(scanner, function_name);

        // Выполняем проверку типов и создаем узел
        node = perform_type_checking_and_create_node(operator_name, node, right_node);
        // Устанавливаем тип результата в TYPE_BOOL
        node->data_type = TYPE_BOOL;
    }

    return node;
}

ASTNode *parse_equality(Scanner *scanner, char *function_name)
{
    ASTNode *node = parse_relational(scanner, function_name);

    while (current_token.type == TOKEN_EQUAL || current_token.type == TOKEN_NOT_EQUAL)
    {
        const char *operator_name = current_token.lexeme;
        current_token = get_next_token(scanner);
        ASTNode *right_node = parse_relational(scanner, function_name);
        // Выполняем проверку типов и создаем узел
        node = perform_type_checking_and_create_node(operator_name, node, right_node);
        // Устанавливаем тип результата в TYPE_BOOL
        node->data_type = TYPE_BOOL;
    }

    return node;
}

ASTNode *parse_expression(Scanner *scanner, char *function_name)
{
    return parse_equality(scanner, function_name);
}

// Parses a primary expression (literal, identifier, or parenthesized expression)
ASTNode *parse_primary_expression(Scanner *scanner, char *function_name)
{
    LOG("DEBUG_PARSER: Parsing primary expression. Current token: %d\n Line and column: %d %d\n", current_token.type, current_token.line, current_token.column);

    if (current_token.type == TOKEN_INT_LITERAL)
    {
        char *value = string_duplicate(current_token.lexeme);
        ASTNode *literal_node = create_literal_node(TYPE_INT, value);
        current_token = get_next_token(scanner);
        return literal_node;
    }
    else if (current_token.type == TOKEN_FLOAT_LITERAL)
    {
        char *value = string_duplicate(current_token.lexeme);
        ASTNode *literal_node = create_literal_node(TYPE_FLOAT, value);
        current_token = get_next_token(scanner);
        return literal_node;
    }
    else if (current_token.type == TOKEN_STRING_LITERAL)
    {
        char *value = string_duplicate(current_token.lexeme);
        ASTNode *literal_node = create_literal_node(TYPE_U8, value);
        current_token = get_next_token(scanner);
        return literal_node;
    }
    else if (current_token.type == TOKEN_IDENTIFIER)
    {
        // DataType data_type;
        char *identifier_name = NULL;
        Symbol *symbol = NULL;
        bool is_builtin = is_builtin_function(current_token.lexeme, scanner);
        if (is_builtin)
        {
            return parse_builtin_fucntion_call(scanner, symbol, identifier_name, function_name);
        }
        else
        {
            symbol = symtable_search(&symtable, current_token.lexeme);

            if (symbol != NULL && symbol->symbol_type == SYMBOL_FUNCTION)
            {
                return parse_function_call(scanner, symbol, identifier_name, function_name);
            }
            else
            {
                return parse_idendifier(scanner, symbol, identifier_name, function_name);
            }
        }
    }
    else if (current_token.type == TOKEN_LEFT_PAREN)
    {
        current_token = get_next_token(scanner);
        ASTNode *expr_node = parse_expression(scanner, function_name);
        expect_token(TOKEN_RIGHT_PAREN, scanner);
        return expr_node;
    }
    else if (current_token.type == TOKEN_NULL)
    {
        char *value = string_duplicate(current_token.lexeme);
        ASTNode *literal_node = create_literal_node(TYPE_NULL, value);
        current_token = get_next_token(scanner);
        return literal_node;
    }
    else
    {
        error_exit(ERR_SYNTAX, "Expected literal, identifier, or '(' for expression.");
        return NULL;
    }
}

// Function to parse the import line at the beginning of the program
ASTNode *parse_import(Scanner *scanner)
{
    LOG("DEBUG_PARSER: Parsing import statement\n");

    expect_token(TOKEN_CONST, scanner);

    if (current_token.type != TOKEN_IDENTIFIER || strcmp(current_token.lexeme, "ifj") != 0)
    {
        error_exit(ERR_SYNTAX, "Expected identifier 'ifj'.");
    }
    current_token = get_next_token(scanner);

    expect_token(TOKEN_ASSIGN, scanner);

    if (current_token.type != TOKEN_IMPORT)
    {
        error_exit(ERR_SYNTAX, "Expected '@import'.");
    }
    current_token = get_next_token(scanner);

    expect_token(TOKEN_LEFT_PAREN, scanner);

    if (current_token.type != TOKEN_STRING_LITERAL || strcmp(current_token.lexeme, "ifj24.zig") != 0)
    {
        error_exit(ERR_SYNTAX, "Expected string literal \"ifj24.zig\". Got: %s", current_token.lexeme);
    }
    char *import_value = string_duplicate(current_token.lexeme);
    current_token = get_next_token(scanner);

    expect_token(TOKEN_RIGHT_PAREN, scanner);

    expect_token(TOKEN_SEMICOLON, scanner);

    LOG("DEBUG_PARSER: Finished parsing import statement\n");

    return create_literal_node(TYPE_U8, import_value);
}

ASTNode *parse_builtin_fucntion_call(Scanner *scanner, Symbol *symbol, char *identifier_name, char *function_name)
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
        arguments =  parse_arguments(scanner, symbol, arguments, params_count, &arg_count, function_name, builtin_function_name);
    }
    expect_token(TOKEN_RIGHT_PAREN, scanner);

    if (arg_count != params_count)
    {
        error_exit(ERR_SEMANTIC_PARAMS, "Invalid number of params");
    }

    ASTNode *func_call_node = create_function_call_node(identifier_name, arguments, arg_count);
    func_call_node->data_type = builtin_functions[builtin_index].return_type;
    return func_call_node;
}

ASTNode *parse_function_call(Scanner *scanner, Symbol *symbol, char *identifier_name, char *function_name)
{
    identifier_name = current_token.lexeme;
    current_token = get_next_token(scanner);

    expect_token(TOKEN_LEFT_PAREN, scanner);

    ASTNode **arguments = symbol->declaration_node->parameters;
    int params_count = symbol->declaration_node->param_count;
    int arg_count = 0;

    if (current_token.type != TOKEN_RIGHT_PAREN)
    {
        arguments = parse_arguments(scanner, symbol, arguments, params_count, &arg_count, function_name, NULL);
    }
    expect_token(TOKEN_RIGHT_PAREN, scanner);

    if (arg_count != params_count)
    {
        error_exit(ERR_SEMANTIC_PARAMS, "Invalid number of params");
    }

    ASTNode *func_call_node = create_function_call_node(identifier_name, arguments, arg_count);
    func_call_node->data_type = symbol->data_type;
    return func_call_node;
}

ASTNode *parse_idendifier(Scanner *scanner, Symbol *symbol, char *identifier_name, char *function_name)
{
    identifier_name = construct_variable_name(current_token.lexeme, function_name);
    symbol = symtable_search(&symtable, identifier_name);
    if (symbol == NULL)
    {
        error_exit(ERR_SEMANTIC_UNDEF, "Undefined variable or function. Got lexeme: %s", current_token.lexeme);
    }
    ASTNode *identifier_node = create_identifier_node(identifier_name);
    identifier_node->data_type = symbol->data_type;

    current_token = get_next_token(scanner);
    return identifier_node;
}

ASTNode **parse_arguments(Scanner *scanner, Symbol *symbol, ASTNode **arguments, int param_count, int *arg_count, char *function_name, char *builtin_function_name)
{

    arguments = (ASTNode **)safe_malloc(sizeof(ASTNode *));
    arguments[(*arg_count)++] = parse_expression(scanner, function_name);
    if (check_arguments_compability(symbol, arguments, arg_count, builtin_function_name))
    {
        error_exit(ERR_SEMANTIC_PARAMS, "Invalid type of arguments");
    }

    while (current_token.type == TOKEN_COMMA)
    {
        current_token = get_next_token(scanner);
        if (current_token.type == TOKEN_RIGHT_PAREN)
            break;
        if (*arg_count >= param_count)
        {
            error_exit(ERR_SEMANTIC_PARAMS, "Too many arguments in function call to %s.", symbol->name);
        }
        arguments = (ASTNode **)safe_realloc(arguments, ((*arg_count) + 1) * sizeof(ASTNode *));
        arguments[(*arg_count)++] = parse_expression(scanner, function_name);
        if (check_arguments_compability(symbol, arguments, arg_count, builtin_function_name))
        {
            error_exit(ERR_SEMANTIC_PARAMS, "Invalid type of arguments");
        }
    }
    return arguments;
}

bool check_arguments_compability(Symbol *symbol, ASTNode **arguments, int *arg_count, char *builtin_function_name)
{
    if(arguments == NULL){
        return false;
    }
    DataType declared_datatype = TYPE_UNKNOWN;
    int builtin_index = -1;

    if (symbol->declaration_node != NULL)
    {
        declared_datatype = symbol->declaration_node->parameters[(*arg_count) - 1]->data_type;
    }
    else if (builtin_function_name != NULL)
    {
        builtin_index = get_builtin_function_index(builtin_function_name);
        if (builtin_index == -1)
        {
            error_exit(ERR_SEMANTIC_UNDEF, "Unknown built-in function: %s", builtin_function_name);
        }
        declared_datatype = builtin_functions[builtin_index].param_types[(*arg_count) - 1];
    }
    else
    {
        error_exit(ERR_INTERNAL, "Both symbol->declaration_node and builtin_function_name are NULL");
    }

    return (arguments[(*arg_count) - 1]->data_type != declared_datatype && declared_datatype != TYPE_ALL);
}

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
    error_exit(ERR_SEMANTIC_UNDEF, "Unknown built-in function: %s", identifier);
    return false;
}

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
    return TYPE_UNKNOWN;
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
    return -1;
}

void scope_check_identifiers_in_tree(ASTNode *root)
{
    if (!root)
    {
        return;
    }
    if ((root->type == NODE_IDENTIFIER || root->type == NODE_ASSIGNMENT) && strcmp(root->name, "_") != 0)
    {
        Symbol *symbol = symtable_search(&symtable, root->name);
        ASTNode *declaration_node;
        if (symbol->symbol_type == SYMBOL_PARAMETER)
        {
            Symbol *parent_function = symtable_search(&symtable, symbol->parent_function);
            declaration_node = parent_function->declaration_node;
            bool found = scope_check(declaration_node, root);
            if (!found)
            {
                error_exit(ERR_SEMANTIC_UNDEF, "Variable is not defined in this scope");
            }
        }
        else
        {
            declaration_node = symbol->declaration_node;
            bool found = scope_check(declaration_node, root);
            if (!found)
            {
                error_exit(ERR_SEMANTIC_UNDEF, "Variable is not defined in this scope");
            }
        }
    }
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
    if (node_decl == node_identifier)
    {
        return true;
    }
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

    return false;
}

void parse_functions_declaration(Scanner *scanner, ASTNode *program_node)
{
    Scanner saved_scanner_state = *scanner;
    FILE saved_input = *scanner->input;
    Token saved_token = current_token;

    ASTNode *current_function = NULL;
    while (current_token.type != TOKEN_EOF)
    {
        if ((current_token.type == TOKEN_PUB) || (current_token.type == TOKEN_FN))
        {
            ASTNode *function_node = parse_function(scanner, false);
            if (program_node->body == NULL)
            {
                program_node->body = function_node;
            }
            else
            {
                current_function->next = function_node;
            }
            current_function = function_node;
        }
        else
        {
            error_exit(ERR_SYNTAX, "Expected function definition. Line: %d, Column: %d", current_token.line, current_token.column);
        }
    }
    *scanner = saved_scanner_state;
    *scanner->input = saved_input;
    current_token = saved_token;

    return;
}

DataType parse_type(Scanner *scanner)
{
    if (current_token.type == TOKEN_I32)
    {
        current_token = get_next_token(scanner);
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
            current_token = get_next_token(scanner);
            return TYPE_INT_NULLABLE;
        }
        else if (current_token.type == TOKEN_F64)
        {
            current_token = get_next_token(scanner);
            return TYPE_FLOAT_NULLABLE;
        }
        else if (current_token.type == TOKEN_U8)
        {
            current_token = get_next_token(scanner);
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
        current_token = get_next_token(scanner);
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
            current_token = get_next_token(scanner);
            return TYPE_INT_NULLABLE;
        }
        else if (current_token.type == TOKEN_F64)
        {
            current_token = get_next_token(scanner);
            return TYPE_FLOAT_NULLABLE;
        }
        else if (current_token.type == TOKEN_U8)
        {
            current_token = get_next_token(scanner);
            return TYPE_U8_NULLABLE;
        }
    }

    error_exit(ERR_SYNTAX, "Expected return type.");
    return TYPE_UNKNOWN;
}

// Parses a block of statements enclosed in {}

/*
A function that recursively traverses an entire AST and:
1. In case of void function looks for any return statement and checks if its type is different from void
2. In case of a function of a type other than void, it checks if all return statements match the given type of the function,
 and looks for a return statement in the main block of the function.
*/
bool check_return_types_recursive(ASTNode *function_node, DataType return_type)
{
    bool has_return = false;

    if (!function_node)
    {
        return false;
    }
    if (function_node->type == NODE_RETURN)
    {
        if (function_node->data_type != return_type && !can_assign_type(return_type, function_node->data_type))
        {
            error_exit(ERR_SEMANTIC_PARAMS, "Incompatible types of return statement");
        }
        return true;
    }
    if (function_node->type == NODE_IF)
    {
        bool if_branch = check_return_types_recursive(function_node->body, return_type);
        bool else_branch = function_node->left ? check_return_types_recursive(function_node->left, return_type) : false;
        has_return = if_branch && else_branch;
    }

    else if (function_node->type == NODE_WHILE)
    {
        check_return_types_recursive(function_node->body, return_type);
    }
    else
    {

        has_return |= check_return_types_recursive(function_node->body, return_type);
        has_return |= check_return_types_recursive(function_node->left, return_type);
    }

    if (has_return)
    {
        return true;
    }

    return check_return_types_recursive(function_node->next, return_type);
}

bool check_all_return_types(ASTNode *function_node, DataType return_type)
{
    if (!function_node)
    {
        return true;
    }
    if (function_node->type == NODE_RETURN)
    {
        if (function_node->data_type != return_type && (function_node->left->type != NODE_LITERAL || !can_assign_type(return_type, function_node->data_type)))
        {
            error_exit(ERR_SEMANTIC_PARAMS, "Incompatible return type. Expected: %d, Got: %d", return_type, function_node->data_type);
        }
    }
    bool body_check = check_all_return_types(function_node->body, return_type);
    bool left_check = check_all_return_types(function_node->left, return_type);
    bool next_check = check_all_return_types(function_node->next, return_type);

    return body_check && left_check && next_check;
}

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
        if (!check_return_types_recursive(function_node, return_type))
        {
            error_exit(ERR_SEMANTIC_RETURN, "Missing return statement");
        }
        if (!check_all_return_types(function_node, return_type))
        {
            error_exit(ERR_SEMANTIC_PARAMS, "Function contains return statements with incompatible types");
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
}

bool can_assign_type(DataType expected_type, DataType actual_type)
{
    if (expected_type == actual_type)
        return true;

    // Разрешаем неявное преобразование int к float
    if (expected_type == TYPE_FLOAT && actual_type == TYPE_INT)
        return true;

    // Разрешаем присвоение не-nullable значения nullable переменной
    if (is_nullable(expected_type) && !is_nullable(actual_type) && detach_nullable(expected_type) == actual_type)
        return true;

    // Не разрешаем присвоение nullable значения не-nullable переменной
    if (!is_nullable(expected_type) && is_nullable(actual_type))
        return false;

    // Разрешаем присвоение null nullable переменной
    if (is_nullable(expected_type) && actual_type == TYPE_NULL)
        return true;

    return false;
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