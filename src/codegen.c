#include "codegen.h"
#include "ast.h"
#include "symtable.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "scanner.h"
#include <stdbool.h>
#include "utils.h"

typedef struct {
    bool uses_substring;
    bool uses_strcmp;
    bool uses_string;
} BuiltinFunctionUsage;

typedef struct TempVarMapEntry {
    ASTNode *node;
    int temp_var_number;
    struct TempVarMapEntry *next;
} TempVarMapEntry;

static TempVarMapEntry *temp_var_map = NULL;

static int temp_var_counter = 0;

static BuiltinFunctionUsage builtin_function_usage = {false, false, false};

#undef DEBUG_PARSER
#ifdef DEBUG_PARSER
#define LOG(...) fprintf(__VA_ARGS__)
#else
#define LOG(...)
#endif
// Global variable for storing the output file
static FILE *output_file;

typedef struct DeclaredVar {
    char *var_name;
    struct DeclaredVar *next;
} DeclaredVar;

DeclaredVar *declared_vars = NULL;

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

void add_declared_variable(const char *var_name) {
    DeclaredVar *new_var = malloc(sizeof(DeclaredVar));
    new_var->var_name = string_duplicate(var_name);
    new_var->next = declared_vars;
    declared_vars = new_var;
}

void reset_declared_variables() {
    DeclaredVar *current = declared_vars;
    while (current) {
        DeclaredVar *next = current->next;
        free(current->var_name);
        free(current);
        current = next;
    }
    declared_vars = NULL;
}

void reset_temp_var_map() {
    TempVarMapEntry *entry = temp_var_map;
    while (entry != NULL) {
        TempVarMapEntry *next = entry->next;
        free(entry);
        entry = next;
    }
    temp_var_map = NULL;
}

int get_temp_var_number_for_node(ASTNode *node) {
    TempVarMapEntry *entry = temp_var_map;
    while (entry != NULL) {
        if (entry->node == node) {
            return entry->temp_var_number;
        }
        entry = entry->next;
    }
    // Узел не найден, создаём новый номер
    int temp_var_number = temp_var_counter++;
    TempVarMapEntry *new_entry = malloc(sizeof(TempVarMapEntry));
    new_entry->node = node;
    new_entry->temp_var_number = temp_var_number;
    new_entry->next = temp_var_map;
    temp_var_map = new_entry;
    return temp_var_number;
}


void codegen_generate_function(ASTNode *function_node);
void codegen_generate_block(FILE *output, ASTNode *block_node, const char *current_function);
void codegen_generate_statement(FILE *output, ASTNode *statement_node, const char *current_function);
void codegen_generate_expression(FILE *output, ASTNode *node, const char *current_function);
void codegen_generate_function_call(FILE *output, ASTNode *node, const char *current_function);
void codegen_generate_variable_declaration(FILE *output, ASTNode *declaration_node);
void codegen_generate_assignment(FILE *output, ASTNode *assignment_node);
void codegen_generate_return(FILE *output, ASTNode *return_node);
void codegen_generate_if(FILE *output, ASTNode *if_node);
void codegen_generate_while(FILE *output, ASTNode *while_node);
void codegen_generate_builtin_functions();
void codegen_declare_variables_in_statement(FILE *output, ASTNode *node);
void codegen_declare_variables_in_block(FILE *output, ASTNode *block_node);


int get_next_temp_var() {
    return temp_var_counter++;
}
char* generate_unique_var_name(const char* base_name) {
    static int unique_var_counter = 0;
    unique_var_counter++;
    char* var_name = malloc(64); // Не забудьте освободить память после использования
    snprintf(var_name, 64, "%%-%s-%d", base_name, unique_var_counter);
    return var_name;
}


