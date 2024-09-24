#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>
#include "tokens.h"

// Структура для хранения состояния сканера
typedef struct {
    FILE *input;         // Входной поток
    int line;            // Текущая строка
    int column;          // Текущая колонка
    int current_char;    // Текущий символ
} Scanner;

// Инициализация сканера
void scanner_init(FILE *input_file, Scanner *scanner);

// Освобождение ресурсов сканера
void scanner_free(Scanner *scanner);

// Получение следующего токена
Token get_next_token();

// Функции для работы с символами
int scanner_is_keyword(const char *lexeme, Scanner *scanner);
int scanner_is_operator(int c, Scanner *scanner);
int scanner_is_delimiter(int c, Scanner *scanner);

#endif // SCANNER_H
