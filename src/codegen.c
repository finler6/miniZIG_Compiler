/**
 * @file codegen.c
 *
 * Code generation module implementation.
 *
 * IFJ Project 2024, Team 'xstepa77'
 *
 * @author
 *   <xlitvi02> Gleb Litvinchuk
 *   <xstepa77> Pavel Stepanov
 *   <xkovin00> Viktoriia Kovin
 *   <xshmon00> Gleb Shmonin
 */

#include "codegen.h"
#include "ast.h"
#include "parser.h"
#include "utils.h"
#include "error.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static TempVarMapEntry *temp_var_map = NULL;
static int unique_var_counter = 0;
static int temp_var_counter = 0;
static BuiltinFunctionUsage builtin_function_usage = {false, false, false};

DeclaredVar *declared_vars = NULL;
TempVar *temp_vars = NULL;

/** Global variable for storing the output file */
static FILE *output_file;

int get_next_temp_var() {
    return temp_var_counter++;
}

/**
 * Adds a temporary variable to the list if not already added.
 */
void add_temp_var(const char *var_name) {
    TempVar *current = temp_vars;
    while (current) {
        if (strcmp(current->name, var_name) == 0) {
            return; // Variable already added
        }
        current = current->next;
    }

    TempVar *new_var = safe_malloc(sizeof(TempVar));
    new_var->name = string_duplicate(var_name);
    new_var->next = temp_vars;
    temp_vars = new_var;
}

/**
 * Resets the list of temporary variables.
 */
void reset_temp_vars() {
    TempVar *current = temp_vars;
    while (current) {
        TempVar *next = current->next;
        safe_free(current->name);
        safe_free(current);
        current = next;
    }
    temp_vars = NULL;
}

/**
 * Checks if a variable is already declared.
 */
bool is_variable_declared(const char *var_name) {
    DeclaredVar *current = declared_vars;
    while (current) {
        if (strcmp(current->var_name, var_name) == 0) {
            return true;
        }
        current = current->next;
    }
    return false;
}

/**
 * Adds a declared variable to the list if not already declared.
 */
void add_declared_variable(const char *var_name) {
    if (is_variable_declared(var_name)) {
        // Variable already declared, do not add again
        return;
    }
    DeclaredVar *new_var = safe_malloc(sizeof(DeclaredVar));
    new_var->var_name = string_duplicate(var_name);
    new_var->next = declared_vars;
    declared_vars = new_var;
}

/**
 * Resets the temporary variable map.
 */
void reset_temp_var_map() {
    TempVarMapEntry *entry = temp_var_map;
    while (entry != NULL) {
        TempVarMapEntry *next = entry->next;
        safe_free(entry->var_name);
        safe_free(entry->key);
        safe_free(entry);
        entry = next;
    }
    temp_var_map = NULL;
}

/**
 * Resets the list of declared variables.
 */
void reset_declared_variables() {
    DeclaredVar *current = declared_vars;
    while (current) {
        DeclaredVar *next = current->next;
        safe_free(current->var_name);
        safe_free(current);
        current = next;
    }
    declared_vars = NULL;
}

/**
 * Generates a unique variable name based on a base name.
 * Optionally maps the variable name to an AST node and key.
 */
char *generate_unique_var_name(const char *base_name, ASTNode *node, const char *key) {
    char *var_name = safe_malloc(64);
    snprintf(var_name, 64, "%%%s_%d", base_name, unique_var_counter++);
    add_temp_var(var_name); // Add to temp variable list

    if (node != NULL && key != NULL) {
        // Map the AST node and key to the variable name
        TempVarMapEntry *new_entry = safe_malloc(sizeof(TempVarMapEntry));
        new_entry->node = node;
        new_entry->key = string_duplicate(key);
        new_entry->var_name = var_name;
        new_entry->next = temp_var_map;
        temp_var_map = new_entry;
    }

    return var_name;
}

/**
 * Retrieves the temporary variable name associated with a given AST node and key.
 */
char *get_temp_var_name_for_node(ASTNode *node, const char *key) {
    TempVarMapEntry *entry = temp_var_map;
    while (entry != NULL) {
        if (entry->node == node && strcmp(entry->key, key) == 0) {
            return entry->var_name;
        }
        entry = entry->next;
    }
    error_exit(ERR_INTERNAL, "Error: Temporary variable for node not found.\n");
    return NULL;
}

/**
 * Collects usage information of built-in functions in the AST.
 */
void collect_builtin_function_usage(ASTNode *node) {
    if (!node)
        return;

    switch (node->type)
    {
    case NODE_PROGRAM:

        for (ASTNode *func = node->body; func != NULL; func = func->next)
        {
            collect_builtin_function_usage(func);
        }
        break;

    case NODE_FUNCTION:

        collect_builtin_function_usage(node->body);
        break;

    case NODE_BLOCK:

        for (ASTNode *stmt = node->body; stmt != NULL; stmt = stmt->next)
        {
            collect_builtin_function_usage(stmt);
        }
        break;

    case NODE_FUNCTION_CALL:
        if (strcmp(node->name, "ifj.substring") == 0)
        {
            builtin_function_usage.uses_substring = true;
        }
        else if (strcmp(node->name, "ifj.strcmp") == 0)
        {
            builtin_function_usage.uses_strcmp = true;
        }
        else if (strcmp(node->name, "ifj.string") == 0)
        {
            builtin_function_usage.uses_string = true;
        }

        for (int i = 0; i < node->arg_count; ++i)
        {
            collect_builtin_function_usage(node->arguments[i]);
        }
        break;

    case NODE_BINARY_OPERATION:
        collect_builtin_function_usage(node->left);
        collect_builtin_function_usage(node->right);
        break;

    case NODE_VARIABLE_DECLARATION:
    case NODE_ASSIGNMENT:
        collect_builtin_function_usage(node->left);
        break;

    case NODE_IF:
        collect_builtin_function_usage(node->condition);
        collect_builtin_function_usage(node->body);
        if (node->left)
        {
            collect_builtin_function_usage(node->left);
        }
        break;

    case NODE_WHILE:
        collect_builtin_function_usage(node->condition);
        collect_builtin_function_usage(node->body);
        break;

    case NODE_RETURN:
        if (node->left)
        {
            collect_builtin_function_usage(node->left);
        }
        break;

    case NODE_LITERAL:
    case NODE_IDENTIFIER:

        break;

    default:
        error_exit(ERR_INTERNAL, "Unsupported node type in collect_builtin_function_usage: %d\n", node->type);
        break;
    }
}