void collect_builtin_function_usage(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_PROGRAM:
            // Обходим все функции программы
            for (ASTNode *func = node->body; func != NULL; func = func->next) {
                collect_builtin_function_usage(func);
            }
            break;

        case NODE_FUNCTION:
            // Обходим параметры функции (если нужно)
            // Обходим тело функции
            collect_builtin_function_usage(node->body);
            break;

        case NODE_BLOCK:
            // Обходим все операторы в блоке
            for (ASTNode *stmt = node->body; stmt != NULL; stmt = stmt->next) {
                collect_builtin_function_usage(stmt);
            }
            break;

        case NODE_FUNCTION_CALL:
            if (strcmp(node->name, "ifj.substring") == 0) {
                builtin_function_usage.uses_substring = true;
            } else if (strcmp(node->name, "ifj.strcmp") == 0) {
                builtin_function_usage.uses_strcmp = true;
            } else if (strcmp(node->name, "ifj.string") == 0) {
                builtin_function_usage.uses_string = true;
            }
            // Обходим аргументы функции
            for (int i = 0; i < node->arg_count; ++i) {
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
            if (node->left) {
                collect_builtin_function_usage(node->left); // ветка else
            }
            break;

        case NODE_WHILE:
            collect_builtin_function_usage(node->condition);
            collect_builtin_function_usage(node->body);
            break;

        case NODE_RETURN:
            if (node->left) {
                collect_builtin_function_usage(node->left);
            }
            break;

        case NODE_LITERAL:
        case NODE_IDENTIFIER:
            // Ничего не делаем
            break;

        default:
            fprintf(stderr, "Unsupported node type in collect_builtin_function_usage: %d\n", node->type);
            exit(1);
    }
}


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
        "RETURN\n"
    );
}

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
        "RETURN\n"
    );
}

void codegen_generate_ifj_string_function() {
    fprintf(output_file,
        "LABEL ifj-string\n"
        "CREATEFRAME\n"
        "PUSHFRAME\n"
        "DEFVAR LF@str_literal\n"
        "POPS LF@str_literal\n"
        "PUSHS LF@str_literal\n"
        "POPFRAME\n"
        "RETURN\n"
    );
}


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


const char* remove_last_prefix(const char* name) {
    static char buffer[1024];
    if (!name) {
        return NULL;
    }

    const char* last_dot = strrchr(name, '.');
    if (last_dot && *(last_dot + 1) != '\0') {
        size_t new_len = last_dot - name;
        if (new_len >= sizeof(buffer)) {
            fprintf(stderr, "Error: Buffer overflow in remove_last_suffix.\n");
            exit(EXIT_FAILURE);
        }
        strncpy(buffer, name, new_len); 
        buffer[new_len] = '\0';         
        return buffer;
    }
    return name; 
}



/*void collect_functions(ASTNode *program_node) {
    ASTNode *current_function = program_node->body;

    while (current_function) {
        if (current_function->type == NODE_FUNCTION) {
            function_count++;
            functions = realloc(functions, function_count * sizeof(FunctionInfo));
            functions[function_count - 1].name = string_duplicate(current_function->name);
            functions[function_count - 1].param_count = current_function->param_count;

            if (current_function->param_count > 0) {
                functions[function_count - 1].parameters = malloc(current_function->param_count * sizeof(char *));
                for (int i = 0; i < current_function->param_count; i++) {
                    functions[function_count - 1].parameters[i] = strdup(current_function->parameters[i]->name);
                }
            } else {
                functions[function_count - 1].parameters = NULL;
            }
        }
        current_function = current_function->next;
    }
}*/

char *escape_ifj24_string(const char *input);
static int label_counter = 0;

int generate_unique_label() {
    return label_counter++;
}

// Initialize code generator
void codegen_init(const char *filename) {
    if (filename) {
        output_file = fopen(filename, "w");
        if (!output_file) {
            fprintf(stderr, "Error: Cannot open output file %s for writing.\n", filename);
            exit(EXIT_FAILURE);
        }
    } else {
        output_file = stdout; 
    }
}


// Finalize code generation
void codegen_finalize() {
    if (output_file) {
        fclose(output_file);
        output_file = NULL;
    }
}

