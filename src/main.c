/*
 * Project Name: Implementation of the IFJ24 Imperative Language Compiler
 * Authors: [List the names and logins of team members who worked on this file]
 */

#include <stdio.h>
#include <stdlib.h>
#include "scanner.h"
#include "parser.h"
#include "codegen.h"
#include "error.h"
#include "symtable.h"

int main(int argc, char *argv[]) {
    // Проверяем количество аргументов командной строки
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
        return ERR_INTERNAL;  // Код 99 согласно спецификации для внутренних ошибок
    }

    // Открываем исходный файл для чтения
    FILE *source_file = fopen(argv[1], "r");
    if (!source_file) {
        fprintf(stderr, "Error opening file: %s\n", argv[1]);
        return ERR_INTERNAL;  // Код 99 согласно спецификации для внутренних ошибок
    }

    // Создаем и инициализируем сканер
    Scanner scanner;
    scanner_init(source_file, &scanner);

    // Инициализируем генератор кода
    codegen_init(stdout); // Вывод на стандартный вывод

    // Начинаем генерацию кода для программы
    codegen_program_start();

    // Инициализируем парсер с указанием сканера
    parser_init(&scanner); 

    // Запускаем синтаксический анализатор (если он возвращает код ошибки, обрабатываем его)
    parse_program(&scanner);

    // Завершаем генерацию кода
    codegen_program_end();
    codegen_finalize();

    // Освобождаем ресурсы сканера
    scanner_free(&scanner);

    // Закрываем исходный файл
    fclose(source_file);

    return ERR_OK;  // Код 0 согласно спецификации для успешного выполнения
}