/**
 * Generates the built-in 'substring' function code if used.
 */
void codegen_generate_substring_function() {
    fprintf(output_file,
            "LABEL ifj-substring\n"
            "CREATEFRAME\n"
            "PUSHFRAME\n"
            "DEFVAR LF@str\n"
            "DEFVAR LF@start\n"
            "DEFVAR LF@end\n"
            "DEFVAR LF@length\n"
            "DEFVAR LF@retval\n"
            "DEFVAR LF@tmp_bool\n"
            "DEFVAR LF@tmp_char\n"
            "POPS LF@end\n"
            "POPS LF@start\n"
            "POPS LF@str\n"
            "STRLEN LF@length LF@str\n"

            "LT LF@tmp_bool LF@start int@0\n"
            "JUMPIFEQ $substr_null LF@tmp_bool bool@true\n"

            "LT LF@tmp_bool LF@end int@0\n"
            "JUMPIFEQ $substr_null LF@tmp_bool bool@true\n"

            "GT LF@tmp_bool LF@start LF@end\n"
            "JUMPIFEQ $substr_null LF@tmp_bool bool@true\n"

            "LT LF@tmp_bool LF@start LF@length\n"
            "JUMPIFEQ $check_j LF@tmp_bool bool@true\n"
            "JUMP $substr_null\n"

            "LABEL $check_j\n"
            "GT LF@tmp_bool LF@end LF@length\n"
            "JUMPIFEQ $substr_null LF@tmp_bool bool@true\n"

            "SUB LF@length LF@end LF@start\n"

            "MOVE LF@retval string@\n"

            "LABEL $substr_loop\n"
            "JUMPIFEQ $substr_end LF@length int@0\n"

            "GETCHAR LF@tmp_char LF@str LF@start\n"

            "CONCAT LF@retval LF@retval LF@tmp_char\n"

            "ADD LF@start LF@start int@1\n"
            "SUB LF@length LF@length int@1\n"

            "JUMP $substr_loop\n"
            "LABEL $substr_end\n"
            "PUSHS LF@retval\n"
            "POPFRAME\n"
            "RETURN\n"
            "LABEL $substr_null\n"
            "PUSHS nil@nil\n"
            "POPFRAME\n"
            "RETURN\n");
}

/**
 * Generates the built-in 'strcmp' function code if used.
 */
void codegen_generate_strcmp_function() {
    fprintf(output_file,
            "LABEL ifj-strcmp\n"
            "CREATEFRAME\n"
            "PUSHFRAME\n"
            "DEFVAR LF@str1\n"
            "DEFVAR LF@str2\n"
            "DEFVAR LF@len1\n"
            "DEFVAR LF@len2\n"
            "DEFVAR LF@i\n"
            "DEFVAR LF@char1\n"
            "DEFVAR LF@char2\n"
            "DEFVAR LF@retval\n"
            "DEFVAR LF@tmp_int\n"
            "DEFVAR LF@tmp_bool\n"
            "POPS LF@str2\n"
            "POPS LF@str1\n"

            "STRLEN LF@len1 LF@str1\n"
            "STRLEN LF@len2 LF@str2\n"
            "MOVE LF@i int@0\n"
            "LABEL $strcmp_loop\n"
            "LT LF@tmp_bool LF@i LF@len1\n"
            "JUMPIFEQ $strcmp_end LF@tmp_bool bool@false\n"
            "LT LF@tmp_bool LF@i LF@len2\n"
            "JUMPIFEQ $strcmp_end LF@tmp_bool bool@false\n"
            "GETCHAR LF@char1 LF@str1 LF@i\n"
            "GETCHAR LF@char2 LF@str2 LF@i\n"
            "GT LF@tmp_bool LF@char1 LF@char2\n"
            "JUMPIFEQ $strcmp_greater LF@tmp_bool bool@true\n"
            "LT LF@tmp_bool LF@char1 LF@char2\n"
            "JUMPIFEQ $strcmp_less LF@tmp_bool bool@true\n"
            "ADD LF@i LF@i int@1\n"
            "JUMP $strcmp_loop\n"
            "LABEL $strcmp_end\n"
            "SUB LF@tmp_int LF@len1 LF@len2\n"
            "JUMPIFEQ $strcmp_equal LF@tmp_int int@0\n"
            "GT LF@tmp_bool LF@len1 LF@len2\n"
            "JUMPIFEQ $strcmp_greater LF@tmp_bool bool@true\n"
            "JUMP $strcmp_less\n"
            "LABEL $strcmp_equal\n"
            "MOVE LF@retval int@0\n"
            "JUMP $strcmp_finish\n"
            "LABEL $strcmp_greater\n"
            "MOVE LF@retval int@1\n"
            "JUMP $strcmp_finish\n"
            "LABEL $strcmp_less\n"
            "MOVE LF@retval int@-1\n"
            "LABEL $strcmp_finish\n"
            "PUSHS LF@retval\n"
            "POPFRAME\n"
            "RETURN\n");
}