void codegen_generate_program(ASTNode *program_node) {
    if (!program_node || program_node->type != NODE_PROGRAM) {
        LOG(stderr, "DEBUG: Invalid program node.\n");
        return;
    }

    LOG(stderr, "DEBUG: Starting code generation for program.\n");

    // Сначала собираем информацию об использовании встроенных функций
    collect_builtin_function_usage(program_node);

    // Генерируем заголовок
    fprintf(output_file, ".IFJcode24\n");

    // Генерируем основной код
    fprintf(output_file, "CALL main\n");
    fprintf(output_file, "EXIT int@0\n");

    // Генерируем функции пользователя
    ASTNode *current_function = program_node->body;

    while (current_function) {
        if (current_function->type == NODE_FUNCTION) {
            LOG(stderr, "DEBUG: Found function '%s'. Generating its code.\n", current_function->name);
            codegen_generate_function(current_function);
        }
        current_function = current_function->next;
    }

    // Генерируем реализации встроенных функций, если они используются
    codegen_generate_builtin_functions();
}




void codegen_generate_function(ASTNode *function) {
    temp_var_counter = 0; // Сбрасываем счётчик временных переменных
    reset_temp_var_map(); // Сбрасываем отображение
    reset_declared_variables(); // Сбрасываем список объявленных переменных
    fprintf(output_file, "LABEL %s\n", function->name);
    fprintf(output_file, "CREATEFRAME\n");
    fprintf(output_file, "PUSHFRAME\n");

    // Объявляем переменные для параметров функции
    for (int i = 0; i < function->param_count; i++) {
        fprintf(output_file, "DEFVAR LF@%s\n", remove_last_prefix(function->parameters[i]->name));
        fprintf(output_file, "POPS LF@%s\n", remove_last_prefix(function->parameters[i]->name));
    }

    // Объявляем временные переменные
    fprintf(output_file, "DEFVAR LF@%%tmp_type\n");
    fprintf(output_file, "DEFVAR LF@%%tmp_var\n");
    fprintf(output_file, "DEFVAR LF@%%tmp_bool\n");

    // Объявляем все переменные, используемые в функции
    codegen_declare_variables_in_block(output_file, function->body);

    // Генерируем тело функции
    codegen_generate_block(output_file, function->body, function->name);

    fprintf(output_file, "POPFRAME\n");
    fprintf(output_file, "RETURN\n");
}


// Function to generate code for a block of statements
void codegen_generate_block(FILE *output, ASTNode *block_node, const char *current_function) {
    ASTNode *current = block_node->body;
    output = output;
    while (current) {
        codegen_generate_statement(output_file, current, current_function);
        current = current->next;
    }
}

void codegen_generate_expression(FILE *output, ASTNode *node, const char *current_function) {
    if (node == NULL) {
        LOG(stderr, "Node is NULL\n");
        return;
    }
    LOG(stderr, "Node type: %d\n", node->type);

    switch (node->type) {
        case NODE_LITERAL:
            if (node->data_type == TYPE_INT) {
                fprintf(output, "PUSHS int@%s\n", node->value);
            } else if (node->data_type == TYPE_FLOAT) {
                double float_value = atof(node->value);
                fprintf(output, "PUSHS float@%.13a\n", float_value);
            } else if (node->data_type == TYPE_U8) {
                char *escaped_value = escape_ifj24_string(node->value);
                fprintf(output, "PUSHS string@%s\n", escaped_value);
                free(escaped_value);
            } else if (node->data_type == TYPE_NULL) {
                fprintf(output, "PUSHS nil@nil\n");
            } else if (node->data_type == TYPE_BOOL) {
                fprintf(output, "PUSHS bool@%s\n", strcmp(node->value, "true") == 0 ? "true" : "false");
            }
            break;

        case NODE_IDENTIFIER:
        {
            if (strcmp(node->name, "nil") == 0) {
                fprintf(output, "PUSHS nil@nil\n"); 
                fprintf(output, "EQS\n");           
                fprintf(output, "NOTS\n");          
            } else if (is_nullable(node->data_type)) {
                fprintf(output, "TYPE LF@%%tmp_type LF@%s\n", remove_last_prefix(node->name));
                fprintf(output, "PUSHS LF@%%tmp_type\n");
                fprintf(output, "PUSHS string@nil\n");
                fprintf(output, "EQS\n");
                fprintf(output, "NOTS\n");
            } else if (strcmp(node->name, "true") == 0) {
                fprintf(output, "PUSHS bool@true\n");
            } else if (strcmp(node->name, "false") == 0) {
                fprintf(output, "PUSHS bool@false\n");
            } else {
                LOG(stderr, "175");
                fprintf(output, "PUSHS LF@%s\n", remove_last_prefix(node->name));
            }
        }
            break;


        case NODE_BINARY_OPERATION:
            codegen_generate_expression(output, node->left, node->name);
            codegen_generate_expression(output, node->right, node->name);
            if (strcmp(node->name, "+") == 0) {
                fprintf(output, "ADDS\n");
            } else if (strcmp(node->name, "-") == 0) {
                fprintf(output, "SUBS\n");
            } else if (strcmp(node->name, "*") == 0) {
                fprintf(output, "MULS\n");
            } else if (strcmp(node->name, "/") == 0) {
                fprintf(output, "DIVS\n");
            } else if (strcmp(node->name, "<") == 0) {
                fprintf(output, "LTS\n");
            } else if (strcmp(node->name, "<=") == 0) {
                fprintf(output, "GTS\n");
                fprintf(output, "NOTS\n");
            } else if (strcmp(node->name, ">") == 0) {
                fprintf(output, "GTS\n");
            } else if (strcmp(node->name, ">=") == 0) {
                fprintf(output, "LTS\n");
                fprintf(output, "NOTS\n");
            } else if (strcmp(node->name, "==") == 0) {
                fprintf(output, "EQS\n");
            } else if (strcmp(node->name, "!=") == 0) {
                fprintf(output, "EQS\n");
                fprintf(output, "NOTS\n");
            }
            break;

        case NODE_FUNCTION_CALL:
            codegen_generate_function_call(output, node, current_function);
            break;

        default:
            fprintf(stderr, "Unsupported expression type for code generation, type: %d, name: %s\n", node->type, node->name);
            exit(1);
    }
}


