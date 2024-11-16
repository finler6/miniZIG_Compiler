#include "codegen.h"
#include "ast.h"
#include "symtable.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "scanner.h"

// Global variable for storing the output file
static FILE *output_file;

void codegen_generate_function(ASTNode *function_node);
void codegen_generate_block(FILE *output, ASTNode *block_node);
void codegen_generate_statement(FILE *output, ASTNode *statement_node);
void codegen_generate_expression(FILE *output, ASTNode *node);
void codegen_generate_function_call(FILE *output, ASTNode *node);
void codegen_generate_variable_declaration(FILE *output, ASTNode *declaration_node);
void codegen_generate_assignment(FILE *output, ASTNode *assignment_node);
void codegen_generate_return(FILE *output, ASTNode *return_node);
void codegen_generate_if(FILE *output, ASTNode *if_node);
void codegen_generate_while(FILE *output, ASTNode *while_node);

char *escape_ifj24_string(const char *input);
static int label_counter = 0;

int generate_unique_label() {
    return label_counter++;
}

// Initialize code generator
void codegen_init(const char *filename) {
    output_file = fopen(filename, "w");
    if (!output_file) {
        fprintf(stderr, "Error: Cannot open output file %s for writing.\n", filename);
        exit(EXIT_FAILURE);
    }
    fprintf(output_file, ".IFJcode24\n"); // Header for output file
}

// Finalize code generation
void codegen_finalize() {
    if (output_file) {
        fclose(output_file);
        output_file = NULL;
    }
}

// Function to generate program code
void codegen_generate_program(ASTNode *program_node) {
    if (!program_node || program_node->type != NODE_PROGRAM) {
        fprintf(stderr, "Error: Invalid program node.\n");
        return;
    }

    ASTNode *current_function = program_node->body;
    while (current_function) {
        if (current_function->type == NODE_FUNCTION) {
            codegen_generate_function(current_function);
        }
        current_function = current_function->next;
    }
}

void codegen_generate_function(ASTNode *function_node) {
    fprintf(output_file, "CREATEFRAME\n");
    fprintf(output_file, "PUSHFRAME\n");
    fprintf(output_file, "LABEL %s\n", function_node->name);

    // Обработка параметров функции
    for (int i = 0; i < function_node->param_count; ++i) {
        fprintf(output_file, "DEFVAR LF@%s\n", function_node->parameters[i]->name);
        fprintf(output_file, "POPS LF@%s\n", function_node->parameters[i]->name);
    }

    // Определение переменной для возврата
    fprintf(output_file, "DEFVAR LF@%%retval\n");

    // Генерация кода для тела функции
    codegen_generate_block(output_file, function_node->body);

    // Убедимся, что main завершается без RETURN, а другие функции корректно выходят с RETURN и POPFRAME
    if (strcmp(function_node->name, "main") == 0) {
        fprintf(output_file, "EXIT int@0\n");  // EXIT для завершения main
    } else {
        fprintf(output_file, "POPFRAME\n");    // В обычных функциях завершаем POPFRAME и RETURN
        fprintf(output_file, "RETURN\n");
    }
}

// Function to generate code for a block of statements
void codegen_generate_block(FILE *output, ASTNode *block_node) {
    ASTNode *current_statement = block_node->body;
    while (current_statement) {
        codegen_generate_statement(output, current_statement);
        current_statement = current_statement->next;
    }
}

void codegen_generate_expression(FILE *output, ASTNode *node) {
    if (node == NULL) {
        return;
    }

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
            } else if (node->data_type == SYMBOL_VARIABLE) {
                fprintf(output, "PUSHS nil@nil\n");
            }
            break;

        case NODE_IDENTIFIER:
            fprintf(output, "PUSHS LF@%s\n", node->name);
            break;

        case NODE_BINARY_OPERATION:
            codegen_generate_expression(output, node->left);
            codegen_generate_expression(output, node->right);

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
                fprintf(output, "LTS\n");
                fprintf(output, "PUSHS bool@true\n");
                fprintf(output, "ORS\n");
            } else if (strcmp(node->name, ">") == 0) {
                fprintf(output, "GTS\n");
            } else if (strcmp(node->name, ">=") == 0) {
                fprintf(output, "GTS\n");
                fprintf(output, "PUSHS bool@true\n");
                fprintf(output, "ORS\n");
            } else if (strcmp(node->name, "==") == 0) {
                fprintf(output, "EQS\n");
            } else if (strcmp(node->name, "!=") == 0) {
                fprintf(output, "EQS\n");
                fprintf(output, "NOTS\n");
            }
            break;

        case NODE_FUNCTION_CALL:
            codegen_generate_function_call(output, node);
            break;

        default:
            fprintf(stderr, "Unsupported expression type for code generation\n");
            exit(1);
    }
}

