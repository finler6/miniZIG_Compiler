#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include <stdio.h>

typedef struct FunctionInfo {
    char *name;
    int param_count;
    char **parameters;
} FunctionInfo;

static FunctionInfo *functions = NULL;
static int function_count = 0;

// Инициализация генератора кода
void codegen_init(const char *filename);

// Завершение работы генератора кода
void codegen_finalize();

// Генерация кода для всей программы
void codegen_generate_program(ASTNode *program_node);

// Генерация кода для отдельной функции
void codegen_generate_function(ASTNode *function_node);

// Генерация блока
void codegen_generate_block(ASTNode *block_node, const char *current_function);

// Генерация оператора
void codegen_generate_statement(ASTNode *statement_node, const char *current_function);

// Генерация выражений
void codegen_generate_expression(FILE *output, ASTNode *node, const char *current_function);

// Генерация вызова функции
void codegen_generate_function_call(FILE *output, ASTNode *node, const char *current_function);

// Экранная функция
char *escape_ifj24_string(const char *input);

#endif // CODEGEN_H