void codegen_generate_function_call(FILE *output, ASTNode *node, const char *current_function) {
    if (node == NULL || node->type != NODE_FUNCTION_CALL) {
        fprintf(stderr, "Invalid function call node for code generation\n");
        exit(1);
    }
    if (strcmp(node->name, "ifj.write") == 0) {
        ASTNode *arg = node->arguments[0];
        int temp_var_number = get_temp_var_number_for_node(node);
        codegen_generate_expression(output, arg, current_function);
        fprintf(output, "POPS LF@%%tmp_var_%d\n", temp_var_number);
        if (is_nullable(arg->data_type)) {
            fprintf(output, "TYPE LF@%%tmp_type_%d LF@%%tmp_var_%d\n", temp_var_number, temp_var_number);
            fprintf(output, "JUMPIFEQ $write_null_%d LF@%%tmp_type_%d string@nil\n", temp_var_number, temp_var_number);
            // Если не nil, выводим значение
            fprintf(output, "WRITE LF@%%tmp_var_%d\n", temp_var_number);
            fprintf(output, "JUMP $write_end_%d\n", temp_var_number);
            // Если nil, выводим 'null'
            fprintf(output, "LABEL $write_null_%d\n", temp_var_number);
            fprintf(output, "WRITE string@null\n");
            fprintf(output, "LABEL $write_end_%d\n", temp_var_number);
        } else {
            // Если тип не nullable, просто выводим значение
            fprintf(output, "WRITE LF@%%tmp_var_%d\n", temp_var_number);
        }
    } else if (strcmp(node->name, "ifj.readi32") == 0) {
        char* retval_var = generate_unique_var_name("retval");
        fprintf(output, "DEFVAR LF@%s\n", retval_var);
        fprintf(output, "READ LF@%s int\n", retval_var);
        fprintf(output, "PUSHS LF@%s\n", retval_var);
        free(retval_var);
    } else if (strcmp(node->name, "ifj.readf64") == 0) {
        char* retval_var = generate_unique_var_name("retval");
        fprintf(output, "DEFVAR LF@%s\n", retval_var);
        fprintf(output, "READ LF@%s float\n", retval_var);
        fprintf(output, "PUSHS LF@%s\n", retval_var);
        free(retval_var);
    } else if (strcmp(node->name, "ifj.readstr") == 0) {
        char* retval_var = generate_unique_var_name("retval");
        fprintf(output, "DEFVAR LF@%s\n", retval_var);
        fprintf(output, "READ LF@%s string\n", retval_var);
        fprintf(output, "PUSHS LF@%s\n", retval_var);
        free(retval_var);
    } else if (strcmp(node->name, "ifj.length") == 0) {
        codegen_generate_expression(output, node->arguments[0], current_function);
        char* tmp_str_var = generate_unique_var_name("tmp-str");
        char* retval_var = generate_unique_var_name("retval");
        fprintf(output, "DEFVAR LF@%s\n", tmp_str_var);
        fprintf(output, "POPS LF@%s\n", tmp_str_var);
        fprintf(output, "DEFVAR LF@%s\n", retval_var);
        fprintf(output, "STRLEN LF@%s LF@%s\n", retval_var, tmp_str_var);
        fprintf(output, "PUSHS LF@%s\n", retval_var);
        free(tmp_str_var);
        free(retval_var);
    } else if (strcmp(node->name, "ifj.concat") == 0) {
        codegen_generate_expression(output, node->arguments[0], current_function);
        codegen_generate_expression(output, node->arguments[1], current_function);
        char* tmp_str1_var = generate_unique_var_name("tmp-str1");
        char* tmp_str2_var = generate_unique_var_name("tmp-str2");
        char* retval_var = generate_unique_var_name("retval");
        fprintf(output, "DEFVAR LF@%s\n", tmp_str2_var);
        fprintf(output, "POPS LF@%s\n", tmp_str2_var);
        fprintf(output, "DEFVAR LF@%s\n", tmp_str1_var);
        fprintf(output, "POPS LF@%s\n", tmp_str1_var);
        fprintf(output, "DEFVAR LF@%s\n", retval_var);
        fprintf(output, "CONCAT LF@%s LF@%s LF@%s\n", retval_var, tmp_str1_var, tmp_str2_var);
        fprintf(output, "PUSHS LF@%s\n", retval_var);
        free(tmp_str1_var);
        free(tmp_str2_var);
        free(retval_var);
    } else if (strcmp(node->name, "ifj.i2f") == 0) {
        codegen_generate_expression(output, node->arguments[0], current_function);
        char* tmp_int_var = generate_unique_var_name("tmp-int");
        char* retval_var = generate_unique_var_name("retval");
        fprintf(output, "DEFVAR LF@%s\n", tmp_int_var);
        fprintf(output, "POPS LF@%s\n", tmp_int_var);
        fprintf(output, "DEFVAR LF@%s\n", retval_var);
        fprintf(output, "INT2FLOAT LF@%s LF@%s\n", retval_var, tmp_int_var);
        fprintf(output, "PUSHS LF@%s\n", retval_var);
        free(tmp_int_var);
        free(retval_var);
    } else if (strcmp(node->name, "ifj.f2i") == 0) {
        codegen_generate_expression(output, node->arguments[0], current_function);
        char* tmp_float_var = generate_unique_var_name("tmp-float");
        char* retval_var = generate_unique_var_name("retval");
        fprintf(output, "DEFVAR LF@%s\n", tmp_float_var);
        fprintf(output, "POPS LF@%s\n", tmp_float_var);
        fprintf(output, "DEFVAR LF@%s\n", retval_var);
        fprintf(output, "FLOAT2INT LF@%s LF@%s\n", retval_var, tmp_float_var);
        fprintf(output, "PUSHS LF@%s\n", retval_var);
        free(tmp_float_var);
        free(retval_var);
    } else if (strcmp(node->name, "ifj.substring") == 0) {
        // Код для ifj.substring остаётся без изменений, если временные переменные уже уникальны
        // Если нет, то нужно аналогично применить generate_unique_var_name
        builtin_function_usage.uses_substring = true;
        codegen_generate_expression(output, node->arguments[0], current_function); // строка
        codegen_generate_expression(output, node->arguments[1], current_function); // начальный индекс
        codegen_generate_expression(output, node->arguments[2], current_function); // конечный индекс
        fprintf(output, "CALL ifj-substring\n");
    } else if (strcmp(node->name, "ifj.ord") == 0) {
        codegen_generate_expression(output, node->arguments[0], current_function); // строка
        codegen_generate_expression(output, node->arguments[1], current_function); // индекс
        // Объявление и использование временных переменных с уникальными именами
        char* str_var = generate_unique_var_name("str");
        char* idx_var = generate_unique_var_name("idx");
        char* strlen_var = generate_unique_var_name("strlen");
        char* tmp_bool_var = generate_unique_var_name("tmp-bool");
        char* retval_var = generate_unique_var_name("retval");
        fprintf(output, "DEFVAR LF@%s\n", idx_var);
        fprintf(output, "POPS LF@%s\n", idx_var);
        fprintf(output, "DEFVAR LF@%s\n", str_var);
        fprintf(output, "POPS LF@%s\n", str_var);
        fprintf(output, "DEFVAR LF@%s\n", strlen_var);
        fprintf(output, "STRLEN LF@%s LF@%s\n", strlen_var, str_var);
        fprintf(output, "DEFVAR LF@%s\n", tmp_bool_var);
        fprintf(output, "LT LF@%s LF@%s int@0\n", tmp_bool_var, idx_var);
        fprintf(output, "JUMPIFEQ $ord_error LF@%s bool@true\n", tmp_bool_var);
        fprintf(output, "SUB LF@%s LF@%s int@1\n", strlen_var, strlen_var);
        fprintf(output, "GT LF@%s LF@%s LF@%s\n", tmp_bool_var, idx_var, strlen_var);
        fprintf(output, "JUMPIFEQ $ord_error LF@%s bool@true\n", tmp_bool_var);
        fprintf(output, "DEFVAR LF@%s\n", retval_var);
        fprintf(output, "STRI2INT LF@%s LF@%s LF@%s\n", retval_var, str_var, idx_var);
        fprintf(output, "PUSHS LF@%s\n", retval_var);
        fprintf(output, "JUMP $ord_end\n");
        fprintf(output, "LABEL $ord_error\n");
        fprintf(output, "PUSHS int@0\n");
        fprintf(output, "LABEL $ord_end\n");
    } else if (strcmp(node->name, "ifj.strcmp") == 0) {
        builtin_function_usage.uses_strcmp = true;
        codegen_generate_expression(output, node->arguments[0], current_function); // строка 1
        codegen_generate_expression(output, node->arguments[1], current_function); // строка 2
        fprintf(output, "CALL ifj-strcmp\n");
    } else if (strcmp(node->name, "ifj.string") == 0) {
        builtin_function_usage.uses_string = true;
        codegen_generate_expression(output, node->arguments[0], current_function); // строка 1
        fprintf(output, "CALL ifj-string\n");
    } else if (strcmp(node->name, "ifj.chr") == 0) {
        codegen_generate_expression(output, node->arguments[0], current_function);
        char* tmp_int_var = generate_unique_var_name("tmp-int");
        char* tmp_temp_var = generate_unique_var_name("tmp-temp");
        char* retval_var = generate_unique_var_name("retval");
        fprintf(output, "DEFVAR LF@%s\n", tmp_int_var);
        fprintf(output, "POPS LF@%s\n", tmp_int_var);
        fprintf(output, "DEFVAR LF@%s\n", tmp_temp_var);
        fprintf(output, "DEFVAR LF@%s\n", retval_var);
        fprintf(output, "DIV LF@%s LF@%s int@256\n", tmp_temp_var, tmp_int_var);
        fprintf(output, "MUL LF@%s LF@%s int@256\n", tmp_temp_var, tmp_temp_var);
        fprintf(output, "SUB LF@%s LF@%s LF@%s\n", tmp_int_var, tmp_int_var, tmp_temp_var);
        fprintf(output, "INT2CHAR LF@%s LF@%s\n", retval_var, tmp_int_var);
        fprintf(output, "PUSHS LF@%s\n", retval_var);
        free(tmp_int_var);
        free(tmp_temp_var);
        free(retval_var);
    } else {
        // Обработка пользовательских функций
        for (int i = 0; i < node->arg_count; ++i) {
            codegen_generate_expression(output, node->arguments[i], current_function);
        }
        fprintf(output, "CALL %s\n", node->name);
        if (node->left) {
            fprintf(output, "POPS LF@%s\n", remove_last_prefix(node->left->name));
        }
    }
}