void codegen_generate_function_call(FILE *output, ASTNode *node) {
    if (node == NULL || node->type != NODE_FUNCTION_CALL) {
        fprintf(stderr, "Invalid function call node for code generation\n");
        exit(1);
    }

    // Проверка на встроенную функцию ifj.write
    if (strcmp(node->name, "ifj.write") == 0) {
        ASTNode *arg = node->arguments[0];

        // Обработка вывода, используя аргументы
        if (arg->type == NODE_LITERAL) {
            if (arg->data_type == TYPE_U8) {
                char *escaped_value = escape_ifj24_string(arg->value);
                fprintf(output, "WRITE string@%s\n", escaped_value);
                free(escaped_value);
            } else if (arg->data_type == TYPE_INT) {
                fprintf(output, "WRITE int@%s\n", arg->value);
            } else if (arg->data_type == TYPE_FLOAT) {
                double float_value = atof(arg->value);
                fprintf(output, "WRITE float@%.13a\n", float_value);
            }
        } else if (arg->type == NODE_IDENTIFIER) {
            fprintf(output, "WRITE LF@%s\n", arg->name);
        }
    }
        // Обработка только функций чтения ifj.readi32, ifj.readf64, ifj.readstr
    else if (strcmp(node->name, "ifj.readi32") == 0) {
        fprintf(output, "READ LF@%%retval int\n");
    } else if (strcmp(node->name, "ifj.readf64") == 0) {
        fprintf(output, "READ LF@%%retval float\n");
    } else if (strcmp(node->name, "ifj.readstr") == 0) {
        fprintf(output, "READ LF@%%retval string\n");
    } else {
        // Подготовка аргументов для пользовательской функции
        for (int i = node->arg_count - 1; i >= 0; --i) {
            codegen_generate_expression(output, node->arguments[i]);
        }

        // Вызов пользовательской функции и сохранение результата в %retval
        fprintf(output, "CALL %s\n", node->name);
    }
}

void codegen_generate_statement(FILE *output, ASTNode *node) {
    if (node == NULL) {
        fprintf(stderr, "Invalid statement node for code generation\n");
        exit(1);
    }

    switch (node->type) {
        case NODE_VARIABLE_DECLARATION:
            fprintf(output, "DEFVAR LF@%s\n", node->name);
            if (node->left != NULL) {
                if (node->left->type == NODE_FUNCTION_CALL &&
                    (strcmp(node->left->name, "ifj.readi32") == 0 ||
                     strcmp(node->left->name, "ifj.readf64") == 0 ||
                     strcmp(node->left->name, "ifj.readstr") == 0)) {
                    if (strcmp(node->left->name, "ifj.readi32") == 0) {
                        fprintf(output, "READ LF@%%retval int\n");
                    } else if (strcmp(node->left->name, "ifj.readf64") == 0) {
                        fprintf(output, "READ LF@%%retval float\n");
                    } else if (strcmp(node->left->name, "ifj.readstr") == 0) {
                        fprintf(output, "READ LF@%%retval string\n");
                    }
                    fprintf(output, "MOVE LF@%s LF@%%retval\n", node->name);
                } else {
                    codegen_generate_expression(output, node->left);
                    fprintf(output, "POPS LF@%s\n", node->name);
                }
            }
            break;

        case NODE_ASSIGNMENT:
            codegen_generate_expression(output, node->left);
            if (node->left->type == NODE_FUNCTION_CALL &&
                (strcmp(node->left->name, "ifj.readi32") == 0 ||
                 strcmp(node->left->name, "ifj.readf64") == 0 ||
                 strcmp(node->left->name, "ifj.readstr") == 0)) {
                if (strcmp(node->left->name, "ifj.readi32") == 0) {
                    fprintf(output, "READ LF@%%retval int\n");
                } else if (strcmp(node->left->name, "ifj.readf64") == 0) {
                    fprintf(output, "READ LF@%%retval float\n");
                } else if (strcmp(node->left->name, "ifj.readstr") == 0) {
                    fprintf(output, "READ LF@%%retval string\n");
                }
                fprintf(output, "MOVE LF@%s LF@%%retval\n", node->name);
            } else {
                fprintf(output, "POPS LF@%s\n", node->name);
            }
            break;

        case NODE_RETURN:
            // Generate code for return statement
            if (node->left != NULL) {
                codegen_generate_expression(output, node->left);
                fprintf(output, "POPS LF@%%retval\n");
            }
            fprintf(output, "RETURN\n");
            break;

        case NODE_IF:
            // Generate code for if statement
        {
            int label_num = generate_unique_label();
            codegen_generate_expression(output, node->condition);
            fprintf(output, "PUSHS bool@true\n");
            fprintf(output, "JUMPIFNEQS $else_%d\n", label_num);
            codegen_generate_block(output, node->body); // Generate code for if body
            fprintf(output, "JUMP $endif_%d\n", label_num);
            fprintf(output, "LABEL $else_%d\n", label_num);
            if (node->left != NULL) {
                codegen_generate_block(output, node->left); // Generate code for else body
            }
            fprintf(output, "LABEL $endif_%d\n", label_num);
        }
            break;

        case NODE_WHILE:
            // Generate code for while loop
        {
            int label_num = generate_unique_label();
            fprintf(output, "LABEL $while_start_%d\n", label_num);
            codegen_generate_expression(output, node->condition);
            fprintf(output, "PUSHS bool@true\n");
            fprintf(output, "JUMPIFNEQS $while_end_%d\n", label_num);
            codegen_generate_block(output, node->body);
            fprintf(output, "JUMP $while_start_%d\n", label_num);
            fprintf(output, "LABEL $while_end_%d\n", label_num);
        }
            break;

        case NODE_FUNCTION_CALL:
            // Generate code for function call
            codegen_generate_function_call(output, node);
            break;

        default:
            fprintf(stderr, "Unsupported statement type for code generation\n");
            exit(1);
    }
}

