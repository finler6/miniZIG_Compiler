#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h" // Подключаем заголовочный файл с определением структуры AST
#include <stdio.h>

typedef struct FunctionInfo {
    char *name;
    int param_count;
    char **parameters;
} FunctionInfo;

static FunctionInfo *functions = NULL;
static int function_count = 0;

// Инициализация генератора кода (например, открытие файлов вывода)
void codegen_init(const char *filename);

// Завершение работы генератора кода (например, закрытие файлов)
void codegen_finalize();

// Функция для генерации кода для всей программы
void codegen_generate_program(ASTNode *program_node);

// Функция для генерации кода для отдельной функции
void codegen_generate_function(ASTNode *function_node);

// Обновим сигнатуры функций, где требуется current_function
void codegen_generate_block(FILE *output, ASTNode *block_node, const char* current_function);
void codegen_generate_statement(FILE *output, ASTNode *statement_node, const char* current_function);
void codegen_generate_expression(FILE *output, ASTNode *node, const char* current_function);
void codegen_generate_function_call(FILE *output, ASTNode *node, const char* current_function);

// Функция для генерации объявления переменной
void codegen_generate_variable_declaration(FILE *output, ASTNode *declaration_node);

// Функция для генерации присваивания
void codegen_generate_assignment(FILE *output, ASTNode *assignment_node);

// Функция для генерации оператора return
void codegen_generate_return(FILE *output, ASTNode *return_node);

// Функция для генерации оператора if
void codegen_generate_if(FILE *output, ASTNode *if_node);

// Функция для генерации оператора while
void codegen_generate_while(FILE *output, ASTNode *while_node);

// Вспомогательная функция для получения уникальной метки (если требуется)
int generate_unique_label();

// Вспомогательная функция для получения имени функции по имени переменной
const char* get_function_name_from_variable(const char* var_name);

// Прохождение по AST и сбор всех функций
void collect_functions(ASTNode *program_node);

#endif // CODEGEN_H