/**
 * Generates the built-in 'string' function code if used.
 */
void codegen_generate_ifj_string_function() {
    fprintf(output_file,
            "LABEL ifj-string\n"
            "CREATEFRAME\n"
            "PUSHFRAME\n"
            "DEFVAR LF@str_literal\n"
            "POPS LF@str_literal\n"
            "PUSHS LF@str_literal\n"
            "POPFRAME\n"
            "RETURN\n");
}

/**
 * Generates code for all used built-in functions.
 */
void codegen_generate_builtin_functions() {
    if (builtin_function_usage.uses_substring) {
        codegen_generate_substring_function();
    }
    if (builtin_function_usage.uses_strcmp) {
        codegen_generate_strcmp_function();
    }
    if (builtin_function_usage.uses_string) {
        codegen_generate_ifj_string_function();
    }
}

/**
 * Removes the last prefix from a variable or function name (e.g., module prefix).
 */
const char *remove_last_prefix(const char *name) {
    static char buffer[1024];
    if (!name) {
        return NULL;
    }

    const char *last_dot = strrchr(name, '.');
    if (last_dot && *(last_dot + 1) != '\0') {
        size_t new_len = last_dot - name;
        if (new_len >= sizeof(buffer)) {
            error_exit(ERR_INTERNAL, "Error: Buffer overflow in remove_last_prefix.\n");
        }
        strncpy(buffer, name, new_len);
        buffer[new_len] = '\0';
        return buffer;
    }
    return name;
}

char *escape_ifj24_string(const char *input);

static int label_counter = 0;

/**
 * Generates a unique label number.
 */
int generate_unique_label() {
    return label_counter++;
}

/**
 * Initializes the code generator with the specified output file.
 */
void codegen_init(const char *filename) {
    if (filename) {
        output_file = fopen(filename, "w");
        if (!output_file) {
            error_exit(ERR_INTERNAL, "Error: Cannot open output file %s for writing.\n", filename);
        }
    } else {
        output_file = stdout;
    }
}

/**
 * Finalizes the code generator and closes the output file.
 */
void codegen_finalize() {
    if (output_file) {
        fclose(output_file);
        output_file = NULL;
    }
}

/**
 * Generates code for the entire program.
 */
void codegen_generate_program(ASTNode *program_node) {
    if (!program_node || program_node->type != NODE_PROGRAM) {
        return;
    }

    collect_builtin_function_usage(program_node);

    fprintf(output_file, ".IFJcode24\n");

    fprintf(output_file, "CALL main\n");
    fprintf(output_file, "EXIT int@0\n");

    ASTNode *current_function = program_node->body;

    while (current_function) {
        if (current_function->type == NODE_FUNCTION) {
            codegen_generate_function(current_function);
        }
        current_function = current_function->next;
    }

    codegen_generate_builtin_functions();
}

/**
 * Checks if a variable name corresponds to a function parameter.
 */