void codegen_generate_variable_declaration(FILE *output, ASTNode *declaration_node) {
    fprintf(output, "DEFVAR LF@%s\n", declaration_node->name);
    if (declaration_node->left) { // If there is an initial value
        codegen_generate_expression(output, declaration_node->left);
        fprintf(output, "POPS LF@%s\n", declaration_node->name);
    }
}

void codegen_generate_assignment(FILE *output, ASTNode *assignment_node) {
    codegen_generate_expression(output, assignment_node->left);
    fprintf(output, "POPS LF@%s\n", assignment_node->name);
}

void codegen_generate_return(FILE *output, ASTNode *return_node) {
    if (return_node->left) { // Если есть возвращаемое значение
        codegen_generate_expression(output, return_node->left);
        fprintf(output, "POPS LF@%%retval\n");
    }
    fprintf(output, "RETURN\n");
}

void codegen_generate_if(FILE *output, ASTNode *if_node) {
    static int if_label_count = 0;
    int current_label = if_label_count++;

    // Проверка на условие вида `num == nil`
    if (if_node->condition->type == NODE_BINARY_OPERATION &&
        strcmp(if_node->condition->name, "==") == 0 &&
        if_node->condition->right->data_type == SYMBOL_VARIABLE) {

        fprintf(output, "PUSHS nil@nil\n"); // Кладем nil на стек для сравнения
        fprintf(output, "PUSHS LF@%s\n", if_node->condition->left->name); // Кладем значение переменной на стек
        fprintf(output, "EQS\n"); // Сравниваем значение с nil
        fprintf(output, "PUSHS bool@true\n");
        fprintf(output, "JUMPIFNEQS $else_%d\n", current_label);

    } else if (if_node->condition->type == NODE_BINARY_OPERATION &&
               strcmp(if_node->condition->name, "!=") == 0 &&
               if_node->condition->right->data_type == SYMBOL_VARIABLE) {

        fprintf(output, "PUSHS nil@nil\n");
        fprintf(output, "PUSHS LF@%s\n", if_node->condition->left->name); // Кладем значение переменной на стек
        fprintf(output, "EQS\n"); // Сравниваем значение с nil
        fprintf(output, "NOTS\n"); // Инвертируем результат, чтобы получить `!= nil`
        fprintf(output, "PUSHS bool@true\n");
        fprintf(output, "JUMPIFNEQS $else_%d\n", current_label);

    } else {
        // Обычная генерация условия
        codegen_generate_expression(output, if_node->condition);
        fprintf(output, "PUSHS bool@true\n");
        fprintf(output, "JUMPIFNEQS $else_%d\n", current_label);
    }

    // Генерация кода для основного блока `if`
    codegen_generate_block(output, if_node->body);
    fprintf(output, "JUMP $endif_%d\n", current_label);
    fprintf(output, "LABEL $else_%d\n", current_label);

    // Генерация кода для блока `else`, если он присутствует
    if (if_node->left != NULL) {
        codegen_generate_block(output, if_node->left);
    }

    fprintf(output, "LABEL $endif_%d\n", current_label);
}

void codegen_generate_while(FILE *output, ASTNode *while_node) {
    static int while_label_count = 0;
    int current_label = while_label_count++;

    // Начальная метка цикла
    fprintf(output, "LABEL $while_start_%d\n", current_label);

    // Генерация выражения для условия цикла
    codegen_generate_expression(output, while_node->condition);

    // Проверка условия. Если значение на стеке меньше или равно нулю, выход из цикла.
    fprintf(output, "PUSHS int@0\n");
    fprintf(output, "GTS\n");
    fprintf(output, "JUMPIFNEQS $while_end_%d\n", current_label);

    // Тело цикла
    codegen_generate_block(output, while_node->body);

    // Переход в начало цикла
    fprintf(output, "JUMP $while_start_%d\n", current_label);

    // Метка завершения цикла
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

