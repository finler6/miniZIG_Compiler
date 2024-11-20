#include "codegen.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static FILE *output_file;
static int label_counter = 0;

void collect_functions(ASTNode *program_node) {
    ASTNode *current_function = program_node->body;

    // Проходим по дереву, собираем функции
    while (current_function) {
        if (current_function->type == NODE_FUNCTION) {
            function_count++;
            functions = realloc(functions, function_count * sizeof(FunctionInfo));
            functions[function_count - 1].name = strdup(current_function->name);
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
}

void codegen_init(const char *filename) {
    output_file = fopen(filename, "w");
    if (!output_file) {
        fprintf(stderr, "Error: Cannot open file %s for writing.\n", filename);
        exit(EXIT_FAILURE);
    }
    fprintf(output_file, ".IFJcode24\n");
}

void codegen_finalize() {
    if (output_file) {
        fclose(output_file);
        output_file = NULL;
    }
}

// Генерация уникальных меток
int generate_label() {
    return label_counter++;
}

void codegen_generate_program(ASTNode *program_node) {
    if (!program_node || program_node->type != NODE_PROGRAM) {
        fprintf(stderr, "DEBUG: Invalid program node.\n");
        return;
    }

    fprintf(stderr, "DEBUG: Starting code generation for program.\n");

    ASTNode *current_function = program_node->body;

    // Сначала находим main и генерируем её
    while (current_function) {
        if (current_function->type == NODE_FUNCTION && strcmp(current_function->name, "main") == 0) {
            fprintf(stderr, "DEBUG: Found main function. Generating its code.\n");
            codegen_generate_function(current_function);
            break;
        }
        current_function = current_function->next;
    }

    // Снова проходим по всем функциям, кроме main
    current_function = program_node->body;
    while (current_function) {
        if (current_function->type == NODE_FUNCTION && strcmp(current_function->name, "main") != 0) {
            fprintf(stderr, "DEBUG: Found function '%s'. Generating its code.\n", current_function->name);
            codegen_generate_function(current_function);
        }
        current_function = current_function->next;
    }

    // Генерируем вызов main
    fprintf(stderr, "DEBUG: Calling main function.\n");
    fprintf(output_file, "CALL main\n");
    fprintf(output_file, "EXIT int@0\n");
}

void codegen_generate_function(ASTNode *function) {
    fprintf(output_file, "LABEL %s\n", function->name);
    fprintf(output_file, "CREATEFRAME\n");
    fprintf(output_file, "PUSHFRAME\n");

    for (int i = 0; i < function->param_count; i++) {
        fprintf(output_file, "DEFVAR LF@%s\n", function->parameters[i]->name);
        fprintf(output_file, "POPS LF@%s\n", function->parameters[i]->name);
    }

    // Объявление переменной для возврата
    fprintf(output_file, "DEFVAR LF@%%retval\n");

    // Генерация тела функции
    codegen_generate_block(function->body, function->name);

    fprintf(output_file, "POPFRAME\n");
    fprintf(output_file, "RETURN\n");
}

// Генерация блока
void codegen_generate_block(ASTNode *block, const char *function_name) {
    ASTNode *current = block->body;
    while (current) {
        codegen_generate_statement(current, function_name);
        current = current->next;
    }
}

// Генерация операторов
void codegen_generate_statement(ASTNode *statement, const char *function_name) {
    switch (statement->type) {
        case NODE_VARIABLE_DECLARATION: {
            fprintf(stderr, "DEBUG: Declaring variable '%s' in function '%s'\n", statement->name, function_name);
            fprintf(output_file, "DEFVAR LF@%s\n", statement->name);
            if (statement->left) {
                codegen_generate_expression(output_file, statement->left, function_name);
                fprintf(output_file, "POPS LF@%s\n", statement->name);
            }
            break;
        }
        case NODE_ASSIGNMENT: {
            codegen_generate_expression(output_file,statement->left, function_name);
            fprintf(output_file, "POPS LF@%s\n", statement->name);
            break;
        }
        case NODE_RETURN: {
            fprintf(stderr, "DEBUG: Handling return statement in function '%s'\n", function_name);
            if (statement->left) {
                codegen_generate_expression(output_file, statement->left, function_name);
                fprintf(output_file, "POPS LF@%%retval\n");
            }
            fprintf(output_file, "RETURN\n");
            break;
        }
        case NODE_FUNCTION_CALL: {
            fprintf(stderr, "DEBUG: Handling function call '%s' in function '%s'\n", statement->name, function_name);

            // Генерация вызова функции
            codegen_generate_function_call(output_file, statement, function_name);

            break;
        }
        default:
            fprintf(stderr, "DEBUG: Unsupported statement type %d in function '%s'\n", statement->type, function_name);
            exit(1);
    }
}

void codegen_generate_function_call(FILE *output, ASTNode *call, const char *current_function) {
    fprintf(stderr, "DEBUG: Generating function call for '%s' in function '%s'\n", call->name, current_function);
    if (!strcmp(call->name, "ifj.write")) {
        // Обрабатываем предопределённую функцию ifj.write
        fprintf(stderr, "DEBUG: Generating write instruction for 'ifj.write'\n");
        codegen_generate_expression(output, call->arguments[0], current_function);
        fprintf(output, "POPS LF@%s\n", call->arguments[0]->name);
        fprintf(output, "WRITE LF@%s\n", call->arguments[0]->name);
    } else if (!strcmp(call->name, "ifj.readstr")) {
        fprintf(output, "READ LF@%%retval string\n");
    } else if (!strcmp(call->name, "ifj.readi32")) {
        fprintf(output, "READ LF@%%retval int\n");
    } else if (!strcmp(call->name, "ifj.readf64")) {
        fprintf(output, "READ LF@%%retval float\n");
    } else if (!strcmp(call->name, "ifj.concat")) {
        // Конкатенация строк
        codegen_generate_expression(output_file, call->arguments[0], current_function);
        codegen_generate_expression(output_file, call->arguments[1], current_function);
        fprintf(output, "CONCAT LF@%%retval\n");
    } else if (!strcmp(call->name, "ifj.length")) {
        // Длина строки
        codegen_generate_expression(output_file, call->arguments[0], current_function);
        fprintf(output, "STRLEN LF@%%retval\n");
    } else if (!strcmp(call->name, "ifj.i2f")) {
        // Преобразование int в float
        codegen_generate_expression(output_file, call->arguments[0], current_function);
        fprintf(output, "INT2FLOAT LF@%%retval\n");
    } else if (!strcmp(call->name, "ifj.f2i")) {
        // Преобразование float в int
        (output_file, call->arguments[0], current_function);
        fprintf(output, "FLOAT2INT LF@%%retval\n");
    } else if (!strcmp(call->name, "ifj.substring")) {
        // Подстрока
        codegen_generate_expression(output_file, call->arguments[0], current_function);
        codegen_generate_expression(output_file, call->arguments[1], current_function);
        codegen_generate_expression(output_file, call->arguments[2], current_function);
        fprintf(output, "SUBSTR LF@%%retval\n");
    } else if (!strcmp(call->name, "ifj.ord")) {
        // Получение кода символа
        codegen_generate_expression(output_file, call->arguments[0], current_function);
        codegen_generate_expression(output_file, call->arguments[1], current_function);
        fprintf(output, "ORD LF@%%retval\n");
    } else if (!strcmp(call->name, "ifj.chr")) {
        // Получение символа по коду
        codegen_generate_expression(output_file, call->arguments[0], current_function);
        fprintf(output, "INT2CHAR LF@%%retval\n");
    } else {
        // Обычные вызовы функций
        for (int i = call->arg_count - 1; i >= 0; i--) {
            codegen_generate_expression(output, call->arguments[i], current_function);
        }

        fprintf(output, "CALL %s\n", call->name);

        // Если функция возвращает значение
        if (call->left) {
            fprintf(output, "MOVE LF@%s LF@%%retval\n", call->left->name);
        }
    }
}

void codegen_generate_expression(FILE *output_file, ASTNode *expression, const char *function_name) {
    if (!expression) {
        fprintf(stderr, "Error: NULL expression node.\n");
        exit(1);
    }

    switch (expression->type) {
        case NODE_LITERAL:
            switch (expression->data_type) {
                case TYPE_INT:
                    fprintf(output_file, "PUSHS int@%s\n", expression->value);
                    break;
                case TYPE_FLOAT:
                    fprintf(output_file, "PUSHS float@%s\n", expression->value);
                    break;
                case TYPE_U8: {
                    char *escaped_string = escape_ifj24_string(expression->value);
                    fprintf(output_file, "PUSHS string@%s\n", escaped_string);
                    free(escaped_string);
                    break;
                }
                case TYPE_BOOL:
                    fprintf(output_file, "PUSHS bool@%s\n", strcmp(expression->value, "true") == 0 ? "true" : "false");
                    break;
                case TYPE_NULL:
                    fprintf(output_file, "PUSHS nil@nil\n");
                    break;
                default:
                    fprintf(stderr, "Error: Unsupported literal data type %d.\n", expression->data_type);
                    exit(1);
            }
            break;

        case NODE_IDENTIFIER:
            fprintf(output_file, "PUSHS LF@%s\n", expression->name);
            break;

        case NODE_BINARY_OPERATION:
            codegen_generate_expression(output_file,expression->left, function_name);
            codegen_generate_expression(output_file,expression->right, function_name);

            if (strcmp(expression->name, "+") == 0) {
                fprintf(output_file, "ADDS\n");
            } else if (strcmp(expression->name, "-") == 0) {
                fprintf(output_file, "SUBS\n");
            } else if (strcmp(expression->name, "*") == 0) {
                fprintf(output_file, "MULS\n");
            } else if (strcmp(expression->name, "/") == 0) {
                fprintf(output_file, "DIVS\n");
            } else {
                fprintf(stderr, "Error: Unsupported binary operator '%s'.\n", expression->name);
                exit(1);
            }
            break;

        case NODE_FUNCTION_CALL:
            codegen_generate_function_call(output_file,expression, function_name);
            break;

        default:
            fprintf(stderr, "Error: Unsupported expression type %d.\n", expression->type);
            exit(1);
    }
}

// Функция для экранирования строк в формате IFJcode24
char *escape_ifj24_string(const char *input) {
    size_t length = strlen(input);
    size_t buffer_size = length * 4 + 1; // резервируем место для экранирования
    char *escaped_string = malloc(buffer_size);
    if (!escaped_string) {
        fprintf(stderr, "Memory allocation failed for escaped string.\n");
        exit(1);
    }

    size_t index = 0;
    for (size_t i = 0; i < length; i++) {
        unsigned char c = input[i];
        // Экранируем неразрешённые символы и управляющие символы
        if (c <= 32 || c == 35 || c == 92 || c >= 127) { // ASCII 0-32, 35 (#), 92 (\), 127+
            index += snprintf(escaped_string + index, buffer_size - index, "\\%03d", c);
        } else {
            escaped_string[index++] = c;
        }
    }

    escaped_string[index] = '\0';
    return escaped_string;
}