void codegen_declare_variables_in_block(FILE *output, ASTNode *block_node) {
    ASTNode *current = block_node->body;
    while (current) {
        codegen_declare_variables_in_statement(output, current);
        current = current->next;
    }
}


void codegen_declare_variables_in_statement(FILE *output, ASTNode *node) {
    if (node == NULL) {
        return;
    }

    switch (node->type) {
        case NODE_VARIABLE_DECLARATION: {
            const char *var_name = remove_last_prefix(node->name);
            if (!is_variable_declared(var_name)) {
                fprintf(output, "DEFVAR LF@%s\n", var_name);
                add_declared_variable(var_name);
            }
            break;
        }

        case NODE_FUNCTION_CALL: {
            // Обрабатываем аргументы функции
            for (int i = 0; i < node->arg_count; ++i) {
                codegen_declare_variables_in_statement(output, node->arguments[i]);
            }

            if (strcmp(node->name, "ifj.write") == 0) {
                int temp_var_number = get_temp_var_number_for_node(node);
                char temp_var_name[32];
                snprintf(temp_var_name, sizeof(temp_var_name), "%%tmp_var_%d", temp_var_number);

                if (!is_variable_declared(temp_var_name)) {
                    fprintf(output, "DEFVAR LF@%s\n", temp_var_name);
                    add_declared_variable(temp_var_name);
                }

                if (is_nullable(node->arguments[0]->data_type)) {
                    snprintf(temp_var_name, sizeof(temp_var_name), "%%tmp_type_%d", temp_var_number);
                    if (!is_variable_declared(temp_var_name)) {
                        fprintf(output, "DEFVAR LF@%s\n", temp_var_name);
                        add_declared_variable(temp_var_name);
                    }
                }
            }
            break;
        }
        case NODE_IF:
        case NODE_WHILE:
        case NODE_BLOCK:
            // Рекурсивно обходим дочерние узлы
            codegen_declare_variables_in_block(output, node->body);
            if (node->left) {
                codegen_declare_variables_in_block(output, node->left);
            }
            break;

        // Добавьте другие случаи при необходимости

        default:
            // Рекурсивно обходим дочерние узлы, если они есть
            if (node->left) {
                codegen_declare_variables_in_statement(output, node->left);
            }
            if (node->right) {
                codegen_declare_variables_in_statement(output, node->right);
            }
            break;
    }
}