bool is_function_parameter(ASTNode *function, const char *var_name) {
    for (int i = 0; i < function->param_count; i++) {
        if (strcmp(remove_last_prefix(function->parameters[i]->name), var_name) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * Generates code for a function.
 */
void codegen_generate_function(ASTNode *function) {
    reset_temp_var_map();
    reset_declared_variables();
    reset_temp_vars(); // Reset temporary variables

    fprintf(output_file, "LABEL %s\n", function->name);
    fprintf(output_file, "CREATEFRAME\n");
    fprintf(output_file, "PUSHFRAME\n");

    // Declare function parameters
    for (int i = 0; i < function->param_count; i++) {
        const char *param_name = remove_last_prefix(function->parameters[i]->name);
        fprintf(output_file, "DEFVAR LF@%s\n", param_name);
        fprintf(output_file, "POPS LF@%s\n", param_name);
        add_declared_variable(param_name);
    }

    // Declare standard temporary variables
    fprintf(output_file, "DEFVAR LF@%%tmp_type\n");
    add_declared_variable("%%tmp_type");
    fprintf(output_file, "DEFVAR LF@%%tmp_var\n");
    add_declared_variable("%%tmp_var");
    fprintf(output_file, "DEFVAR LF@%%tmp_bool\n");
    add_declared_variable("%%tmp_bool");

    // First Pass: Collect variables (including temporary ones)
    collect_variables_in_block(function->body);

    // Declare all variables collected (excluding parameters and standard temporary variables)
    DeclaredVar *current_declared_var = declared_vars;
    while (current_declared_var) {
        const char *var_name = current_declared_var->var_name;
        // Skip parameters and standard temporary variables
        if (strcmp(var_name, "%%tmp_type") != 0 &&
            strcmp(var_name, "%%tmp_var") != 0 &&
            strcmp(var_name, "%%tmp_bool") != 0 &&
            !is_function_parameter(function, var_name)) {
            fprintf(output_file, "DEFVAR LF@%s\n", var_name);
        }
        current_declared_var = current_declared_var->next;
    }

    // Declare all temporary variables collected
    TempVar *current_temp_var = temp_vars;
    while (current_temp_var) {
        const char *var_name = current_temp_var->name;
        fprintf(output_file, "DEFVAR LF@%s\n", var_name);
        current_temp_var = current_temp_var->next;
    }

    // Second Pass: Generate code
    codegen_generate_block(output_file, function->body, function->name);

    fprintf(output_file, "POPFRAME\n");
    fprintf(output_file, "RETURN\n");
}

/**
 * Collects variables used in a block.
 */
void collect_variables_in_block(ASTNode *block_node) {
    ASTNode *current = block_node->body;
    while (current) {
        collect_variables_in_statement(current);
        current = current->next;
    }
}

/**
 * Collects variables used in a statement.
 */
void collect_variables_in_statement(ASTNode *node) {
    if (node == NULL) {
        return;
    }

    switch (node->type)
    {
    case NODE_VARIABLE_DECLARATION:
        add_declared_variable(remove_last_prefix(node->name));
        if (node->left != NULL)
        {
            collect_variables_in_expression(node->left);
        }
        break;

    case NODE_ASSIGNMENT:
        add_declared_variable(remove_last_prefix(node->name));
        collect_variables_in_expression(node->left);
        break;

    case NODE_RETURN:
        if (node->left)
        {
            collect_variables_in_expression(node->left);
        }
        break;

    case NODE_IF:
        collect_variables_in_expression(node->condition);
        collect_variables_in_block(node->body);
        if (node->left)
        {
            collect_variables_in_block(node->left);
        }
        break;

    case NODE_WHILE:
        collect_variables_in_expression(node->condition);
        collect_variables_in_block(node->body);
        break;

    case NODE_FUNCTION_CALL:
        collect_variables_in_function_call(node);
        break;

    default:
        // Handle other statement types if necessary
        break;
    }
}

/**
 * Collects variables used in a function call.
 */
void collect_variables_in_function_call(ASTNode *node) {
    if (node == NULL || node->type != NODE_FUNCTION_CALL) {
        error_exit(ERR_INTERNAL, "Invalid function call node for variable collection\n");
    }

    if (strcmp(node->name, "ifj.write") == 0) {
        ASTNode *arg = node->arguments[0];
        collect_variables_in_expression(arg);

        // Associate temp_var_name with 'arg' using key "temp_var"
        generate_unique_var_name("temp", arg, "temp_var");

        if (is_nullable(arg->data_type)) {
            // Associate temp_type_name with 'arg' using key "temp_type"
            generate_unique_var_name("tmp_type", arg, "temp_type");
        }
    } else if (strcmp(node->name, "ifj.readi32") == 0 ||
               strcmp(node->name, "ifj.readf64") == 0 ||
               strcmp(node->name, "ifj.readstr") == 0) {
        // Associate retval_var with 'node' using key "retval_var"
        generate_unique_var_name("retval", node, "retval_var");
    } else if (strcmp(node->name, "ifj.length") == 0) {
        collect_variables_in_expression(node->arguments[0]);

        // Associate tmp_str_var with 'node->arguments[0]' using key "tmp_str_var"
        generate_unique_var_name("tmp_str", node->arguments[0], "tmp_str_var");

        // Associate retval_var with 'node' using key "retval_var"
        generate_unique_var_name("retval", node, "retval_var");
    } else if (strcmp(node->name, "ifj.concat") == 0) {
        collect_variables_in_expression(node->arguments[0]);
        collect_variables_in_expression(node->arguments[1]);

        // Associate variables with respective argument nodes
        generate_unique_var_name("tmp_str1", node->arguments[0], "tmp_str1_var");
        generate_unique_var_name("tmp_str2", node->arguments[1], "tmp_str2_var");

        // Associate retval_var with 'node' using key "retval_var"
        generate_unique_var_name("retval", node, "retval_var");
    } else if (strcmp(node->name, "ifj.i2f") == 0 ||
               strcmp(node->name, "ifj.f2i") == 0) {
        collect_variables_in_expression(node->arguments[0]);

        // Associate tmp_var with 'node->arguments[0]' using key "tmp_var"
        generate_unique_var_name("tmp_var", node->arguments[0], "tmp_var");

        // Associate retval_var with 'node' using key "retval_var"
        generate_unique_var_name("retval", node, "retval_var");
    } else if (strcmp(node->name, "ifj.substring") == 0 ||
               strcmp(node->name, "ifj.strcmp") == 0 ||
               strcmp(node->name, "ifj.string") == 0) {
        for (int i = 0; i < node->arg_count; ++i) {
            collect_variables_in_expression(node->arguments[i]);
        }
        // The built-in function handles variables internally
    } else if (strcmp(node->name, "ifj.chr") == 0) {
        collect_variables_in_expression(node->arguments[0]);

        // Associate tmp_int_var with 'node->arguments[0]' using key "tmp_int_var"
        generate_unique_var_name("tmp_int", node->arguments[0], "tmp_int_var");

        // Associate tmp_temp_var and retval_var with 'node' using unique keys
        generate_unique_var_name("tmp_temp", node, "tmp_temp_var");
        generate_unique_var_name("retval", node, "retval_var");
    } else if (strcmp(node->name, "ifj.ord") == 0) {
        collect_variables_in_expression(node->arguments[0]); // String argument
        collect_variables_in_expression(node->arguments[1]); // Index argument

        // Associate variables with 'node' using unique keys
        generate_unique_var_name("str", node, "str_var");
        generate_unique_var_name("idx", node, "idx_var");
        generate_unique_var_name("strlen", node, "strlen_var");
        generate_unique_var_name("tmp_bool", node, "tmp_bool_var");
        generate_unique_var_name("retval", node, "retval_var");
    } else {
        // User-defined function call
        for (int i = 0; i < node->arg_count; ++i) {
            collect_variables_in_expression(node->arguments[i]);
        }
        if (node->left) {
            add_declared_variable(remove_last_prefix(node->left->name));
        }
    }
}

/**
 * Collects variables used in an expression.
 */
void collect_variables_in_expression(ASTNode *node) {
    if (node == NULL) {
        return;
    }

    switch (node->type)
    {
    case NODE_LITERAL:
        // No variables to collect
        break;

    case NODE_IDENTIFIER:
        // Ensure variable is declared
        add_declared_variable(remove_last_prefix(node->name));
        break;

    case NODE_BINARY_OPERATION:
    {
        collect_variables_in_expression(node->left);
        generate_unique_var_name("temp", node->left, "temp_var");

        collect_variables_in_expression(node->right);
        generate_unique_var_name("temp", node->right, "temp_var");

        generate_unique_var_name("result", node, "result_var");
        break;
    }

    case NODE_FUNCTION_CALL:
        collect_variables_in_function_call(node);
        break;

    default:
        error_exit(ERR_INTERNAL, "Unsupported expression type for variable collection, type: %d, name: %s\n", node->type, node->name ? node->name : "NULL");
    }
}

/**
 * Generates code for a block of statements.
 */
void codegen_generate_block(FILE *output, ASTNode *block_node, const char *current_function) {
    ASTNode *current = block_node->body;
    while (current) {
        codegen_generate_statement(output, current, current_function);
        current = current->next;
    }
}

/**
 * Generates code for an expression.
 */
void codegen_generate_expression(FILE *output, ASTNode *node, const char *current_function) {
    if (node == NULL) {
        return;
    }

    switch (node->type)
    {
    case NODE_LITERAL:
        if (node->data_type == TYPE_INT)
        {
            fprintf(output, "PUSHS int@%s\n", node->value);
        }
        else if (node->data_type == TYPE_FLOAT)
        {
            double float_value = atof(node->value);
            fprintf(output, "PUSHS float@%.13a\n", float_value);
        }
        else if (node->data_type == TYPE_U8)
        {
            char *escaped_value = escape_ifj24_string(node->value);
            fprintf(output, "PUSHS string@%s\n", escaped_value);
            safe_free(escaped_value);
        }
        else if (node->data_type == TYPE_NULL)
        {
            fprintf(output, "PUSHS nil@nil\n");
        }
        else if (node->data_type == TYPE_BOOL)
        {
            fprintf(output, "PUSHS bool@%s\n", strcmp(node->value, "true") == 0 ? "true" : "false");
        }
        break;

    case NODE_IDENTIFIER:
    {
        if (strcmp(node->name, "nil") == 0)
        {
            fprintf(output, "PUSHS nil@nil\n");
            fprintf(output, "EQS\n");
            fprintf(output, "NOTS\n");
        }
        else if (is_nullable(node->data_type))
        {
            fprintf(output, "TYPE LF@%%tmp_type LF@%s\n", remove_last_prefix(node->name));
            fprintf(output, "PUSHS LF@%%tmp_type\n");
            fprintf(output, "PUSHS string@nil\n");
            fprintf(output, "EQS\n");
            fprintf(output, "NOTS\n");
        }
        else if (strcmp(node->name, "true") == 0)
        {
            fprintf(output, "PUSHS bool@true\n");
        }
        else if (strcmp(node->name, "false") == 0)
        {
            fprintf(output, "PUSHS bool@false\n");
        }
        else
        {
            fprintf(output, "PUSHS LF@%s\n", remove_last_prefix(node->name));
        }
    }
    break;

    case NODE_BINARY_OPERATION:
    {
        // Generate code for left operand
        codegen_generate_expression(output, node->left, current_function);
        char *left_temp_var = get_temp_var_name_for_node(node->left, "temp_var");
        fprintf(output, "POPS LF@%s\n", left_temp_var);

        // Generate code for right operand
        codegen_generate_expression(output, node->right, current_function);
        char *right_temp_var = get_temp_var_name_for_node(node->right, "temp_var");
        fprintf(output, "POPS LF@%s\n", right_temp_var);

        char *result_temp_var = get_temp_var_name_for_node(node, "result_var");
        // Perform the operation based on the operator
        if (strcmp(node->name, "-") == 0)
        {
            fprintf(output, "SUB LF@%s LF@%s LF@%s\n", result_temp_var, left_temp_var, right_temp_var);
        }
        else if (strcmp(node->name, "/") == 0)
        {
            if (node->left->data_type == TYPE_INT && node->right->data_type == TYPE_INT)
            {
                fprintf(output, "IDIV LF@%s LF@%s LF@%s\n", result_temp_var, left_temp_var, right_temp_var);
            }
            else
            {
                // Convert to float if necessary
                if (node->left->data_type == TYPE_INT)
                {
                    fprintf(output, "INT2FLOAT LF@%s LF@%s\n", left_temp_var, left_temp_var);
                }
                if (node->right->data_type == TYPE_INT)
                {
                    fprintf(output, "INT2FLOAT LF@%s LF@%s\n", right_temp_var, right_temp_var);
                }
                fprintf(output, "DIV LF@%s LF@%s LF@%s\n", result_temp_var, left_temp_var, right_temp_var);
            }
        }
        else if (strcmp(node->name, "+") == 0)
        {
            fprintf(output, "ADD LF@%s LF@%s LF@%s\n", result_temp_var, left_temp_var, right_temp_var);
        }
        else if (strcmp(node->name, "*") == 0)
        {
            fprintf(output, "MUL LF@%s LF@%s LF@%s\n", result_temp_var, left_temp_var, right_temp_var);
        }
        else if (strcmp(node->name, "<") == 0)
        {
            fprintf(output, "LT LF@%s LF@%s LF@%s\n", result_temp_var, left_temp_var, right_temp_var);
        }
        else if (strcmp(node->name, "<=") == 0)
        {
            fprintf(output, "GT LF@%s LF@%s LF@%s\n", result_temp_var, left_temp_var, right_temp_var);
            fprintf(output, "NOT LF@%s LF@%s\n", result_temp_var, result_temp_var);
        }
        else if (strcmp(node->name, ">") == 0)
        {
            fprintf(output, "GT LF@%s LF@%s LF@%s\n", result_temp_var, left_temp_var, right_temp_var);
        }
        else if (strcmp(node->name, ">=") == 0)
        {
            fprintf(output, "LT LF@%s LF@%s LF@%s\n", result_temp_var, left_temp_var, right_temp_var);
            fprintf(output, "NOT LF@%s LF@%s\n", result_temp_var, result_temp_var);
        }
        else if (strcmp(node->name, "==") == 0)
        {
            fprintf(output, "EQ LF@%s LF@%s LF@%s\n", result_temp_var, left_temp_var, right_temp_var);
        }
        else if (strcmp(node->name, "!=") == 0)
        {
            fprintf(output, "EQ LF@%s LF@%s LF@%s\n", result_temp_var, left_temp_var, right_temp_var);
            fprintf(output, "NOT LF@%s LF@%s\n", result_temp_var, result_temp_var);
        }
        else
        {
            error_exit(ERR_INTERNAL, "Unsupported operator: %s\n", node->name);
        }

        // Push the result onto the stack
        fprintf(output, "PUSHS LF@%s\n", result_temp_var);

        // Do not free temporary variable names here
        break;
    }

    case NODE_FUNCTION_CALL:
        codegen_generate_function_call(output, node, current_function);
        break;

    default:
        error_exit(ERR_INTERNAL, "Unsupported expression type for code generation, type: %d, name: %s\n", node->type, node->name);
        break;
    }
}

/**
 * Generates code for a function call.
 */
void codegen_generate_function_call(FILE *output, ASTNode *node, const char *current_function) {
    if (node == NULL || node->type != NODE_FUNCTION_CALL) {
        error_exit(ERR_INTERNAL, "Invalid function call node for code generation\n");
    }

    if (strcmp(node->name, "ifj.write") == 0) {
        ASTNode *arg = node->arguments[0];
        codegen_generate_expression(output, arg, current_function);

        char *temp_var_name = get_temp_var_name_for_node(arg, "temp_var");
        fprintf(output, "POPS LF@%s\n", temp_var_name);

        if (is_nullable(arg->data_type)) {
            char *temp_type_name = get_temp_var_name_for_node(arg, "temp_type");
            int label_num = generate_unique_label();

            fprintf(output, "TYPE LF@%s LF@%s\n", temp_type_name, temp_var_name);
            fprintf(output, "JUMPIFEQ $write_null_%d LF@%s string@nil\n", label_num, temp_type_name);

            fprintf(output, "WRITE LF@%s\n", temp_var_name);
            fprintf(output, "JUMP $write_end_%d\n", label_num);

            fprintf(output, "LABEL $write_null_%d\n", label_num);
            fprintf(output, "WRITE string@null\n");
            fprintf(output, "LABEL $write_end_%d\n", label_num);
        } else {
            fprintf(output, "WRITE LF@%s\n", temp_var_name);
        }
    }
    else if (strcmp(node->name, "ifj.readi32") == 0 ||
             strcmp(node->name, "ifj.readf64") == 0 ||
             strcmp(node->name, "ifj.readstr") == 0)
    {
        char *retval_var = get_temp_var_name_for_node(node, "retval_var");
        const char *type = (strcmp(node->name, "ifj.readi32") == 0) ? "int" : (strcmp(node->name, "ifj.readf64") == 0) ? "float"
                                                                                                                       : "string";
        fprintf(output, "READ LF@%s %s\n", retval_var, type);
        fprintf(output, "PUSHS LF@%s\n", retval_var);
    }
    else if (strcmp(node->name, "ifj.length") == 0)
    {
        codegen_generate_expression(output, node->arguments[0], current_function);
        char *tmp_str_var = get_temp_var_name_for_node(node->arguments[0], "tmp_str_var");
        char *retval_var = get_temp_var_name_for_node(node, "retval_var");

        fprintf(output, "POPS LF@%s\n", tmp_str_var);
        fprintf(output, "STRLEN LF@%s LF@%s\n", retval_var, tmp_str_var);
        fprintf(output, "PUSHS LF@%s\n", retval_var);
    }
    else if (strcmp(node->name, "ifj.concat") == 0)
    {
        codegen_generate_expression(output, node->arguments[0], current_function);
        codegen_generate_expression(output, node->arguments[1], current_function);
        char *tmp_str1_var = get_temp_var_name_for_node(node->arguments[0], "tmp_str1_var");
        char *tmp_str2_var = get_temp_var_name_for_node(node->arguments[1], "tmp_str2_var");
        char *retval_var = get_temp_var_name_for_node(node, "retval_var");

        fprintf(output, "POPS LF@%s\n", tmp_str2_var);
        fprintf(output, "POPS LF@%s\n", tmp_str1_var);
        fprintf(output, "CONCAT LF@%s LF@%s LF@%s\n", retval_var, tmp_str1_var, tmp_str2_var);
        fprintf(output, "PUSHS LF@%s\n", retval_var);
    }
    else if (strcmp(node->name, "ifj.i2f") == 0)
    {
        codegen_generate_expression(output, node->arguments[0], current_function);
        char *tmp_var = get_temp_var_name_for_node(node->arguments[0], "tmp_var");
        char *retval_var = get_temp_var_name_for_node(node, "retval_var");

        fprintf(output, "POPS LF@%s\n", tmp_var);
        fprintf(output, "INT2FLOAT LF@%s LF@%s\n", retval_var, tmp_var);
        fprintf(output, "PUSHS LF@%s\n", retval_var);
    }
    else if (strcmp(node->name, "ifj.f2i") == 0)
    {
        codegen_generate_expression(output, node->arguments[0], current_function);
        char *tmp_var = get_temp_var_name_for_node(node->arguments[0], "tmp_var");
        char *retval_var = get_temp_var_name_for_node(node, "retval_var");

        fprintf(output, "POPS LF@%s\n", tmp_var);
        fprintf(output, "FLOAT2INT LF@%s LF@%s\n", retval_var, tmp_var);
        fprintf(output, "PUSHS LF@%s\n", retval_var);
    }
    else if (strcmp(node->name, "ifj.substring") == 0 ||
             strcmp(node->name, "ifj.strcmp") == 0 ||
             strcmp(node->name, "ifj.string") == 0)
    {
        // Handle substring, strcmp, and string functions
        for (int i = 0; i < node->arg_count; ++i)
        {
            codegen_generate_expression(output, node->arguments[i], current_function);
        }
        if (strcmp(node->name, "ifj.string") == 0)
        {
            fprintf(output, "CALL ifj-string\n");
        }
        else if (strcmp(node->name, "ifj.strcmp") == 0)
        {
            fprintf(output, "CALL ifj-strcmp\n");
        }
        else
        {
            fprintf(output, "CALL ifj-substring\n");
        }
    }
    else if (strcmp(node->name, "ifj.chr") == 0)
    {
        codegen_generate_expression(output, node->arguments[0], current_function);
        char *tmp_int_var = get_temp_var_name_for_node(node->arguments[0], "tmp_int_var");
        char *tmp_temp_var = get_temp_var_name_for_node(node, "tmp_temp_var");
        char *retval_var = get_temp_var_name_for_node(node, "retval_var");

        fprintf(output, "POPS LF@%s\n", tmp_int_var);
        // Ensure the integer is within valid range (0-255)
        fprintf(output, "IDIV LF@%s LF@%s int@256\n", tmp_temp_var, tmp_int_var);
        fprintf(output, "MUL LF@%s LF@%s int@256\n", tmp_temp_var, tmp_temp_var);
        fprintf(output, "SUB LF@%s LF@%s LF@%s\n", tmp_int_var, tmp_int_var, tmp_temp_var);
        fprintf(output, "INT2CHAR LF@%s LF@%s\n", retval_var, tmp_int_var);
        fprintf(output, "PUSHS LF@%s\n", retval_var);
    }
    else if (strcmp(node->name, "ifj.ord") == 0)
    {
        codegen_generate_expression(output, node->arguments[0], current_function); // string
        codegen_generate_expression(output, node->arguments[1], current_function); // index

        // Retrieve variable names using the same keys
        char *str_var = get_temp_var_name_for_node(node, "str_var");
        char *idx_var = get_temp_var_name_for_node(node, "idx_var");
        char *strlen_var = get_temp_var_name_for_node(node, "strlen_var");
        char *tmp_bool_var = get_temp_var_name_for_node(node, "tmp_bool_var");
        char *retval_var = get_temp_var_name_for_node(node, "retval_var");

        fprintf(output, "POPS LF@%s\n", idx_var);
        fprintf(output, "POPS LF@%s\n", str_var);
        fprintf(output, "STRLEN LF@%s LF@%s\n", strlen_var, str_var);
        fprintf(output, "LT LF@%s LF@%s int@0\n", tmp_bool_var, idx_var);
        fprintf(output, "JUMPIFEQ $ord_error_%d LF@%s bool@true\n", label_counter, tmp_bool_var);
        fprintf(output, "SUB LF@%s LF@%s int@1\n", strlen_var, strlen_var);
        fprintf(output, "GT LF@%s LF@%s LF@%s\n", tmp_bool_var, idx_var, strlen_var);
        fprintf(output, "JUMPIFEQ $ord_error_%d LF@%s bool@true\n", label_counter, tmp_bool_var);
        fprintf(output, "STRI2INT LF@%s LF@%s LF@%s\n", retval_var, str_var, idx_var);
        fprintf(output, "PUSHS LF@%s\n", retval_var);
        fprintf(output, "JUMP $ord_end_%d\n", label_counter);
        fprintf(output, "LABEL $ord_error_%d\n", label_counter);
        fprintf(output, "PUSHS int@0\n");
        fprintf(output, "LABEL $ord_end_%d\n", label_counter);
        label_counter++;
    }
    else
    {
        // User-defined function call
        // Push arguments onto the stack in reverse order
        for (int i = node->arg_count - 1; i >= 0; i--)
        {
            codegen_generate_expression(output, node->arguments[i], current_function);
        }
        // Call the function
        fprintf(output, "CALL %s\n", node->name);
        // If the function returns a value and it's assigned to a variable
        if (node->left)
        {
            fprintf(output, "POPS LF@%s\n", remove_last_prefix(node->left->name));
        }
    }
}

/**
 * Declares variables used in a block.
 */
void codegen_declare_variables_in_block(FILE *output, ASTNode *block_node) {
    ASTNode *current = block_node->body;
    while (current) {
        codegen_declare_variables_in_statement(output, current);
        current = current->next;
    }
}

/**
 * Declares variables used in a statement.
 */
void codegen_declare_variables_in_statement(FILE *output, ASTNode *node) {
    if (node == NULL) {
        return;
    }

    switch (node->type)
    {
    case NODE_VARIABLE_DECLARATION:
    {
        const char *var_name = remove_last_prefix(node->name);
        if (!is_variable_declared(var_name))
        {
            fprintf(output, "DEFVAR LF@%s\n", var_name);
            add_declared_variable(var_name);
        }
        break;
    }
    case NODE_ASSIGNMENT:
    {
        codegen_declare_variables_in_statement(output, node->left);
        break;
    }
    case NODE_FUNCTION_CALL:
    {
        // Recursively collect variables in arguments
        for (int i = 0; i < node->arg_count; ++i)
        {
            codegen_declare_variables_in_statement(output, node->arguments[i]);
        }
        break;
    }
    case NODE_BINARY_OPERATION:
    {
        // Recursively collect variables in left and right expressions
        codegen_declare_variables_in_statement(output, node->left);
        codegen_declare_variables_in_statement(output, node->right);
        break;
    }
    case NODE_IF:
    case NODE_WHILE:
    case NODE_BLOCK:
        codegen_declare_variables_in_block(output, node);
        break;
    case NODE_LITERAL:
    case NODE_IDENTIFIER:
        // No variables to declare
        break;
    default:
        if (node->left)
        {
            codegen_declare_variables_in_statement(output, node->left);
        }
        if (node->right)
        {
            codegen_declare_variables_in_statement(output, node->right);
        }
        break;
    }
}

/**
 * Generates code for a statement.
 */
void codegen_generate_statement(FILE *output, ASTNode *node, const char *current_function) {
    if (node == NULL) {
        error_exit(ERR_INTERNAL, "Invalid statement node for code generation\n");
    }

    switch (node->type)
    {
    case NODE_VARIABLE_DECLARATION:
        if (node->left != NULL)
        {
            codegen_generate_expression(output, node->left, current_function);
            const char *var_name = remove_last_prefix(node->name);
            if (var_name == NULL)
            {
                error_exit(ERR_INTERNAL, "Error: Variable name is NULL in VARIABLE_DECLARATION.\n");
            }
            fprintf(output, "POPS LF@%s\n", var_name);
        }
        break;

    case NODE_ASSIGNMENT:
        codegen_generate_expression(output, node->left, current_function);
        fprintf(output, "POPS LF@%s\n", remove_last_prefix(node->name));
        break;

    case NODE_RETURN:
        codegen_generate_return(output, node, current_function);
        break;

    case NODE_IF:
        codegen_generate_if(output, node);
        break;

    case NODE_WHILE:
        codegen_generate_while(output, node);
        break;

    case NODE_FUNCTION_CALL:
        codegen_generate_function_call(output, node, current_function);
        break;

    default:
        error_exit(ERR_INTERNAL, "Unsupported statement type for code generation\n");
        break;
    }
}

/**
 * Generates code for a variable declaration.
 */
void codegen_generate_variable_declaration(FILE *output, ASTNode *declaration_node) {
    if (!declaration_node || !declaration_node->name) {
        error_exit(ERR_INTERNAL, "Error: Invalid variable declaration.\n");
    }

    if (declaration_node->left)
    {
        codegen_generate_expression(output, declaration_node->left, NULL);
        fprintf(output, "POPS LF@%s\n", remove_last_prefix(declaration_node->name));
    }
}

/**
 * Generates code for an assignment statement.
 */
void codegen_generate_assignment(FILE *output, ASTNode *assignment_node) {
    codegen_generate_expression(output, assignment_node->left, assignment_node->name);
    fprintf(output, "POPS LF@%s\n", remove_last_prefix(assignment_node->name));
}

/**
 * Generates code for a return statement.
 */
void codegen_generate_return(FILE *output, ASTNode *return_node, const char *current_function) {
    if (return_node->left) {
        codegen_generate_expression(output, return_node->left, current_function);
        // The return value is now on the stack
    }
    fprintf(output, "POPFRAME\n");
    fprintf(output, "RETURN\n");
}

/**
 * Generates code for an if statement.
 */
void codegen_generate_if(FILE *output, ASTNode *if_node) {
    static int if_label_count = 0;

    int current_label = if_label_count++;

    if (if_node->condition->type == NODE_IDENTIFIER && is_nullable(if_node->condition->data_type))
    {
        fprintf(output, "TYPE LF@%%tmp_type LF@%s\n", remove_last_prefix(if_node->condition->name));
        fprintf(output, "JUMPIFEQ $else_%d LF@%%tmp_type string@nil\n", current_label);
    }
    else
    {
        codegen_generate_expression(output, if_node->condition, if_node->name);

        fprintf(output, "PUSHS bool@false\n");
        fprintf(output, "JUMPIFEQS $else_%d\n", current_label);
    }

    codegen_generate_block(output, if_node->body, if_node->name);
    fprintf(output, "JUMP $endif_%d\n", current_label);

    fprintf(output, "LABEL $else_%d\n", current_label);
    if (if_node->left != NULL) {
        codegen_generate_block(output, if_node->left, if_node->name);
    }

    fprintf(output, "LABEL $endif_%d\n", current_label);
}

/**
 * Generates code for a while loop.
 */
void codegen_generate_while(FILE *output, ASTNode *while_node) {
    int label_num = generate_unique_label();
    fprintf(output, "LABEL $while_start_%d\n", label_num);

    codegen_generate_expression(output, while_node->condition, while_node->name);

    fprintf(output, "PUSHS bool@false\n");
    fprintf(output, "JUMPIFEQS $while_end_%d\n", label_num);

    codegen_generate_block(output, while_node->body, while_node->name);

    fprintf(output, "JUMP $while_start_%d\n", label_num);
    fprintf(output, "LABEL $while_end_%d\n", label_num);
}

/**
 * Escapes special characters in a string for IFJcode24.
 */
char *escape_ifj24_string(const char *input) {
    size_t length = strlen(input);
    size_t buffer_size = length * 4 + 1; // Reserve space for escaping
    char *escaped_string = safe_malloc(buffer_size);
    size_t index = 0;
    for (size_t i = 0; i < length; i++) {
        unsigned char c = input[i];

        if (c <= 32 || c == 35 || c == 92 || c >= 127) { // ASCII 0-32, 35 (#), 92 (\), 127+ (non-printable)
            index += snprintf(escaped_string + index, buffer_size - index, "\\%03d", c);
        } else {
            escaped_string[index++] = c;
        }
    }

    escaped_string[index] = '\0';
    return escaped_string;
}