void codegen_generate_statement(FILE *output, ASTNode *node, const char *current_function) {
    if (node == NULL) {
        fprintf(stderr, "Invalid statement node for code generation\n");
        exit(1);
    }

    switch (node->type) {
        case NODE_VARIABLE_DECLARATION:
            if (node->left != NULL) {
                codegen_generate_expression(output, node->left, current_function);
                fprintf(output, "POPS LF@%s\n", remove_last_prefix(node->name));
            }
            break;

        case NODE_ASSIGNMENT:
            codegen_generate_expression(output, node->left, current_function);
            fprintf(output, "POPS LF@%s\n", remove_last_prefix(node->name));

            break;

        case NODE_RETURN:
            // Generate code for return statement
            if (node->left != NULL) {
                codegen_generate_expression(output, node->left, node->name);
                //fprintf(output, "POPS LF@%%retval\n");
            }
            //fprintf(output, "RETURN\n");
            break;

        case NODE_IF:
            // Generate code for if statement
        {
            LOG(stderr, "DEBUG: Entering NODE_IF case\n");
            codegen_generate_if(output, node);
        }
            break;

        case NODE_WHILE:
            // Generate code for while loop
        {
            int label_num = generate_unique_label();
            fprintf(output, "LABEL $while_start_%d\n", label_num);
            codegen_generate_expression(output, node->condition, node->name);
            fprintf(output, "PUSHS bool@true\n");
            fprintf(output, "JUMPIFNEQS $while_end_%d\n", label_num);
            codegen_generate_block(output, node->body, node->name); // Генерация тела цикла
            fprintf(output, "JUMP $while_start_%d\n", label_num);
            fprintf(output, "LABEL $while_end_%d\n", label_num);
        }
            break;

        case NODE_FUNCTION_CALL:
        {
            codegen_generate_function_call(output, node, node->name);
        }
            break;

        default:
            fprintf(stderr, "Unsupported statement type for code generation\n");
            exit(1);
    }
}

void codegen_generate_variable_declaration(FILE *output, ASTNode *declaration_node) {
    if (!declaration_node || !declaration_node->name) {
        fprintf(stderr, "Error: Invalid variable declaration.\n");
        exit(1);
    }

    if (declaration_node->left) {
        codegen_generate_expression(output, declaration_node->left, NULL);
        fprintf(output, "POPS LF@%s\n", remove_last_prefix(declaration_node->name));
    }
}

void codegen_generate_assignment(FILE *output, ASTNode *assignment_node) {
    codegen_generate_expression(output, assignment_node->left, assignment_node->name);
    fprintf(output, "POPS LF@%s\n", remove_last_prefix(assignment_node->name));
}

void codegen_generate_return(FILE *output, ASTNode *return_node) {
    if (return_node->left) { 
        codegen_generate_expression(output, return_node->left, return_node->name);
        fprintf(output, "POPS LF@%%retval\n");
    }
    fprintf(output, "RETURN\n");
}

void codegen_generate_if(FILE *output, ASTNode *if_node) {
    static int if_label_count = 0;

    int current_label = if_label_count++;

    if (if_node->condition->type == NODE_IDENTIFIER && is_nullable(if_node->condition->data_type)) {
        int temp_var_number = get_temp_var_number_for_node(if_node->condition);
        fprintf(output, "TYPE LF@%%tmp_type LF@%s\n",  remove_last_prefix(if_node->condition->name));
        fprintf(output, "JUMPIFEQ $else_%d LF@%%tmp_type string@nil\n", current_label);
    } else {
        // Генерация условия
        codegen_generate_expression(output, if_node->condition, if_node->name);

        // Сравнение результата с false
        fprintf(output, "PUSHS bool@false\n");
        fprintf(output, "JUMPIFEQS $else_%d\n", current_label);
    }

    // Генерация тела 'if'
    codegen_generate_block(output, if_node->body, if_node->name);
    fprintf(output, "JUMP $endif_%d\n", current_label);

    // Блок 'else'
    fprintf(output, "LABEL $else_%d\n", current_label);
    if (if_node->left != NULL) {
        codegen_generate_block(output, if_node->left, if_node->name);
    }

    fprintf(output, "LABEL $endif_%d\n", current_label);
}





void codegen_generate_while(FILE *output, ASTNode *while_node) {
    static int while_label_count = 0;
    int current_label = while_label_count++;
    fprintf(output, "LABEL $while_start_%d\n", current_label);

    // Генерация условия
    codegen_generate_expression(output, while_node->condition, while_node->name);

    // Проверка результата условия
    fprintf(output, "PUSHS bool@false\n");
    fprintf(output, "JUMPIFEQS $while_end_%d\n", current_label);

    // Генерация тела цикла
    codegen_generate_block(output, while_node->body, while_node->name);

    fprintf(output, "JUMP $while_start_%d\n", current_label);
    fprintf(output, "LABEL $while_end_%d\n", current_label);
}


char *escape_ifj24_string(const char *input) {
    size_t length = strlen(input);
    size_t buffer_size = length * 4 + 1; // reserve space for escaping
    char *escaped_string = malloc(buffer_size);
    if (!escaped_string) {
        fprintf(stderr, "Memory allocation failed for escaped string.\n");
        exit(EXIT_FAILURE);
    }

    size_t index = 0;
    for (size_t i = 0; i < length; i++) {
        unsigned char c = input[i];

        // Escape non-printable characters and specific characters required by IFJcode24
        if (c <= 32 || c == 35 || c == 92 || c >= 127) { // ASCII 0-32, 35 (#), 92 (\), 127+ (non-printable)
            index += snprintf(escaped_string + index, buffer_size - index, "\\%03d", c);
        } else {
            escaped_string[index++] = c;
        }
    }

    escaped_string[index] = '\0';
    return escaped_string;
}

